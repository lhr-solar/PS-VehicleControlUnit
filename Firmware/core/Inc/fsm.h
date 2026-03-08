/**
 * @file fsm.h
 * @brief Tritium motor controller FSM types and public API
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 *
 * WARNING: BITFIELD_INPUT_LIST order must match BITS dict in
 *          scripts/fsm_generator.py. If you add/reorder entries,
 *          update that script and regenerate fsm_table.h. Same 
 *          with FSMState_e.
 */

#pragma once

#include "FreeRTOS.h"
#include <stdint.h>
#include <stdbool.h>

// make sure if you change this to also change it in scripts/fsm_generator.py
#define BITFIELD_INPUT_LIST(X) \
    X(NEUTRAL)               \
    X(FORWARD)               \
    X(REVERSE)               \
    X(CRUISE_CONTROL_BUTTON) \
    X(REGEN_BUTTON)          \
    X(READY_TO_REGEN)        \
    X(REGEN_ENABLED)         \
    X(BRAKE)                 \
    X(PRECHARGE_COMPLETE)

typedef enum {
#define X(name) BIT_IDX_##name,
    BITFIELD_INPUT_LIST(X)
#undef X
    BITFIELD_INPUT_COUNT
} InputBitsIdx_e;

typedef enum {
#define X(name) name##_BIT = (1U << BIT_IDX_##name),
    BITFIELD_INPUT_LIST(X)
#undef X
} InputBits_t;

#define NEXT_STATES_LENGTH  (1U << BITFIELD_INPUT_COUNT)
#define ALL_INPUT_BITS      ((1U << BITFIELD_INPUT_COUNT) - 1U)


// must matche in the generator script
typedef enum {
    STATE_INIT = 0,
    FORWARD_DRIVE,
    NEUTRAL_DRIVE,
    REVERSE_DRIVE,
    REGEN,
    CRUISE_CONTROL,
    DISABLED,
    CAR_NOT_READY,
    NUM_STATES
} FSMState_e;


typedef struct {
    FSMState_e  stateName;
    void      (*stateHandler)(void);
    int8_t      NextStates[NEXT_STATES_LENGTH];
} MocoState_t;


extern MocoState_t FSM[NUM_STATES];
extern MocoState_t currentState;


void     fsm_init(void);
void     fsm_step(void);
void     fsm_disable(void);
void     fsm_recover(void);
void     fsm_set_precharge_complete(bool val);

uint16_t fsm_get_car_status(void);
bool     fsm_is_over_rollover_speed(void);

void     Task_UpdateControlStatus(void *args);
void     Task_FSM(void *args);
void     Task_BroadcastVCUStatus(void *args);


float map_to_percent(uint8_t input, uint8_t in_min, uint8_t in_max, 
                     uint8_t out_min, uint8_t out_max);
