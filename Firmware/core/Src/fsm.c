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
#include "CAN.h"
#include "faults.h"
#include "CarCAN_can_msgs.h"
#include "MotorCAN_can_msgs.h"
#include <string.h>
#include <stdlib.h>

#define MAX_VELOCITY        100.0f
#define BRAKE_THRESH        42.0f
#define BRAKE_THRESH_HYST   30.0f


MocoState_t currentState;

static driver_input_status_t  driver_input    = {0};
static accel_brake_position_t accel_brake     = {0};
static lws_standard_t         lws             = {0};
static controls_status_t      controls_status = {0};

// Only the BPS fields consumed by the FSM
static struct { uint8_t BPS_Fault; uint8_t BPS_Regen_OK; } bps_status = {0};
// Only the MC field consumed by the FSM
static struct { float MC_VehicleVelocity; } motor_velocity = {0};

static float  accel_pedal_pct     = 0.0f;
static float  brake_pedal_pct     = 0.0f;
static float  brake_threshold     = BRAKE_THRESH;
static bool   precharge_complete  = false;
static bool   rollover_limit_active = false;
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

static void  send_motor_drive_cmd(float velocity, float current);
static void  update_from_can(void);
static void  rebuild_bitfield(void);
static bool  ready_to_roll(void);
static float apply_rollover_limit(float requested_current);


///// can msg unpackers


// Driver_Input_Status  0x60  2 bytes  Intel bit-order
static void unpack_driver_input(const uint8_t *b, driver_input_status_t *o) {
    o->Ignition_Array      = (b[0] >> 0) & 1;
    o->Ignition_Motor      = (b[0] >> 1) & 1;
    o->Ignition_Off        = (b[0] >> 2) & 1;
    o->Cruise_Enable       = (b[0] >> 3) & 1;
    o->Cruise_Set          = (b[0] >> 4) & 1;
    o->Gear_Forward        = (b[0] >> 5) & 1;
    o->Gear_Neutral        = (b[0] >> 6) & 1;
    o->Gear_Reverse        = (b[0] >> 7) & 1;
    o->Hazard_Pressed      = (b[1] >> 0) & 1;
    o->Horn_Pressed        = (b[1] >> 1) & 1;
    o->Blinker_Left        = (b[1] >> 2) & 1;
    o->Blinker_Right       = (b[1] >> 3) & 1;
    o->PushToTalk_Pressed  = (b[1] >> 4) & 1;
    o->Regen_Activate      = (b[1] >> 5) & 1;
    o->Regen_Enable        = (b[1] >> 6) & 1;
}

// Accel_Brake_Position  0x50  5 bytes
static void unpack_accel_brake(const uint8_t *b, accel_brake_position_t *o) {
    o->Accel_Pos_Main            = b[0];
    o->Accel_Pos_Redundant       = b[1];
    o->Brake_Pos_Main            = b[2];
    o->Brake_Pos_Redundant       = b[3];
    o->Accel_Pos_Main_Fault      = (b[4] >> 0) & 1;
    o->Accel_Pos_Redundant_Fault = (b[4] >> 1) & 1;
    o->Brake_Pos_Main_Fault      = (b[4] >> 2) & 1;
    o->Brake_Pos_Redundant_Fault = (b[4] >> 3) & 1;
    o->Brake_Pressure_1_Fault    = (b[4] >> 4) & 1;
    o->Brake_Pressure_2_Fault    = (b[4] >> 5) & 1;
}

