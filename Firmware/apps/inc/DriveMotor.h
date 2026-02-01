/**
 * @copyright Copyright (c) 2018-2023 UT Longhorn Racing Solar
 * @file SendTritium.h
 * @brief 
 * 
 * @defgroup SendTritium
 * @addtogroup SendTritium
 * @{
 */
#ifndef __SENDTRITIUM_H
#define __SENDTRITIUM_H

// #include "common.h"
// #include "os.h"
// #include "Dashboard.h"
// #include "Tasks.h"
// #include <cstdint>
#include "can_ids.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include <stdint.h>

//#define SENDTRITIUM_PRINT_MES
// #define CANBUS_MOTOR_SAFE_TO_RUN 1

// #define MOTOR_MSG_PERIOD 100 // in ms
// #define FSM_PERIOD 100 // in ms
// #define DEBOUNCE_PERIOD 2 // in units of FSM_PERIOD

// #define MAX_VELOCITY 12000.0f // rpm (unobtainable value)

// // Used to define accel & brake (hysteresis) thresholds for when to start/stop powering the motor, respectively
// #define ACCEL_PEDAL_THRESHOLD 15 // percent
// #define BRAKE_UNPRESSED_THRESHOLD 40 // percent
// #define BRAKE_PRESSED_THRESHOLD 30 // percent

// Motor Controller current values. Current is in Amps (A)
#define MAX_MOCO_BATTERY_CURRENT 64.0f  // NOTE: Provided only for reference. This 64A max for daybreak, anticipated to be 135 for next-gen
#define CONT_MOCO_BATTERY_CURRENT 30.0f // Continuous 
#define MAX_MOCO_CURRENT 122.0f

#define PEDAL_MIN 0        // percent
#define PEDAL_MAX 100      // percent
#define CURRENT_SP_MIN 0   // percent
#define CURRENT_SP_MAX 100 // percent
#define SWOC_CURRENT_SP_MAX 60 // percent

// #define GEAR_FAULT_THRESHOLD 3 // number of times gear fault can occur before it is considered a fault

// #define ACCCEL_PEDAL_RESET_THRESHOLD 20
#define BRAKE_THRESH 42
#define BRAKE_THRESH_HYST 30

extern EventGroupHandle_t carStatusEventGroup; //bitfield for car status (thread-safe)

// /**
//  * Error types
//  * 
//  */
// typedef enum
// {
//     SENDTRITIUM_ERR_NONE,
//     SENDTRITIUM_ERR_GEAR_FAULT,     // Received multiple or no gear inputs (e.g. FOR_SW, REV_SW)
// } SendTritium_error_code_t;

#ifdef SENDTRITIUM_EXPOSE_VARS
// Inputs
extern uint8_t brakePedalPercent;
extern uint8_t accelPedalPercent;
extern gear_t gear;
extern bool isBrakeOn; // Used for updating display & brakelight
#endif

typedef enum BitfieldBitIndex {
    BIT_IDX_NEUTRAL = 0,               // Index for NEUTRAL_BIT
    BIT_IDX_FORWARD,                    // Index for FORWARD_BIT
    BIT_IDX_REVERSE,                    // Index for REVERSE_BIT
//  BIT_IDX_NOT_READY,                  // Index for NOT_READY_BITS
    BIT_IDX_CRUISE_CONTROL_BUTTON,      // Index for CRUISE_CONTROL_BUTTON_BIT
    BIT_IDX_REGEN_BUTTON,               // Index for REGEN_BUTTON_BIT
    BIT_IDX_READY_TO_REGEN,             // Index for READY_TO_REGEN_BIT
    BIT_IDX_REGEN_ENABLED,              // Index for REGEN_ENABLED_BIT
    BIT_IDX_BRAKING,                    // Index for BRAKING_BIT
    BITFIELD_INPUT_COUNT                // Total number of bits
} BitfieldBitIndex_t;

