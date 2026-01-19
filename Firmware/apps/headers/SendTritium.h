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
// #include <cstdint>

//#define SENDTRITIUM_PRINT_MES
#define CANBUS_MOTOR_SAFE_TO_RUN 1

#define MOTOR_MSG_PERIOD 100 // in ms
#define FSM_PERIOD 100 // in ms
#define DEBOUNCE_PERIOD 2 // in units of FSM_PERIOD

#define MAX_VELOCITY 12000.0f // rpm (unobtainable value)

// Used to define accel & brake (hysteresis) thresholds for when to start/stop powering the motor, respectively
#define ACCEL_PEDAL_THRESHOLD 15 // percent
#define BRAKE_UNPRESSED_THRESHOLD 40 // percent
#define BRAKE_PRESSED_THRESHOLD 30 // percent

// Motor Controller current values. Current is in Amps (A)
#define MAX_MOCO_BATTERY_CURRENT 64.0f  // NOTE: Provided only for reference. This 64A max for daybreak, anticipated to be 135 for next-gen
#define CONT_MOCO_BATTERY_CURRENT 30.0f // Continuous 
#define MAX_MOCO_CURRENT 122.0f

#define PEDAL_MIN 0        // percent
#define PEDAL_MAX 100      // percent
#define CURRENT_SP_MIN 0   // percent
#define CURRENT_SP_MAX 100 // percent
#define SWOC_CURRENT_SP_MAX 60 // percent

#define GEAR_FAULT_THRESHOLD 3 // number of times gear fault can occur before it is considered a fault

#define ACCCEL_PEDAL_RESET_THRESHOLD 20

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

// Getter functions for local variables in SendTritium.c
EXPOSE_GETTER(uint8_t, brakePedalPercent)
EXPOSE_GETTER(uint8_t, accelPedalPercent)
EXPOSE_GETTER(gear_t, gear)
EXPOSE_GETTER(float, currentSetpoint)
EXPOSE_GETTER(float, velocitySetpoint)
EXPOSE_GETTER(bool, isBrakeOn)

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
static void assertSendTritiumError(controls_error_e sterr);

void Task_SendTritium(void *p_arg);
void disableFSM();




// //Making a source of truth (ts)

// typedef struct {
//     controls_error_e       error_code;
//     bool                   is_evac_needed;
//     callback_t             error_callback;
//     error_scheduler_opt_e  lock_scheduler;
//     error_recovery_opt_e   recovery;
//     BPSFaultErr_e          bps_err;
// } TaskErrorParams;

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

//For convenience and event groups

#define ALL_CAN_MSGS ((1 << FSM_SIGNAL_COUNT) - 1) //all bits set
#define WD_WINDOW_DONE (1 << FSM_SIGNAL_COUNT)  // next bit after CAN signals



// // CAN IDS I care about
// typedef enum FSM_CANIDs {
//   PEDALS = 0x67,  // Example ID, need to change later
//   GEARS = 0x67,
//   REGEN_BUTTON = 0x67,
//   REGEN_ENABLED_BUTTON = 0x67,
//   CRUISE_CONTROL_BUTTON = 0x67,
//   BPS_OK_TO_REGEN = 0x67,
//   BPS_TRIP = 0x67,
//   IGNITION_STATE = 0x67
//   // Add more as needed
// } FSM_CANIDs_t;

// #define NUM_FSM_CANFilters 6

// // Lookup but i need to iterate through them, could make it a LTable but eh
// uint16_t FSM_CANFilterList[NUM_FSM_CANFilters] = {
//     PEDALS,   GEARS,         REGEN_BUTTON, CRUISE_CONTROL_BUTTON,
//     BPS_TRIP, IGNITION_STATE};


#endif

/* @} */