// LWS_Standard  0x2B0  5 bytes  LWS_Angle is signed 16-bit Intel
static void unpack_lws(const uint8_t *b, lws_standard_t *o) {
    o->LWS_Angle = (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
    o->LWS_Speed = b[2];
    o->LWS_OK    = (b[3] >> 0) & 1;
    o->LWS_CAL   = (b[3] >> 1) & 1;
    o->LWS_TRIM  = (b[3] >> 2) & 1;
}

// Controls_Status  0x15  4 bytes
static void unpack_controls_status(const uint8_t *b, controls_status_t *o) {
    o->Controls_Leader_Fault   = b[0];
    o->Controls_Lighting_Fault = (uint16_t)(b[2] | ((uint16_t)b[3] << 8));
}

// BPS_Status  0x01  7 bytes  — only fields the FSM needs
static void unpack_bps_status(const uint8_t *b) {
    bps_status.BPS_Fault    = b[0];          // bits  0-7
    bps_status.BPS_Regen_OK = (b[1] >> 1) & 1;  // bit 9
}

// MC_VelocityMeasurement — Tritium WS22: vehicle velocity float at bytes 4-7
static void unpack_motor_velocity(const uint8_t *b) {
    memcpy(&motor_velocity.MC_VehicleVelocity, &b[4], sizeof(float));
}


///// can helpers


static void carcan_send(uint16_t id, const uint8_t *data, uint8_t len) {
    CAN_TxHeaderTypeDef hdr = {
        .StdId = id, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = len
    };
    can_send(hcan2, &hdr, data, 0);
}

static void motorcan_send(uint16_t id, const uint8_t *data, uint8_t len) {
    CAN_TxHeaderTypeDef hdr = {
        .StdId = id, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = len
    };
    can_send(hcan1, &hdr, data, 0);
}



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

/* =========================================================
 * CAN receive  (hcan2 = CarCAN, hcan1 = MotorCAN)
 *
 * ISR already routed each frame into its per-ID queue.
 * delay_ticks=0 → non-blocking peek; CAN_RECV means got data.
 * ========================================================= */

static void update_from_can(void) {
    CAN_RxHeaderTypeDef hdr;
    uint8_t buf[8] = {0};

    if (can_recv(hcan2, CAN_ID_DRIVER_INPUT_STATUS, &hdr, buf, 0) == CAN_RECV) {
        unpack_driver_input(buf, &driver_input);
        watchdog_received_can_message(WD_IDX_DRIVER_INPUT);
    }

    if (can_recv(hcan2, CAN_ID_ACCEL_BRAKE_POSITION, &hdr, buf, 0) == CAN_RECV) {
        unpack_accel_brake(buf, &accel_brake);
        watchdog_received_can_message(WD_IDX_ACCEL_BRAKE);
    }

    if (can_recv(hcan2, CAN_ID_LWS_STANDARD, &hdr, buf, 0) == CAN_RECV) {
        unpack_lws(buf, &lws);
        watchdog_received_can_message(WD_IDX_STEERING_ANGLE);
        if (!lws.LWS_OK) Faults_ThrowFault(FAULT_ID_STEERING_SENSOR_BAD_DATA);
    }

    if (can_recv(hcan2, CAN_ID_CONTROLS_STATUS, &hdr, buf, 0) == CAN_RECV) {
        unpack_controls_status(buf, &controls_status);
        watchdog_received_can_message(WD_IDX_CONTROLS_STATUS);
        if (controls_status.Controls_Leader_Fault) Faults_ThrowFault(FAULT_ID_CONTROLS_FAULT);
    }

    if (can_recv(hcan2, CAN_ID_BPS_STATUS, &hdr, buf, 0) == CAN_RECV) {
        unpack_bps_status(buf);
        watchdog_received_can_message(WD_IDX_BPS_STATUS);
        if (bps_status.BPS_Fault) Faults_ThrowFault(FAULT_ID_BPS_FAULT);
    }

    if (can_recv(hcan1, CAN_ID_MC_VELOCITYMEASUREMENT, &hdr, buf, 0) == CAN_RECV) {
        unpack_motor_velocity(buf);
    }
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

    accel_pedal_pct = map_to_percent(accel_brake.Accel_Pos_Main,
                                     ACCEL_PEDAL_MIN, ACCEL_PEDAL_MAX, 0, 100);
    brake_pedal_pct = map_to_percent(accel_brake.Brake_Pos_Main,
                                     BRAKE_PEDAL_MIN, BRAKE_PEDAL_MAX, 0, 100);

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


 // bytes 0-3: velocity (float, m/s), bytes 4-7: current (float, 0-1)
static void send_motor_drive_cmd(float velocity, float current) {
    uint8_t data[8] = {0};
    memcpy(&data[0], &velocity, sizeof(float));
    memcpy(&data[4], &current,  sizeof(float));
    motorcan_send(MOTOR_DRIVE, data, 8);
}

static bool ready_to_roll(void) {
    return  driver_input.Ignition_Motor
        && !driver_input.Ignition_Off
        &&  motor_velocity.MC_VehicleVelocity < 1.0f;
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
    if (ready_to_roll()) currentState = FSM[NEUTRAL_DRIVE];
}

static void handle_state_forward(void) {
    send_motor_drive_cmd(MAX_VELOCITY, apply_rollover_limit(accel_pedal_pct));
}

static void handle_state_reverse(void) {
    send_motor_drive_cmd(-MAX_VELOCITY, apply_rollover_limit(accel_pedal_pct));
}

static void handle_state_cruise(void) {
    float current  = apply_rollover_limit(accel_pedal_pct);
    float velocity = rollover_limit_active ? 0.0f : MAX_VELOCITY;
    send_motor_drive_cmd(velocity, current);
}

//////// rtos tasks

void Task_UpdateControlStatus() {
    TickType_t last = xTaskGetTickCount();
    while (1) {
        update_from_can();
        rebuild_bitfield();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(50));
    }
}

void Task_FSM() {
    TickType_t last = xTaskGetTickCount();
    while (1) {
        fsm_step();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
    }
}

// VCU_Status  0x10  3 bytes  100ms
void Task_BroadcastVCUStatus() {
    uint8_t buf[3];

    while (1) {
        // Byte 0: VCU_Fault — map internal faults to DBC enum
        uint8_t vcu_fault = VCU_STATUS_VCU_FAULT_NO_FAULT;
        if (Faults_IsActive(FAULT_ID_PRECHARGE_TIMEOUT))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_PRECHARGE_TIMEOUT;
        else if (Faults_IsActive(FAULT_ID_MOTOR_DC_BUS_OVERVOLTAGE))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_HV_OVERVOLTAGE;
        else if (Faults_IsActive(FAULT_ID_MOTOR_15V_UNDERVOLTAGE))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_HV_UNDERVOLTAGE;
        else if (Faults_IsActive(FAULT_ID_MOTOR_HARDWARE_OVERCURRENT) ||
                 Faults_IsActive(FAULT_ID_MOTOR_SOFTWARE_OVERCURRENT) ||
                 Faults_IsActive(FAULT_ID_MOTOR_BAD_HALL_SEQUENCE)    ||
                 Faults_IsActive(FAULT_ID_MOTOR_WD_RESET)             ||
                 Faults_IsActive(FAULT_ID_MOTOR_CONFIG_READ)          ||
                 Faults_IsActive(FAULT_ID_MOTOR_DESATURATION)         ||
                 Faults_IsActive(FAULT_ID_MOTOR_OVERSPEED))
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
            !Faults_IsActive(FAULT_ID_CONTROLS_STATUS_WATCHDOG) &&
            !Faults_IsActive(FAULT_ID_CONTROLS_FAULT);

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
