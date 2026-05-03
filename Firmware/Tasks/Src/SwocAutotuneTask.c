/**
 * @file SwocAutotuneTask.c
 * @brief On-track helper: learns SWOC setpoint-table rows without tripping FaultHandler on SWOC.
 */

#include "SwocAutotuneTask.h"
#include "CANbus.h"
#include "FSMTask.h"
#include "UpdateVCUInputsTask.h"
#include "User_SDCard.h"
#include "printf.h"

#include "stm32xx_hal.h"

#include <math.h>
#include <string.h>

#if ENABLE_SWOC_AUTOTUNE

#define METERS_SEC_TO_MPH_TUN  2.237f
#define SWOC_TUNE_SD_FILE      "SW.TXT"
/** FatFs/async line limit (~63 chars); prefix "t,..." tick */
#define TUNE_LINE_BODY_MAX    54

/* --- Tunables (adjust here between runs) -------------------------------- */
static const float SWOC_TUNE_DELTA_MPH            = 2.0f;   /* breakpoint = SWOC mph - this */
static const float SWOC_TUNE_VERIFY_PASS_DELTA_MPH = 2.0f; /* must exceed bp + this */
static const uint8_t SWOC_TUNE_PERCENT_STEP       = 5U;
static const uint8_t SWOC_TUNE_MIN_TRIAL_PERCENT  = 10U;
/** Merge breakpoints closer than this (mph) replace the stored row */
static const float SWOC_TUNE_SPEED_BUCKET_MPH      = 0.51f;

#define SWOC_TUNE_ROW_CAP 14

typedef enum {
    TUNE_RS_EXPLORE = 0,
    TUNE_RS_VERIFY,
    TUNE_RS_RECOVER_WAIT_ZERO,
    TUNE_RS_RECOVER_SEND_RESET,
} tune_run_state_e;

static tune_run_state_e run_state = TUNE_RS_EXPLORE;
static swoc_threshold_t tune_rows[SWOC_TUNE_ROW_CAP];
static uint8_t tune_n = 0;

/** Trial breakpoint/target % after an explore SWOC; applied to LUT only in VERIFY state. */
static bool pending_have = false;
static float pending_break_mph = 0.0f;
static uint8_t pending_pct = 100U;

static uint8_t swoc_prev_sample = 0U;

/* Verify-session tracking (reset after each MC reset exiting recovery) */
static float verify_peak_mph = 0.0f;
static bool verify_saw_above_break = false;

static float tune_lut_cap(float speed_mph);

static inline bool tune_apply_pending_lut(void) {
    return pending_have && (run_state == TUNE_RS_VERIFY);
}

static float get_drive_current_sample_only(float speed_mph, uint8_t accel_percent_0_100) {
    uint8_t pedal = accel_percent_0_100;
    if (pedal <= ACCEL_DEADZONE_MIN) {
        pedal = 0U;
    }
    float requested = PERCENT_TO_CURRENT_SETPOINT(pedal);
    float swoc_cap = tune_lut_cap(speed_mph);
    return fminf(swoc_cap, requested);
}

static float sample_speed_mph(void) {
    return fabsf(g_data_read->motor_velocity.MC_VehicleVelocity) * METERS_SEC_TO_MPH_TUN;
}

/** Recovery waits for literal zero CAN vehicle velocity (m/s), not a low-speed epsilon. */
static bool tune_vehicle_speed_is_exactly_zero(void) {
    return g_data_read->motor_velocity.MC_VehicleVelocity == 0.0f;
}

static float sample_current_setpoint_0_1(void) {
    uint8_t p = g_data_read->accel_brake.AccelPedal_Main_Pos;
    return get_drive_current_sample_only(sample_speed_mph(), p);
}

static void tune_rows_sort(void) {
    for (unsigned i = 0; i + 1U < tune_n; ++i) {
        for (unsigned j = 0; j + 1U < tune_n; ++j) {
            if (tune_rows[j].speed_mph > tune_rows[j + 1].speed_mph) {
                swoc_threshold_t t = tune_rows[j];
                tune_rows[j] = tune_rows[j + 1];
                tune_rows[j + 1] = t;
            }
        }
    }
}

/** Merge breakpoints after sort; same speed bucket keeps stricter (min %) row */
static void tune_rows_coalesce_buckets(void) {
    if (tune_n == 0U) {
        return;
    }
    float last_spd = tune_rows[0].speed_mph;
    uint8_t w = 0;
    tune_rows[w] = tune_rows[0];
    for (unsigned i = 1U; i < tune_n; ++i) {
        if (fabsf(tune_rows[i].speed_mph - last_spd) <= SWOC_TUNE_SPEED_BUCKET_MPH) {
            /* same bucket — keep lower max_percent (stricter cap) */
            if (tune_rows[i].max_percent < tune_rows[w].max_percent) {
                tune_rows[w].max_percent = tune_rows[i].max_percent;
            }
        } else {
            ++w;
            tune_rows[w] = tune_rows[i];
            last_spd = tune_rows[i].speed_mph;
        }
    }
    tune_n = (uint8_t)(w + 1U);
}

