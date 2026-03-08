/**
 * @file fsm.c
 * @brief Motor controller FSM implementation
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#define DEFINE_FSM_TABLE
#include "fsm_table.h"

#include "fsm.h"
#include "rollover_speed_table.h"
#include "watchdogs.h"
#include "faults.h"
#include "can_utils.h"
#include <string.h>
#include <stdlib.h>

#define MAX_VELOCITY        100.0f // meters per second
#define BRAKE_THRESH        42.0f  // percent
#define BRAKE_THRESH_HYST   30.0f  // percent


MocoState_t currentState;

static driver_input_status_t    driver_input    = {0};
static accel_brake_position_t   accel_brake     = {0};
static lws_standard_t           lws             = {0};
static controls_status_t        controls_status = {0};
static mc_status_t              motor_status    = {0};
static bps_status_t             bps_status      = {0};
static mc_velocitymeasurement_t motor_velocity  = {0};

static float  accel_pedal_pct     = 0.0f;
static float  brake_pedal_pct     = 0.0f;
static float  brake_threshold     = BRAKE_THRESH;
static bool   precharge_complete  = false;
static bool   rollover_limit_active = false;
static bool   ready_to_roll       = false;
static volatile uint16_t car_status = 0;

// method stubs so linker doesnt shit itself
static void handle_state_init(void);
static void handle_state_not_ready(void);
static void handle_state_forward(void);
static void handle_state_neutral(void);
static void handle_state_reverse(void);
static void handle_state_regen(void);
static void handle_state_cruise(void);
static void handle_state_disabled(void);

static void  update_from_can(void);
static void  rebuild_bitfield(void);
static float apply_rollover_limit(float requested_current);



void fsm_init(void) {
    FSM[STATE_INIT].stateHandler     = handle_state_init;
    FSM[FORWARD_DRIVE].stateHandler  = handle_state_forward;
    FSM[NEUTRAL_DRIVE].stateHandler  = handle_state_neutral;
    FSM[REVERSE_DRIVE].stateHandler  = handle_state_reverse;
    FSM[REGEN].stateHandler          = handle_state_regen;
    FSM[CRUISE_CONTROL].stateHandler = handle_state_cruise;
    FSM[DISABLED].stateHandler       = handle_state_disabled;
    FSM[CAR_NOT_READY].stateHandler  = handle_state_not_ready;

    currentState = FSM[STATE_INIT];
    currentState.stateHandler();
}

void fsm_step(void) {
    currentState = FSM[currentState.NextStates[car_status]];
    if (currentState.stateHandler) currentState.stateHandler();
}

void     fsm_disable(void)                    { currentState = FSM[DISABLED];      }
void     fsm_recover(void)                    { currentState = FSM[CAR_NOT_READY]; }
void     fsm_set_precharge_complete(bool val) { precharge_complete = val;           }
uint16_t fsm_get_car_status(void)             { return (uint16_t)car_status;        }
bool     fsm_is_over_rollover_speed(void)     { return rollover_limit_active;       }



static void update_from_can(void) {
    carcan_try_recv(CAN_ID_DRIVER_INPUT_STATUS, handle_driver_input, &driver_input);
    carcan_try_recv(CAN_ID_ACCEL_BRAKE_POSITION, handle_accel_brake, &accel_brake);
    carcan_try_recv(CAN_ID_LWS_STANDARD, handle_lws, &lws);
    carcan_try_recv(CAN_ID_CONTROLS_STATUS, handle_controls_status, &controls_status);
    carcan_try_recv(CAN_ID_BPS_STATUS, handle_bps, &bps_status);
    vcucan_try_recv(CAN_ID_MC_VELOCITYMEASUREMENT, handle_motor_velocity, &motor_velocity);
    vcucan_try_recv(CAN_ID_MC_STATUS, handle_motor_status, &motor_status);
}


static void rebuild_bitfield(void) {
    uint16_t s = 0;

    if      (driver_input.Gear_Forward) s |= FORWARD_BIT;
    else if (driver_input.Gear_Reverse) s |= REVERSE_BIT;
    else                                s |= NEUTRAL_BIT;

    if (driver_input.Regen_Activate) s |= REGEN_BUTTON_BIT;
    if (driver_input.Regen_Enable)   s |= REGEN_ENABLED_BIT;
    if (driver_input.Cruise_Enable)  s |= CRUISE_CONTROL_BUTTON_BIT;
    if (bps_status.BPS_Regen_OK)     s |= READY_TO_REGEN_BIT;
    if (precharge_complete)          s |= PRECHARGE_COMPLETE_BIT;

    accel_pedal_pct = accel_brake.Accel_Pos_Main;
    brake_pedal_pct = accel_brake.Brake_Pos_Main;

    if (brake_pedal_pct >= brake_threshold) {
        s |= BRAKE_BIT;
        brake_threshold = BRAKE_THRESH_HYST;
    } else {
        brake_threshold = BRAKE_THRESH;
    }

    car_status = s;
}


// goofy ahh logic, uses lut
static float apply_rollover_limit(float requested_current) {
    int deg = abs((int)lws.LWS_Angle) / 10;
    if (deg > (int)ROLLOVER_TABLE_MAX_DEG) deg = (int)ROLLOVER_TABLE_MAX_DEG;

    uint16_t v_max_cms = rollover_speed_table[deg];
    uint16_t v_now_cms = (uint16_t)(motor_velocity.MC_VehicleVelocity * 100.0f);

    if (v_max_cms != ROLLOVER_TABLE_NO_LIMIT && v_now_cms > v_max_cms) {
        rollover_limit_active = true;
        return 0.0f;
    }
    rollover_limit_active = false;
    return requested_current;
}

float map_to_percent(uint8_t input,
                     uint8_t in_min, uint8_t in_max,
                     uint8_t out_min, uint8_t out_max) {
    if (in_min >= in_max || input <= in_min) return (float)out_min / 100.0f;
    if (input  >= in_max)                   return (float)out_max / 100.0f;
    uint16_t oi  = input   - in_min;
    uint16_t ir  = in_max  - in_min;
    uint16_t or_ = out_max - out_min;
    return ((float)(oi * or_) / (float)ir + (float)out_min) / 100.0f;
}

//// state handlers

static void handle_state_init(void)     { currentState = FSM[CAR_NOT_READY]; }
static void handle_state_neutral(void)  { send_motor_drive_cmd(0.0f,  0.0f); }
static void handle_state_disabled(void) { send_motor_drive_cmd(0.0f,  0.0f); }
static void handle_state_regen(void)    { send_motor_drive_cmd(0.0f,  1.0f); }

static void handle_state_not_ready(void) {
    send_motor_drive_cmd(0.0f, 0.0f);
}

static void handle_state_forward(void) {
    if (motor_velocity.MC_VehicleVelocity < -0.5f) {
        // if we're actually going backwards, let off the pedal until we slow down
        send_motor_drive_cmd(0.0f, 0.0f);
    } else {
        send_motor_drive_cmd(MAX_VELOCITY, apply_rollover_limit(accel_pedal_pct));
    }
}

static void handle_state_reverse(void) {
    if (motor_velocity.MC_VehicleVelocity > 0.5f) {
        // if we're actually going forwards, let off the pedal until we slow down
        send_motor_drive_cmd(0.0f, 0.0f);
    } else {
        send_motor_drive_cmd(-MAX_VELOCITY, apply_rollover_limit(accel_pedal_pct));
    }
}

static void handle_state_cruise(void) {
    float current  = apply_rollover_limit(accel_pedal_pct);
    float velocity = rollover_limit_active ? 0.0f : MAX_VELOCITY;
    send_motor_drive_cmd(velocity, current);
}

//////// rtos tasks

void Task_UpdateControlStatus(void *args  __attribute__((unused))) {
    TickType_t last = xTaskGetTickCount();
    while (1) {
        update_from_can();
        rebuild_bitfield();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(50));
    }
}

void Task_FSM(void *args  __attribute__((unused))) {
    TickType_t last = xTaskGetTickCount();
    while (1) {
        fsm_step();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
    }
}

// VCU_Status  0x10  3 bytes  100ms
void Task_BroadcastVCUStatus(void *args  __attribute__((unused))) {
    uint8_t buf[3];

    while (1) {
        // Byte 0: VCU_Fault — map internal faults to DBC enum
        uint8_t vcu_fault = VCU_STATUS_VCU_FAULT_NO_FAULT;
        if (faults_is_active(FAULT_ID_PRECHARGE_TIMEOUT))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_PRECHARGE_TIMEOUT;
        else if (faults_is_active(FAULT_ID_MOTOR_DC_BUS_OVERVOLTAGE))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_HV_OVERVOLTAGE;
        else if (faults_is_active(FAULT_ID_MOTOR_15V_UNDERVOLTAGE))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_HV_UNDERVOLTAGE;
        else if (faults_is_active(FAULT_ID_MOTOR_HARDWARE_OVERCURRENT) ||
                 faults_is_active(FAULT_ID_MOTOR_SOFTWARE_OVERCURRENT) ||
                 faults_is_active(FAULT_ID_MOTOR_BAD_HALL_SEQUENCE)    ||
                 faults_is_active(FAULT_ID_MOTOR_WD_RESET)             ||
                 faults_is_active(FAULT_ID_MOTOR_CONFIG_READ)          ||
                 faults_is_active(FAULT_ID_MOTOR_DESATURATION)         ||
                 faults_is_active(FAULT_ID_MOTOR_OVERSPEED))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_CONTROLLER_FAULT;
        buf[0] = vcu_fault;

        // Byte 1: status bits per DBC positions 8-14
        bool pedals_ok =
            !accel_brake.Accel_Pos_Main_Fault      &&
            !accel_brake.Accel_Pos_Redundant_Fault &&
            !accel_brake.Brake_Pos_Main_Fault      &&
            !accel_brake.Brake_Pos_Redundant_Fault &&
            !accel_brake.Brake_Pressure_1_Fault    &&
            !accel_brake.Brake_Pressure_2_Fault;

        bool driver_input_ok =
            !faults_is_active(FAULT_ID_CONTROLS_STATUS_WATCHDOG) &&
            !faults_is_active(FAULT_ID_CONTROLS_FAULT);

        buf[1] =
            ((uint8_t)precharge_complete                       << 0) |  // Motor_Contactor_State
            ((uint8_t)precharge_complete                       << 1) |  // Motor_Precharge_Contactor_State
            ((uint8_t)precharge_complete                       << 2) |  // Motor_Ready_To_Drive
            ((uint8_t)driver_input_ok                          << 3) |  // VCU_Driver_Input_OK
            ((uint8_t)pedals_ok                                << 4) |  // VCU_Pedals_OK
            ((uint8_t)!!(car_status & READY_TO_REGEN_BIT)      << 5) |  // VCU_Regen_OK
            ((uint8_t)(currentState.stateName == REGEN)        << 6);   // VCU_Regen_Active

        // Byte 2: VCU_FSM_State bits [3:0]
        buf[2] = (uint8_t)(currentState.stateName & 0x0FU);

        carcan_send(CAN_ID_VCU_STATUS, buf, sizeof(buf));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
