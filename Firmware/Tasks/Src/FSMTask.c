/**
 * @file FSM.c
 * @brief Motor controller FSM implementation
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#define DEFINE_FSM_TABLE
#include "fsm_table_dnr.h"
#include "rollover_speed_table.h"

#include "FSMTask.h"
#include "InitTask.h"
#include "UpdateVCUInputsTask.h"
#include "FaultBits.h"
#include "Watchdogs.h"
#include "StatusLEDs.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_VELOCITY        12000
#define MAX_CURRENT_PERCENT 1.0f

// Minimum 5% pedal pressed to register accel input, prevents ghost inputs :P
#define ACCEL_DEADZONE_MIN  5u 
#define METERS_SEC_TO_MPH   2.23694f

// Max increase in commanded current (0.0-1.0) allowed per second. Re-entering
// drive from neutral/brake jumps straight from 0 current to whatever the
// pedal is currently asking for; this caps how fast that climb can happen.
// Untested against hardware - tune up/down while watching for
// MC_FAULT_SoftwareOverCurrent. Decreases in current are never limited.
#define CURRENT_RAMP_PER_SECOND 0.5556f 

// Per-tick step derived from the rate above, so it stays correct if
// FSM_TASK_DELAY_MS ever changes.
#define CURRENT_RAMP_PER_TICK   (CURRENT_RAMP_PER_SECOND * ((float)FSM_TASK_DELAY_MS / 1000.0f))

StaticEventGroup_t fsmInputBuffer = {0};
EventGroupHandle_t fsmInputGroup = {0};
MocoState_t current_state = {0};

static bool rollover_limit_active = false;
static volatile uint16_t fsm_inputs = 0;
// Shared ramp accumulator for ramp_current(); safe because only one state's
// handler runs per tick (current_state is a single active state).
static float last_sent_current = 0.0f;

// forward declarations for FSM[] handler table wiring in fsm_init()
static void handle_state_init(void);
static void handle_state_not_ready(void);
static void handle_state_forward(void);
static void handle_state_neutral(void);
static void handle_state_reverse(void);
static void handle_state_regen(void);
static void handle_state_cruise(void);
static void handle_state_disabled(void);

// forward declarations for helper functions
static void handle_drive_state(bool);
// static float apply_rollover_limit(float requested_current);
// static float swoc_max_current(float speed_mph);
static float get_drive_current(float speed_mph, uint8_t accel_percent_0_100);
static float ramp_current(float target_current);
static void reset_current_ramp(void);

void fsm_init(void) {
    FSM[STATE_INIT].stateHandler = handle_state_init;
    FSM[FORWARD_DRIVE].stateHandler = handle_state_forward;
    FSM[NEUTRAL_DRIVE].stateHandler = handle_state_neutral;
    FSM[REVERSE_DRIVE].stateHandler = handle_state_reverse;
    FSM[REGEN].stateHandler = handle_state_regen;
    FSM[CRUISE_CONTROL].stateHandler = handle_state_cruise;
    FSM[DISABLED].stateHandler = handle_state_disabled;
    FSM[CAR_NOT_READY].stateHandler = handle_state_not_ready;

    current_state = FSM[STATE_INIT];
    current_state.stateHandler();

    fsmInputGroup = xEventGroupCreateStatic(&fsmInputBuffer);
}

void fsm_step(void) {
    fsm_inputs = xEventGroupGetBits(fsmInputGroup);
    current_state = FSM[current_state.NextStates[fsm_inputs]];
    if (current_state.stateHandler) current_state.stateHandler();
}

void fsm_disable(void) { current_state = FSM[DISABLED]; }
void fsm_recover(void) { current_state = FSM[CAR_NOT_READY]; }
uint16_t fsm_get_fsm_inputs(void) { return (uint16_t)fsm_inputs; }
bool fsm_is_over_rollover_speed(void) { return rollover_limit_active; }

void fsm_set_all_inputs(EventBits_t mask) {
    taskENTER_CRITICAL();
    xEventGroupClearBits(fsmInputGroup, FSM_INPUTS_MASK_ALL);
    xEventGroupSetBits(fsmInputGroup, mask & FSM_INPUTS_MASK_ALL);
    taskEXIT_CRITICAL();
}

void fsm_set_input(EventBits_t mask) {
    xEventGroupSetBits(fsmInputGroup, mask & FSM_INPUTS_MASK_ALL);
}

uint16_t fsm_get_inputs(void) {
    return (uint16_t)xEventGroupGetBits(fsmInputGroup);
}

bool fsm_is_input_set(InputBits_t bit) {
    return (xEventGroupGetBits(fsmInputGroup) & bit) != 0;
}



// goofy ahh logic, uses lut
// static float apply_rollover_limit(float requested_current) {
//     // printf("Applying rollover limit, requested current: %.2f, vehicle velocity: %.2f, steering angle: %.1f\r\n",
//         //    requested_current, g_data_read->motor_velocity.MC_VehicleVelocity, (float)g_data_read->lws.LWS_Angle / 10.0f);
//     int deg = abs((int)g_data_read->lws.LWS_Angle) / 10;
//     if (deg > (int)ROLLOVER_TABLE_MAX_DEG) deg = (int)ROLLOVER_TABLE_MAX_DEG;

//     uint16_t v_max_cms = rollover_speed_table[deg];
//     uint16_t v_now_cms = (uint16_t)(g_data_read->motor_velocity.MC_VehicleVelocity * 100.0f);

//     if (v_max_cms != ROLLOVER_TABLE_NO_LIMIT && v_now_cms > v_max_cms) {
//         rollover_limit_active = true;
//         warning_set(WARNING_ID_TIPPING_LIMIT_ACTIVE);
//         return 0.0f;
//     }
    
//     warning_clear(WARNING_ID_REGEN_NOT_ALLOWED);
//     rollover_limit_active = false;
//     return requested_current;
// }

float map_to_percent(uint8_t input, uint8_t in_min, uint8_t in_max, uint8_t out_min,
                     uint8_t out_max) {
    if (in_min >= in_max || input <= in_min) return (float)out_min / 100.0f;
    if (input >= in_max) return (float)out_max / 100.0f;
    uint16_t oi = input - in_min;
    uint16_t ir = in_max - in_min;
    uint16_t or_ = out_max - out_min;
    return ((float)(oi * or_) / (float)ir + (float)out_min) / 100.0f;
}

static float get_drive_current(float speed_mph, uint8_t accel_percent_0_100) {
    uint8_t pedal = accel_percent_0_100;
    if (pedal <= ACCEL_DEADZONE_MIN) pedal = 0U;
    float requested = (float)pedal / 100.0f;
    // float swoc_cap = swoc_max_current(speed_mph);
    float after_rollover = requested; //apply_rollover_limit(requested);
    return after_rollover; //fminf(swoc_cap, after_rollover);
}

// Rate-limits increases in commanded current so re-entering drive after a
// brake/neutral doesn't jump straight to the pedal's current position.
// Decreases pass through immediately, only increases are capped.
static float ramp_current(float target_current) {
    last_sent_current = fminf(target_current, last_sent_current + CURRENT_RAMP_PER_TICK);
    return last_sent_current;
}

static void reset_current_ramp(void) { 
    last_sent_current = 0.0f; 
}

//// state handlers

static void handle_state_init(void) { 
    current_state = FSM[CAR_NOT_READY]; 
}

static void handle_state_disabled(void) { 
    reset_current_ramp(); 
    CAN_Send_Drive_Cmd(0.0f, 0.0f, 0); 
}
static void handle_state_not_ready(void) { 
    reset_current_ramp(); 
    CAN_Send_Drive_Cmd(0.0f, 0.0f, 0); 
}

static void handle_state_regen(void) { 
    reset_current_ramp(); 
    CAN_Send_Drive_Cmd(0.0f, 1.0f, 0); 
}

static void handle_state_neutral(void) { 
    reset_current_ramp(); 
    CAN_Send_Drive_Cmd(0.0f, 0.0f, 0); 
}

static void handle_state_forward(void) {
    handle_drive_state(false);
}

static void handle_state_reverse(void) {
    handle_drive_state(true);
}

static void handle_drive_state(bool reverse) {
    float velocity_setpoint = 0.0f;
    float current_setpoint = 0.0f;

    const float vehicle_velocity = g_data_read->motor_velocity.MC_VehicleVelocity;

    const bool is_wrong_direction =
        (!reverse && vehicle_velocity < -0.5f) ||
        ( reverse && vehicle_velocity >  0.5f);

    if (is_wrong_direction) {
        // If we're moving opposite the requested direction,
        // force zero torque until vehicle slows down
        velocity_setpoint = 0.0f;
        current_setpoint = 0.0f;
        warning_set(WARNING_ID_MOTOR_DIRECTION_CHANGE_LOCKOUT);
    } else {
        if (!reverse) warning_clear(WARNING_ID_REGEN_NOT_ALLOWED);

        velocity_setpoint = reverse ? -MAX_VELOCITY : MAX_VELOCITY;

        float speed_mph = fabsf(vehicle_velocity) * METERS_SEC_TO_MPH;
        current_setpoint = get_drive_current(speed_mph, 
                                             g_data_read->accel_brake.AccelPedal_Main_Pos);
    }

    current_setpoint = ramp_current(current_setpoint);

    CAN_Send_Drive_Cmd(velocity_setpoint, current_setpoint, 0);

    printf("%s Drive cmd: %f vel, %f curr\r\n",
           reverse ? "Reverse" : "Forward",
           velocity_setpoint,
           current_setpoint);
}

static void handle_state_cruise(void) {
    float velocity = rollover_limit_active ? 0.0f : MAX_VELOCITY;
    float speed_mph = fabsf(g_data_read->motor_velocity.MC_VehicleVelocity) * METERS_SEC_TO_MPH;
    float current = get_drive_current(speed_mph, g_data_read->accel_brake.AccelPedal_Main_Pos);
    CAN_Send_Drive_Cmd(velocity, ramp_current(current), 0);
}

// static const swoc_threshold_t SWOC_THRESHOLDS[] = {
//     {10.0f, 0.80f}, {17.0f, 0.75f}, {20.0f, 0.70f},
//     {23.0f, 0.60f}, {25.0f, 0.50f}, {28.5f, 0.45f}
// };
// static const size_t NUM_SWOC_THRESHOLDS = (sizeof(SWOC_THRESHOLDS) / sizeof(SWOC_THRESHOLDS[0]));

// static float swoc_max_current(float speed_mph) {
//     float cap = MAX_CURRENT_PERCENT;
//     for (size_t i = 0; i < NUM_SWOC_THRESHOLDS; ++i) {
//         if (speed_mph >= SWOC_THRESHOLDS[i].speed_mph) {
//             cap = SWOC_THRESHOLDS[i].max_current;
//         }
//     }
//     return cap;
// }


//////// rtos tasks


void Task_FSM(void *args __attribute__((unused))) {
    TickType_t last = xTaskGetTickCount();
    while (1) {
        fsm_step();
        LED_toggle(HB);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(FSM_TASK_DELAY_MS));
    }
}
