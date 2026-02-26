/**
 * @file fsm.c
 * @brief FSM implementation for controlling our motor. 
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#include "fsm.h"
// Instantiate the generated table in this translation unit
#define DEFINE_FSM_TABLE
#include "fsm_table.h"

#include "watchdogs.h"
#include "../../Embedded-Sharepoint/bsp/Inc/CAN.h"
#include "faults.h"

#include <string.h>
#include <stdio.h>

#define MAX_VELOCITY    100.0f
#define BRAKE_THRESH    42
#define BRAKE_THRESH_HYST 30


EventGroupHandle_t car_status_event_group = NULL;
MocoState_t current_state;

const uint16_t fsm_signal_to_can_id[FSM_SIGNAL_COUNT] = {
    [FSM_PEDALS]          = CAN_ID_PEDALS,
    [FSM_GEARS]           = CAN_ID_GEARS,
    [FSM_REGEN_BUTTON]    = CAN_ID_REGEN_BUTTON,
    [FSM_REGEN_ENABLED]   = CAN_ID_REGEN_ENABLED,
    [FSM_CRUISE_CONTROL]  = CAN_ID_CRUISE_CONTROL,
    [FSM_BPS_OK_TO_REGEN] = CAN_ID_BPS_OK_TO_REGEN,
    [FSM_BPS_TRIP]        = CAN_ID_BPS_TRIP,
    [FSM_IGNITION_STATE]  = CAN_ID_IGNITION_STATE,
};

/* =========================================================
 * Private state
 * ========================================================= */

static Gear_e           gear                = DASH_NEU;
static bool             regen_btn_pressed   = false;
static bool             cruise_ctrl_btn     = false;
static bool             regen_enabled       = false;
static bool             ok_to_regen         = false;
static bool             bps_tripped         = false;
static float            accel_pedal_percent = 0.0f;
static float            brake_pedal_percent = 0.0f;
static IgnitionState_e ignition_state      = IGN_OFF;
static float            brake_thresh        = BRAKE_THRESH;
static uint8_t          car_status          = 0;

/* =========================================================
 * Forward declarations (static handlers)
 * ========================================================= */

static void handleFSMInitState(void);
static void handleFSMNotReadyState(void);
static void handleFSMForwardDriveState(void);
static void handleFSMNeutralState(void);
static void handleFSMReverseDriveState(void);
static void handleFSMRegenState(void);
static void handleFSMCruiseControlState(void);
static void handleFSMDisabledState(void);

/* =========================================================
 * Public API
 * ========================================================= */

void FSM_Init(void) {
    carStatusEventGroup = xEventGroupCreate();
    configASSERT(carStatusEventGroup != NULL);

    watchdog_init();
    WATCHDOG_INIT_ALL_FSM_SIGNALS();
    CAN_MSG_Watchdog_StartAll();

    can_init(hcan1, NULL);
    can_init(hcan2, NULL);
    can_start(hcan1);
    can_start(hcan2);

    // Patch state handlers into generated table
    // (generator emits NULL; handlers are static to this file)
    FSM[FSM_STATE_INIT].stateHandler      = handleFSMInitState;
    FSM[FORWARD_DRIVE].stateHandler   = handleFSMForwardDriveState;
    FSM[NEUTRAL].stateHandler         = handleFSMNeutralState;
    FSM[REVERSE_DRIVE].stateHandler   = handleFSMReverseDriveState;
    FSM[REGEN].stateHandler           = handleFSMRegenState;
    FSM[CRUISE_CONTROL].stateHandler  = handleFSMCruiseControlState;
    FSM[DISABLED].stateHandler        = handleFSMDisabledState;
    FSM[CAR_NOT_READY].stateHandler   = handleFSMNotReadyState;

    current_state = FSM[STATE_INIT];
    current_state.stateHandler();
}

void FSM_Step(void) {
    current_state = FSM[ current_state.NextStates[car_status] ];
    if (current_state.stateHandler) current_state.stateHandler();
}

void FSM_Disable(void) { current_state = FSM[DISABLED]; }
void FSM_Reset(void) { current_state = FSM[CAR_NOT_READY]; }

void handleWatchdogFSMFault(void) {
    for (int i = 0; i < FSM_SIGNAL_COUNT; i++) {
        if (!Watchdog_IsSignalAlive((FSM_Signal_t)i)) {
            printf("WD timeout: signal %d (CAN 0x%03X)\n",
                   i, fsm_signal_to_can_id[i]);
        }
    }
    Faults_ThrowFault(FAULT_ID_WATCHDOG_FSM);
}

/* =========================================================
 * Tasks
 * ========================================================= */

