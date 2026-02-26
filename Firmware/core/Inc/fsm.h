/**
 * @file fsm.h
 * @brief Prohelion motor controller FSM types and public API
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 *
 * BitfieldBitIndex_t and FSMStates are parsed by generate_fsm.py.
 * If you change either enum, regenerate fsm_table.h.
 */

#ifndef FSM_H
#define FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "event_groups.h"
#include "can_ids.h"

/* =========================================================
 * Bitfield definition
 * Parsed by generate_fsm.py — do not reorder without regenerating
 * ========================================================= */

// typedef enum BitfieldBitIndex {
//     BIT_IDX_NEUTRAL = 0,
//     BIT_IDX_FORWARD,
//     BIT_IDX_REVERSE,
//     BIT_IDX_CRUISE_CONTROL_BUTTON,
//     BIT_IDX_REGEN_BUTTON,
//     BIT_IDX_READY_TO_REGEN,
//     BIT_IDX_REGEN_ENABLED,
//     BIT_IDX_BRAKE,
//     BITFIELD_INPUT_COUNT
// } BitfieldBitIndex_t;

// typedef enum BitfieldInputs {
//     NEUTRAL_BIT               = 1 << BIT_IDX_NEUTRAL,
//     FORWARD_BIT               = 1 << BIT_IDX_FORWARD,
//     REVERSE_BIT               = 1 << BIT_IDX_REVERSE,
//     CRUISE_CONTROL_BUTTON_BIT = 1 << BIT_IDX_CRUISE_CONTROL_BUTTON,
//     REGEN_BUTTON_BIT          = 1 << BIT_IDX_REGEN_BUTTON,
//     READY_TO_REGEN_BIT        = 1 << BIT_IDX_READY_TO_REGEN,
//     REGEN_ENABLED_BIT         = 1 << BIT_IDX_REGEN_ENABLED,
    
// } BitfieldInputs_t;

// typedef enum BitfieldInputs {
//     FSM_INPUT_NEUTRAL_BIT               = 1 << 0,
//     FSM_INPUT_FORWARD_BIT               = 1 << 1,
//     FSM_INPUT_REVERSE_BIT               = 1 << 2,
//     FSM_INPUT_CRUISE_CTRL_BTN_BIT       = 1 << 3,
//     FSM_INPUT_REGEN_BUTTON_BIT          = 1 << 4,
//     FSM_INPUT_READY_TO_REGEN_BIT        = 1 << 5,
//     FSM_INPUT_REGEN_ENABLED_BIT         = 1 << 6,
//     FSM_INPUT_BRAKE_BIT                 = 1 << 7,
//     NUM_FSM_INPUT                       = 
// } BitfieldInputs_t;

// List of all FSM inputs 
#define FSM_INPUT_LIST \
    X(FSM_INPUT_NEUTRAL) \
    X(FSM_INPUT_FORWARD) \
    X(FSM_INPUT_REVERSE) \
    X(FSM_INPUT_CRUISE_CTRL_BTN) \
    X(FSM_INPUT_REGEN_BUTTON) \
    X(FSM_INPUT_READY_TO_REGEN) \
    X(FSM_INPUT_REGEN_ENABLED) \
    X(FSM_INPUT_BRAKE)

// indexes of all the FSM inputs
typedef enum {
#define X(name) name,
    FSM_INPUT_LIST
#undef X
    NUM_FSM_INPUT
} FSMInput_e;

// Bitmasks for each FSM input
typedef enum {
#define X(name) name##_BIT = (1U << name),
    FSM_INPUT_LIST
#undef X
} FSMInputBit_e;


#define NEXT_STATES_LENGTH  (1 << NUM_FSM_INPUT)
#define ALL_STATUS_BITS     ((1 << NUM_FSM_INPUT) - 1)




// FSM states (parsed by generate_fsm.py so dont reorder without regenerating)
typedef enum {
    FSM_STATE_INIT = 0,
    FSM_STATE_FORWARD_DRIVE,
    FSM_STATE_NEUTRAL,
    FSM_STATE_REVERSE_DRIVE,
    FSM_STATE_REGEN,
    FSM_STATE_CRUISE_CONTROL,
    FSM_STATE_DISABLED,
    FSM_STATE_CAR_NOT_READY,
    NUM_FSM_STATES 
} FSMState_e;


typedef enum { DASH_NEU, DASH_FWD, DASH_REV } Gear_e;
typedef enum { IGN_OFF, IGN_ARR_EN, IGN_MOT_EN } IgnitionState_e;

typedef struct {
    FSMState_e  stateName;
    void     (*stateHandler)(void);
    int        NextStates[NEXT_STATES_LENGTH];
} MocoState_t;



extern MocoState_t     FSM[NUM_STATES];
extern MocoState_t     current_state;
extern EventGroupHandle_t ca;
extern const uint16_t     fsm_signal_to_can_id[FSM_SIGNAL_COUNT];


void FSM_Init(void);
void FSM_Step(void);
void FSM_Disable(void);
void FSM_Reset(void);
void handleWatchdogFSMFault(void);

void Task_UpdateControlStatus(void *p_arg);
void Task_SendMotor(void *p_arg);

float mapToPercent(uint8_t input,
                   uint8_t in_min, uint8_t in_max,
                   uint8_t out_min, uint8_t out_max);

#endif // FSM_H