// BITFIELD INPUT ENUM
typedef enum BitfieldInputs {
  NEUTRAL_BIT = 1 << BIT_IDX_NEUTRAL,                // If we are trying to go neutral
  FORWARD_BIT = 1 << BIT_IDX_FORWARD,                // If we are trying to go forward
  REVERSE_BIT = 1 << BIT_IDX_REVERSE,                // If we are trying to go reverse
//   NOT_READY_BITS = 1 << BIT_IDX_NOT_READY,             // For when the car is starting up
  CRUISE_CONTROL_BUTTON_BIT = 1 << BIT_IDX_CRUISE_CONTROL_BUTTON,  // If the cruise control button is pressed
  REGEN_BUTTON_BIT = 1 << BIT_IDX_REGEN_BUTTON,           // If the regen button is pressed
  READY_TO_REGEN_BIT = 1 << BIT_IDX_READY_TO_REGEN,         // If we are going slow enough to regen
  REGEN_ENABLED_BIT = 1 << BIT_IDX_REGEN_ENABLED,          // If regen is enabled
  BRAKING_BIT = 1 << BIT_IDX_BRAKING,                // If the brake is pressed
  // FAULTED_BIT           = 0x80  // If the car is faulted
} BitfieldInputs_t;

#define NEXT_STATES_LENGTH (1 << BITFIELD_INPUT_COUNT) // 2^number of bits
#define ALL_STATUS_BITS ((1 << BITFIELD_INPUT_COUNT) - 1) // all bits set

// // Getter functions for local variables in SendTritium.c
// EXPOSE_GETTER(uint8_t, brakePedalPercent)
// EXPOSE_GETTER(uint8_t, accelPedalPercent)
// EXPOSE_GETTER(gear_t, gear)
// EXPOSE_GETTER(float, currentSetpoint)
// EXPOSE_GETTER(float, velocitySetpoint)
// EXPOSE_GETTER(bool, isBrakeOn)

/**
 * @brief Linearly map range of integers to another range of integers, and provide the pecentage result.
 * in_min to in_max is mapped to out_min to out_max.
 * @param input input integer value
 * @param in_min minimum value of input range
 * @param in_max maximum value of input range
 * @param out_min minimum value of output range
 * @param out_max maximum value of output range
 * @returns float value from (out_min / 100.0) to (out_max / 100.0)
 */
float mapToPercent(uint8_t input, uint8_t in_min, uint8_t in_max, uint8_t out_min, uint8_t out_max);

// Function prototypes
// static void assertSendTritiumError(controls_error_e sterr);

void Task_UpdateControlStatus(void *p_arg);
void Task_SendMotor(void *p_arg);
void disableFSM();
void recoverFSM();

//Fault handling
void handleWatchdogFSMFault();

//CAN_DATA

/**
 * Standard CAN packet
 * @param can_id 	CANId_t value indicating which message we are trying to send
 * @param idx 	If message is part of a sequence of messages (for messages longer than 64 bits), this indicates the index of the message. 
 * 				This is not designed to exceed the 8bit unsigned max value.
 * @param data 	data of the message
*/
typedef struct {
	uint16_t can_id; 		
	uint8_t idx; 		
	uint8_t data[8]; 
} CANDATA_t;



typedef enum {
  FSM_PEDALS,
  FSM_GEARS,
  FSM_REGEN_BUTTON,
  FSM_REGEN_ENABLED,
  FSM_CRUISE_CONTROL,
  FSM_BPS_OK_TO_REGEN,
  FSM_BPS_TRIP,
  FSM_IGNITION_STATE,
  FSM_SIGNAL_COUNT
} FSM_Signal_t;

static const uint16_t fsm_signal_to_can_id[FSM_SIGNAL_COUNT] = {
  [FSM_PEDALS]            = CAN_ID_PEDALS,
  [FSM_GEARS]             = CAN_ID_GEARS,
  [FSM_REGEN_BUTTON]      = CAN_ID_REGEN_BUTTON,
  [FSM_REGEN_ENABLED]     = CAN_ID_REGEN_ENABLED,
  [FSM_CRUISE_CONTROL]    = CAN_ID_CRUISE_CONTROL,
  [FSM_BPS_OK_TO_REGEN]   = CAN_ID_BPS_OK_TO_REGEN,
  [FSM_BPS_TRIP]          = CAN_ID_BPS_TRIP,
  [FSM_IGNITION_STATE]    = CAN_ID_IGNITION_STATE,
};

typedef enum{
DASH_INIT,
DASH_NEU,
DASH_FWD,
DASH_REV
}gear_t;

typedef enum{
IGN_OFF,
LV_EN,
ARR_EN,
MOT_EN
}ignitionState_t;

//For convenience and event groups

#define ALL_CAN_MSGS ((1 << FSM_SIGNAL_COUNT) - 1) //all bits set
#define WD_WINDOW_DONE (1 << FSM_SIGNAL_COUNT)  // next bit after CAN signals


#endif

/* @} */