void Task_UpdateControlStatus(void *p_arg) {
    (void)p_arg;
    while (true) {
        updateFromCAN();
        FSM_RefreshInputs();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void Task_SendMotor(void *p_arg) {
    (void)p_arg;
    while (true) {
        current_state = FSM[ current_state.NextStates[car_status] ];
        if (current_state.stateHandler) current_state.stateHandler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* =========================================================
 * CAN receive / bitfield
 * ========================================================= */

static can_status_t carCANRead(uint8_t *data, FSM_Signal_t signal) {
    CAN_RxHeaderTypeDef header;
    return can_recv(hcan2, fsm_signal_to_can_id[signal], &header, data, 0);
}

static void updateFromCAN(void) {
    for (int i = 0; i < FSM_SIGNAL_COUNT; i++) {
        uint8_t buf[8] = {0};
        can_status_t s = carCANRead(buf, (FSM_Signal_t)i);
        if (s == CAN_EMPTY || s == CAN_ERR) continue;

        watchdog_received_can_message((FSM_Signal_t)i);

        switch (fsm_signal_to_can_id[i]) {
            case CAN_ID_PEDALS:
                brake_pedal_percent = mapToPercent(buf[0], BRAKE_PEDAL_MIN, BRAKE_PEDAL_MAX, 0, 100);
                accel_pedal_percent = mapToPercent(buf[1], ACCEL_PEDAL_MIN, ACCEL_PEDAL_MAX, 0, 100);
                break;
            case CAN_ID_GEARS:         gear               = (gear_t)(buf[0] & 0x03); break;
            case CAN_ID_REGEN_BUTTON:  regen_btn_pressed  = buf[0] & 0x01;          break;
            case CAN_ID_CRUISE_CONTROL:cruise_ctrl_btn = buf[0] & 0x01;          break;
            case CAN_ID_REGEN_ENABLED: regen_enabled        = buf[0] & 0x01;          break;
            case CAN_ID_BPS_OK_TO_REGEN: ok_to_regen         = buf[0] & 0x01;          break;
            case CAN_ID_BPS_TRIP:      bps_tripped           = buf[0] & 0x01;          break;
            case CAN_ID_IGNITION_STATE: ignition_state       = (ignition_state_e)buf[0]; break;
            default: break;
        }
    }
}

static void FSM_RefreshInputs(void) {
    brake_thresh = (brake_pedal_percent >= brake_thresh) ? BRAKE_THRESH_HYST 
                                                         : BRAKE_THRESH;
    
    uint8_t s = 0;
    if (gear == DASH_FWD)       s |= FSM_INPUT_FORWARD_BIT;
    else if (gear == DASH_REV)  s |= FSM_INPUT_REVERSE_BIT;
    else                        s |= FSM_INPUT_NEUTRAL_BIT;

    if (brake_pedal_percent >= brake_thresh) s |= FSM_INPUT_BRAKE_BIT;
    if (regen_btn_pressed)                   s |= FSM_INPUT_REGEN_BUTTON_BIT;
    if (regen_enabled)                       s |= FSM_INPUT_REGEN_ENABLED_BIT;
    if (ok_to_regen)                         s |= FSM_INPUT_READY_TO_REGEN_BIT;
    if (cruise_ctrl_btn)                     s |= FSM_INPUT_CRUISE_CTRL_BTN_BIT;

    car_status = s;
    xEventGroupClearBits(carStatusEventGroup, ALL_STATUS_BITS);
    xEventGroupSetBits(carStatusEventGroup, s);
}

/* =========================================================
 * Motor commands
 * ========================================================= */

static void sendMotorDriveCommand(float velocity, float current) {
    uint8_t data[8] = {0};
    memcpy(&data[0], &velocity, sizeof(float));
    memcpy(&data[4], &current,  sizeof(float));
    motor_can_send(data, MOTOR_DRIVE);
}

/* =========================================================
 * Helpers
 * ========================================================= */

static bool readyToRoll(void) {
    return getCarSpeed() < 1.0f && ignition_state == IGN_MOT_EN;
}

float mapToPercent(uint8_t input,
                   uint8_t in_min, uint8_t in_max,
                   uint8_t out_min, uint8_t out_max) {
    if (in_min >= in_max || input <= in_min) return out_min / 100.0f;
    if (input  >= in_max)                   return out_max / 100.0f;
    uint8_t oi = input  - in_min;
    uint8_t ir = in_max - in_min;
    uint8_t or_ = out_max - out_min;
    return ((oi * or_) / ir + out_min) / 100.0f;
}

/**
 * State handlers
 */

static void handleFSMInitState(void) { 
    current_state = FSM[CAR_NOT_READY]; 
    
}

static void handleFSMNeutralState(void) { 
    sendMotorDriveCommand(0.0f, 0.0f);
}

static void handleFSMForwardDriveState(void) { 
    sendMotorDriveCommand( MAX_VELOCITY, accel_pedal_percent); 
}

static void handleFSMReverseDriveState(void) { 
    sendMotorDriveCommand(-MAX_VELOCITY, accel_pedal_percent); 
}

static void handleFSMRegenState(void) { 
    sendMotorDriveCommand(0.0f, 100.0f);
}

static void handleFSMCruiseControlState(void) { 
    sendMotorDriveCommand(MAX_VELOCITY, accel_pedal_percent);
}

static void handleFSMDisabledState(void) { 
    sendMotorDriveCommand(0.0f, 0.0f); 
}

static void handleFSMNotReadyState(void) {
    sendMotorDriveCommand(0.0f, 0.0f);
    if (readyToRoll()) current_state = FSM[NEUTRAL];
}
