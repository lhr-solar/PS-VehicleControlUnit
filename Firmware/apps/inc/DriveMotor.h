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
#include "stm32g4xx.h"
#include "CAN.h"
#include "can_ids.h"
#include "can_parsing_generated.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include <stdint.h>


// Motor Controller current values. Current is in Amps (A)
#define MAX_MOCO_BATTERY_CURRENT 64.0f  // NOTE: Provided only for reference. This 64A max for daybreak, anticipated to be 135 for next-gen
#define CONT_MOCO_BATTERY_CURRENT 30.0f // Continuous 
#define MAX_MOCO_CURRENT 122.0f

// #define ACCCEL_PEDAL_RESET_THRESHOLD 20
#define MAX_VELOCITY 20000
extern EventGroupHandle_t carStatusEventGroup; //bitfield for car status (thread-safe)
extern struct filtered_vcu_status_t vcu_status;

//WARNING: IF YOU CHANGE THIS THE PYTHON SCRIPT MUST CHANGE AS WELL
// ----------------------------------------------------------------
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

// -------------------------------------------------------------------------------
#define NEXT_STATES_LENGTH (1 << BITFIELD_INPUT_COUNT) // 2^number of bits
#define ALL_STATUS_BITS ((1 << BITFIELD_INPUT_COUNT) - 1) // all bits set


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

//watchdog stuffs
void recieved_CAN_message(FSM_Signal_t signal);
void CAN_MSG_Watchdog_Create(const char* timerName,
                            FSM_Signal_t signal,
                            uint32_t timeout_ms);
void handleWatchdogFSMFault();
void car_can_send(uint8_t *data, uint16_t can_id);


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


typedef enum FSMStates {
  STATE_INIT = 0,
  FORWARD_DRIVE = 1,
  NEUTRAL = 2,
  REVERSE_DRIVE = 3,
  REGEN = 4,
  CRUISE_CONTROL = 5,
  DISABLED = 6,
  CAR_NOT_READY = 7,
  NUM_STATES = 8
  // Add more states as needed
} FSMStates;


// OS_FLAG_GRP CarStatus_Flags;  // Bitfield for car status

typedef struct TritiumState {
  FSMStates stateName;
  void (*stateHandler)(void);
  int NextStates[NEXT_STATES_LENGTH];  // Default is neutral
} TritiumState_t;



typedef enum {
    BPS_STATUS,
    VCU_STATUS,
    ACCEL_BRAKE_POS,
    DRIVER_INPUT_STATUS,
    FSM_SIGNAL_COUNT
} FSM_Signal_t;

static const uint16_t fsm_signal_to_can_id[FSM_SIGNAL_COUNT] = {
    [BPS_STATUS]           = FILTERED_BPS_STATUS_FRAME_ID,
    [VCU_STATUS]           = FILTERED_VCU_STATUS_FRAME_ID, 
    [ACCEL_BRAKE_POS]      = FILTERED_ACCEL_BRAKE_POSITION_FRAME_ID,
    [DRIVER_INPUT_STATUS]  = FILTERED_DRIVER_INPUT_STATUS_FRAME_ID
};

typedef enum{
DASH_NEU,
DASH_FWD,
DASH_REV
}gear_t;

typedef enum{
IGN_OFF,
ARR_EN,
MOT_EN
}ignitionState_t;

//For convenience and event groups

#define ALL_CAN_MSGS ((1 << FSM_SIGNAL_COUNT) - 1) //all bits set
// #define WD_WINDOW_DONE (1 << FSM_SIGNAL_COUNT)  // next bit after CAN signals

//IMPORTANT
#define DRIVER_CONTROLS_BASE_ADDR 0x42069


//extra vars for logic in updateStatus
extern ignitionState_t ignitionState;
extern bool isBraking;
extern float accelPedalPercent;

extern SemaphoreHandle_t controls_lock;

#endif

/* @} */
