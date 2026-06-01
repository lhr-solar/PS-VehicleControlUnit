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
#include "StatusLEDs.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_VELOCITY        12000 
#define MAX_CURRENT_PERCENT 1.0f

#define ACCEL_DEADZONE_MIN  5u // Minimum 5% pedal pressed to register accel input, prevents ghost inputs :P
#define METERS_SEC_TO_MPH   2.23694f

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

static void handle_drive_state(bool);

static float apply_rollover_limit(float requested_current);
static float swoc_max_current(float speed_mph);
static float get_drive_current(float speed_mph, uint8_t accel_percent_0_100);

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
    // printf("FSM step: %X inputs,  %d curr_state\n\r", fsm_inputs, current_state.stateName);
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
static float apply_rollover_limit(float requested_current) {
    // printf("Applying rollover limit, requested current: %.2f, vehicle velocity: %.2f, steering angle: %.1f\r\n",
        //    requested_current, g_data_read->motor_velocity.MC_VehicleVelocity, (float)g_data_read->lws.LWS_Angle / 10.0f);
    int deg = abs((int)g_data_read->lws.LWS_Angle) / 10;
    if (deg > (int)ROLLOVER_TABLE_MAX_DEG) deg = (int)ROLLOVER_TABLE_MAX_DEG;

    uint16_t v_max_cms = rollover_speed_table[deg];
    uint16_t v_now_cms = (uint16_t)(g_data_read->motor_velocity.MC_VehicleVelocity * 100.0f);

    if (v_max_cms != ROLLOVER_TABLE_NO_LIMIT && v_now_cms > v_max_cms) {
        rollover_limit_active = true;
        warning_set(WARNING_ID_TIPPING_LIMIT_ACTIVE);
        return 0.0f;
    }
    
    warning_clear(WARNING_ID_REGEN_NOT_ALLOWED);
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

static float get_drive_current(float speed_mph, uint8_t accel_percent_0_100) {
    uint8_t pedal = accel_percent_0_100;
    if (pedal <= ACCEL_DEADZONE_MIN) pedal = 0U;
    float requested = (float)pedal / 100.0f;
    float swoc_cap = swoc_max_current(speed_mph);
    float after_rollover = apply_rollover_limit(requested);
    return fminf(swoc_cap, after_rollover);
}

//// state handlers

static void handle_state_init(void) { current_state = FSM[CAR_NOT_READY]; }
static void handle_state_neutral(void) { CAN_Send_Drive_Cmd(0.0f, 0.0f, 0); }
static void handle_state_disabled(void) { CAN_Send_Drive_Cmd(0.0f, 0.0f, 0); }
static void handle_state_regen(void) { CAN_Send_Drive_Cmd(0.0f, 1.0f, 0); }
static void handle_state_not_ready(void) { CAN_Send_Drive_Cmd(0.0f, 0.0f, 0); }

// static void handle_state_forward(void) {
//     float velocitySetpoint = 0.0f;
//     float currentSetpoint = 0.0f;
//     if (g_data_read->motor_velocity.MC_VehicleVelocity < -0.5f) {
//         // if we're actually going backwards, let off the pedal until we slow down
//         velocitySetpoint = 0.0f;
//         currentSetpoint = 0.0f;
//         warning_set(WARNING_ID_MOTOR_DIRECTION_CHANGE_LOCKOUT);
//     } else {
//         warning_clear(WARNING_ID_REGEN_NOT_ALLOWED);
//         velocitySetpoint = MAX_VELOCITY;
//         currentSetpoint = fmin(apply_swoc_speed_limit(g_data_read->motor_velocity.MC_VehicleVelocity * METERS_SEC_TO_MPH), 
//                                 apply_rollover_limit(((float)((g_data_read->accel_brake.AccelPedal_Main_Pos > ACCEL_DEADZONE_MIN) ? g_data_read->accel_brake.AccelPedal_Main_Pos : 0))/100.0f));
//     }

//     printf("Forwards Drive cmd: %f vel, %f curr\r\n", velocitySetpoint, currentSetpoint);

//     CAN_Send_Drive_Cmd(velocitySetpoint, currentSetpoint, 0);

// }

// static void handle_state_reverse(void) {

//     float velocitySetpoint = 0.0f;
//     float currentSetpoint = 0.0f;

//     if (g_data_read->motor_velocity.MC_VehicleVelocity > 0.5f) {
//         // if we're actually going forwards, let off the pedal until we slow down
//         velocitySetpoint = 0.0f;
//         currentSetpoint = 0.0f;
//         warning_set(WARNING_ID_MOTOR_DIRECTION_CHANGE_LOCKOUT);
//     } else {
//         velocitySetpoint = -MAX_VELOCITY;
//         currentSetpoint = fmin(apply_swoc_speed_limit(g_data_read->motor_velocity.MC_VehicleVelocity * METERS_SEC_TO_MPH), ((float)apply_rollover_limit(((g_data_read->accel_brake.AccelPedal_Main_Pos > ACCEL_DEADZONE_MIN) ? g_data_read->accel_brake.AccelPedal_Main_Pos : 0)))/100.0f);
//     }

//     CAN_Send_Drive_Cmd(velocitySetpoint, currentSetpoint, 0);

//     printf("Backwards Drive cmd: %f vel, %f curr", velocitySetpoint, currentSetpoint);

// }

static void handle_state_forward(void) {
    handle_drive_state(false);
}

static void handle_state_reverse(void) {
    handle_drive_state(true);
}

static void handle_drive_state(bool reverse) {

    float velocitySetpoint = 0.0f;
    float currentSetpoint = 0.0f;

    const float vehicleVelocity =
        g_data_read->motor_velocity.MC_VehicleVelocity;

    const bool wrongDirection =
        (!reverse && vehicleVelocity < -0.5f) ||
        ( reverse && vehicleVelocity >  0.5f);

    if (wrongDirection) {

        // If we're moving opposite the requested direction,
        // force zero torque until vehicle slows down
        velocitySetpoint = 0.0f;
        currentSetpoint = 0.0f;

        warning_set(WARNING_ID_MOTOR_DIRECTION_CHANGE_LOCKOUT);

    } else {

        if (!reverse) warning_clear(WARNING_ID_REGEN_NOT_ALLOWED);

        velocitySetpoint = reverse ? -MAX_VELOCITY : MAX_VELOCITY;

        float speed_mph = fabsf(vehicleVelocity) * METERS_SEC_TO_MPH;
        currentSetpoint = get_drive_current(speed_mph, g_data_read->accel_brake.AccelPedal_Main_Pos);
    }

    CAN_Send_Drive_Cmd(velocitySetpoint, currentSetpoint, 0);

    printf("%s Drive cmd: %f vel, %f curr\r\n",
           reverse ? "Reverse" : "Forward",
           velocitySetpoint,
           currentSetpoint);
}

static void handle_state_cruise(void) {
    float velocity = rollover_limit_active ? 0.0f : MAX_VELOCITY;
    float speed_mph = fabsf(g_data_read->motor_velocity.MC_VehicleVelocity) * METERS_SEC_TO_MPH;
    CAN_Send_Drive_Cmd(
        velocity,
        get_drive_current(speed_mph, g_data_read->accel_brake.AccelPedal_Main_Pos),
        0);
}

static const swoc_threshold_t SWOC_THRESHOLDS[] = {
    {10.0f, 0.80f}, {17.0f, 0.75f}, {20.0f, 0.70f},
    {23.0f, 0.60f}, {25.0f, 0.50f}, {28.5f, 0.45f}
};
static const size_t NUM_SWOC_THRESHOLDS = (sizeof(SWOC_THRESHOLDS) / sizeof(SWOC_THRESHOLDS[0]));

static float swoc_max_current(float speed_mph) {
    float cap = MAX_CURRENT_PERCENT;
    for (size_t i = 0; i < NUM_SWOC_THRESHOLDS; ++i) {
        if (speed_mph >= SWOC_THRESHOLDS[i].speed_mph) {
            cap = SWOC_THRESHOLDS[i].max_current;
        }
    }
    return cap;
}


//////// rtos tasks


void Task_FSM(void *args __attribute__((unused))) {
    // TickType_t last = xTaskGetTickCount();
    while (1) {
        fsm_step();
        // CAN_Send_Drive_Cmd(0.0f, 0.0f, 100);
        vTaskDelay(pdMS_TO_TICKS(90));
        // vTaskDelayUntil(&last, pdMS_TO_TICKS(10));
    }
}