static void tune_row_insert_sorted(float speed_mph, uint8_t max_percent) {
    if (tune_n >= SWOC_TUNE_ROW_CAP) {
        return;
    }
    tune_rows[tune_n].speed_mph = speed_mph;
    tune_rows[tune_n].max_percent = max_percent;
    ++tune_n;
    tune_rows_sort();
    tune_rows_coalesce_buckets();
}

/** Piecewise LUT: last matching row wins (same semantics as production SWOC table). */
static float tune_lut_walk(const swoc_threshold_t *rows, uint8_t n, float vmph,
                           float pending_spd, uint8_t pending_max_pct, bool use_pending) {
    float cap = 1.0f;
    for (unsigned i = 0U; i < (unsigned)n; ++i) {
        if (vmph >= rows[i].speed_mph) {
            cap = PERCENT_TO_CURRENT_SETPOINT(rows[i].max_percent);
        }
    }
    if (use_pending && vmph >= pending_spd) {
        cap = PERCENT_TO_CURRENT_SETPOINT(pending_max_pct);
    }
    return cap;
}

static float tune_lut_cap(float speed_mph) {
    bool use_pen = tune_apply_pending_lut();
    return tune_lut_walk(tune_rows, tune_n, speed_mph,
                         pending_break_mph, pending_pct, use_pen && pending_have);
}

static uint8_t tune_pct_at_speedCommittedOnly(float vmph) {
    float frac = tune_lut_walk(tune_rows, tune_n, vmph, 0.0f, 0U, false);
    return (uint8_t)(fminf(100.0f, frac * 100.0f + 0.5f));
}

static void tune_sd_line(const char *msg) {
    char line[SD_DATA_BUFFER_LEN];
    unsigned long tick = HAL_GetTick() / 50UL;
    (void)snprintf(line, sizeof line, "%04lu %s\r\n", tick, msg);
    (void)SDCard_Write(SWOC_TUNE_SD_FILE, line, 0);
}

static void tune_log_swoc(float vmph_swoc, float curr_set_0_1, const char *tag) {
    char body[TUNE_LINE_BODY_MAX + 1];
    (void)snprintf(body, sizeof body, "SWOC %s v=%.1f i=%.2f", tag, (double)vmph_swoc,
                     (double)curr_set_0_1);
    tune_sd_line(body);
}

static void tune_dump_table_to_sd(void) {
    tune_sd_line("--- TABLE COPY ---");
    char body[TUNE_LINE_BODY_MAX + 1];
    (void)snprintf(body, sizeof body, "ROWS %u", (unsigned)tune_n);
    tune_sd_line(body);
    for (unsigned i = 0U; i < (unsigned)tune_n; ++i) {
        (void)snprintf(body, sizeof body, "{%.1ff,%u},", (double)tune_rows[i].speed_mph,
                       (unsigned)tune_rows[i].max_percent);
        tune_sd_line(body);
    }
    tune_sd_line("--- END TABLE ---");
}

static void tune_begin_verify_session(void) {
    verify_peak_mph = 0.0f;
    verify_saw_above_break = false;
}

static void tune_on_swoc_explore(float vmph_at_swoc) {
    pending_break_mph = fmaxf(0.0f, vmph_at_swoc - SWOC_TUNE_DELTA_MPH);
    if (tune_n == 0U) {
        pending_pct = (uint8_t)(100U - SWOC_TUNE_PERCENT_STEP);
    } else {
        uint8_t base = tune_pct_at_speedCommittedOnly(vmph_at_swoc);
        if (base > SWOC_TUNE_PERCENT_STEP) {
            pending_pct = (uint8_t)(base - SWOC_TUNE_PERCENT_STEP);
        } else {
            pending_pct = SWOC_TUNE_MIN_TRIAL_PERCENT;
        }
        if (pending_pct < SWOC_TUNE_MIN_TRIAL_PERCENT) {
            pending_pct = SWOC_TUNE_MIN_TRIAL_PERCENT;
        }
    }
    pending_have = true;
    run_state = TUNE_RS_RECOVER_WAIT_ZERO;
    char b[TUNE_LINE_BODY_MAX + 1];
    (void)snprintf(b, sizeof b, "EXPLORE done bp=%.1f p=%u", (double)pending_break_mph,
                   (unsigned)pending_pct);
    tune_sd_line(b);
}

