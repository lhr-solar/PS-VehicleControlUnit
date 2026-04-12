/**
 * @file FSM.c
 * @brief Motor controller FSM implementation
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#define DEFINE_FSM_TABLE
#include "fsm_table_dnr.h"
#include "rollover_speed_table.h"

#include "FSMTask.h"
#include "UpdateVCUInputsTask.h"
#include "FaultBits.h"
#include "Watchdogs.h"
#include <stdlib.h>
#include <string.h>

#define MAX_VELOCITY      100.0f // meters per second

StaticEventGroup_t fsmInputBuffer = {0};
EventGroupHandle_t fsmInputGroup = {0};

MocoState_t current_state = {0};



static bool rollover_limit_active = false;
static volatile uint16_t fsm_inputs = 0;

// method stubs so linker doesnt shit itself
static void handle_state_init(void);
static void handle_state_not_ready(void);
static void handle_state_forward(void);
static void handle_state_neutral(void);
static void handle_state_reverse(void);
static void handle_state_regen(void);
static void handle_state_cruise(void);
static void handle_state_disabled(void);

// static void update_from_can(void);
// static void rebuild_bitfield(void);
static float apply_rollover_limit(float requested_current);

//must be called Before the FSM task gets called
void FSM_TaskInit(){
    fsmInputGroup = xEventGroupCreateStatic(&fsmInputBuffer);
}

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



// goofy ahh logic, uses lut
static float apply_rollover_limit(float requested_current) {
    int deg = abs((int)g_data_read->lws.LWS_Angle) / 10;
    if (deg > (int)ROLLOVER_TABLE_MAX_DEG) deg = (int)ROLLOVER_TABLE_MAX_DEG;

    uint16_t v_max_cms = rollover_speed_table[deg];
    uint16_t v_now_cms = (uint16_t)(g_data_read->motor_velocity.MC_VehicleVelocity * 100.0f);

    if (v_max_cms != ROLLOVER_TABLE_NO_LIMIT && v_now_cms > v_max_cms) {
        rollover_limit_active = true;
        return 0.0f;
    }
    rollover_limit_active = false;
    return requested_current;
}

float map_to_percent(uint8_t input, uint8_t in_min, uint8_t in_max, uint8_t out_min,
                     uint8_t out_max) {
    if (in_min >= in_max || input <= in_min) return (float)out_min / 100.0f;
    if (input >= in_max) return (float)out_max / 100.0f;
    uint16_t oi = input - in_min;
    uint16_t ir = in_max - in_min;
    uint16_t or_ = out_max - out_min;
    return ((float)(oi * or_) / (float)ir + (float)out_min) / 100.0f;
}

//// state handlers

static void handle_state_init(void) { current_state = FSM[CAR_NOT_READY]; }
static void handle_state_neutral(void) { MotorCAN_Send_Drive_Cmd(0.0f, 0.0f, 0); }
static void handle_state_disabled(void) { MotorCAN_Send_Drive_Cmd(0.0f, 0.0f, 0); }
static void handle_state_regen(void) { MotorCAN_Send_Drive_Cmd(0.0f, 1.0f, 0); }
static void handle_state_not_ready(void) { MotorCAN_Send_Drive_Cmd(0.0f, 0.0f, 0); }

static void handle_state_forward(void) {
    if (g_data_read->motor_velocity.MC_VehicleVelocity < -0.5f) {
        // if we're actually going backwards, let off the pedal until we slow down
        MotorCAN_Send_Drive_Cmd(0.0f, 0.0f, 0);
    } else {
        MotorCAN_Send_Drive_Cmd(MAX_VELOCITY, apply_rollover_limit(g_data_read->accel_brake.AccelPedal_Main_Pos), 0);
    }
}

static void handle_state_reverse(void) {
    if (g_data_read->motor_velocity.MC_VehicleVelocity > 0.5f) {
        // if we're actually going forwards, let off the pedal until we slow down
        MotorCAN_Send_Drive_Cmd(0.0f, 0.0f, 0);
    } else {
        MotorCAN_Send_Drive_Cmd(-MAX_VELOCITY, apply_rollover_limit(g_data_read->accel_brake.AccelPedal_Main_Pos), 0);
    }
}

static void handle_state_cruise(void) {
    float current = apply_rollover_limit(g_data_read->accel_brake.AccelPedal_Main_Pos);
    float velocity = rollover_limit_active ? 0.0f : MAX_VELOCITY;
    MotorCAN_Send_Drive_Cmd(velocity, current, 0);
}



//////// rtos tasks


void Task_FSM(void *args __attribute__((unused))) {
    TickType_t last = xTaskGetTickCount();
    while (1) {
        fsm_step();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
    }
}