static void tune_on_swoc_verify_fail(float vmph_at_swoc) {
    (void)vmph_at_swoc;
    if (pending_pct > SWOC_TUNE_MIN_TRIAL_PERCENT + SWOC_TUNE_PERCENT_STEP) {
        pending_pct = (uint8_t)(pending_pct - SWOC_TUNE_PERCENT_STEP);
    } else {
        pending_pct = SWOC_TUNE_MIN_TRIAL_PERCENT;
        tune_sd_line("WARN trial pct floor");
    }
    run_state = TUNE_RS_RECOVER_WAIT_ZERO;
    char b[TUNE_LINE_BODY_MAX + 1];
    (void)snprintf(b, sizeof b, "VERIFY fail next p=%u", (unsigned)pending_pct);
    tune_sd_line(b);
}

static void tune_commit_pending(void) {
    tune_row_insert_sorted(pending_break_mph, pending_pct);
    pending_have = false;
    run_state = TUNE_RS_EXPLORE;
    tune_sd_line("COMMIT row ok");
    tune_dump_table_to_sd();
}

static void tune_send_mc_reset_sequence(void) {
    for (unsigned k = 0U; k < 3U; ++k) {
        (void)MotorCAN_Send_Reset_Cmd(0U, pdMS_TO_TICKS(10));
    }
    tune_sd_line("MC reset 403 0 x3");
}

static void tune_recovery_tick(void) {
    if (run_state == TUNE_RS_RECOVER_WAIT_ZERO) {
        if (tune_vehicle_speed_is_exactly_zero()) {
            run_state = TUNE_RS_RECOVER_SEND_RESET;
        }
        return;
    }

    if (run_state == TUNE_RS_RECOVER_SEND_RESET) {
        tune_send_mc_reset_sequence();
        swoc_prev_sample = g_data_read->motor_status.MC_FAULT_SoftwareOverCurrent;
        tune_begin_verify_session();
        run_state = TUNE_RS_VERIFY;
        tune_sd_line("RECOVER ok go VERIFY");
    }
}

static void tune_explore_or_verify_tick(void) {
    uint8_t sw = g_data_read->motor_status.MC_FAULT_SoftwareOverCurrent;
    bool rising_swoc = (sw != 0U) && (swoc_prev_sample == 0U);
    swoc_prev_sample = sw;

    float vmph = sample_speed_mph();
    bool pedal = g_data_read->accel_brake.AccelPedal_Main_Pos > ACCEL_DEADZONE_MIN;

    if (rising_swoc) {
        float iset = sample_current_setpoint_0_1();
        if (run_state == TUNE_RS_EXPLORE) {
            tune_log_swoc(vmph, iset, "explore");
            tune_on_swoc_explore(vmph);
        } else if (run_state == TUNE_RS_VERIFY) {
            tune_log_swoc(vmph, iset, "verify");
            tune_on_swoc_verify_fail(vmph);
        }
        return;
    }

    if (run_state == TUNE_RS_VERIFY && pending_have) {
        if (pedal) {
            if (vmph > verify_peak_mph) {
                verify_peak_mph = vmph;
            }
            if (vmph >= pending_break_mph) {
                verify_saw_above_break = true;
            }
        }
        if (verify_saw_above_break &&
            verify_peak_mph >= (pending_break_mph + SWOC_TUNE_VERIFY_PASS_DELTA_MPH)) {
            tune_commit_pending();
        }
    }
}

void SwocAutotune_Init(void) {
    if (SDCard_Init() != SD_OK) {
        printf("SwocAutotune: SD init failed\r\n");
    }
    tune_sd_line("BOOT swoc_autotune");
}

float SwocAutotune_MaxCurrentFraction(float speed_mph) {
    return tune_lut_cap(speed_mph);
}

void SwocAutotune_OnInputCycleComplete(void) {
    if (!fsm_is_input_set(PRECHARGE_COMPLETE_BIT)) {
        return;
    }

    switch (run_state) {
    case TUNE_RS_RECOVER_WAIT_ZERO:
    case TUNE_RS_RECOVER_SEND_RESET:
        tune_recovery_tick();
        break;
    case TUNE_RS_EXPLORE:
    case TUNE_RS_VERIFY:
        tune_explore_or_verify_tick();
        break;
    default:
        break;
    }
}

#else /* !ENABLE_SWOC_AUTOTUNE */

void SwocAutotune_Init(void) {}

float SwocAutotune_MaxCurrentFraction(float speed_mph) {
    (void)speed_mph;
    return 1.0f;
}

void SwocAutotune_OnInputCycleComplete(void) {}

#endif /* ENABLE_SWOC_AUTOTUNE */
