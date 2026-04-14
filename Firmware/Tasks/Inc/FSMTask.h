/**
 * @file FSM.h
 * @brief Prohelion wavesculptor 22 motor controller FSM types and public API
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 *
 * WARNING: BITFIELD_INPUT_LIST order must match BITS dict in
 *          scripts/fsm_generator.py. If you add/reorder entries,
 *          update that script and regenerate fsm_table.h. Same
 *          with FSMState_e
 */

#pragma once

#include "inits.h"
#include "CANbus.h"
#include "event_groups.h"

/**
 * @brief A list of inputs; uses X macro pattern
 * @note Make sure if you change this to also change it in scripts/fsm_generator.py
 */
#define BITFIELD_INPUT_LIST(X)                                                                     \
    X(NEUTRAL)                                                                                     \
    X(FORWARD)                                                                                     \
    X(REVERSE)                                                                                     \
    X(CRUISE_CONTROL_BUTTON)                                                                       \
    X(REGEN_BUTTON)                                                                                \
    X(READY_TO_REGEN)                                                                              \
    X(REGEN_ENABLED)                                                                               \
    X(BRAKE)                                                                                       \
    X(PRECHARGE_COMPLETE)

/**
 * @brief An enum of all FSM inputs
 */
typedef enum {
#define X(name) BIT_IDX_##name,
    BITFIELD_INPUT_LIST(X)
#undef X
        BITFIELD_INPUT_COUNT
} InputBitsIdx_e;

#define FSM_INPUT_BIT(x) (1U << x) 

typedef enum {
#define X(name) name##_BIT = (1U << BIT_IDX_##name),
    BITFIELD_INPUT_LIST(X)
#undef X
} InputBits_t;

#define NEXT_STATES_LENGTH      (1U << BITFIELD_INPUT_COUNT)
#define FSM_INPUTS_MASK_ALL     ((1U << BITFIELD_INPUT_COUNT) - 1U)

// must match in the generator script
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
    FSMState_e stateName;
    void (*stateHandler)(void);
    uint8_t NextStates[NEXT_STATES_LENGTH];
} MocoState_t;

extern MocoState_t FSM[NUM_STATES];
extern MocoState_t current_state;

/**
 * @brief Used for stepping current down at higher speeds. An array of these will 
 * represent a bin for min speed and associated max current.
 */
typedef struct {
    float speed_mph;
    uint8_t max_percent;
} swoc_threshold_t;


void fsm_init(void);
void fsm_step(void);
void fsm_disable(void);
void fsm_recover(void);

void fsm_set_all_inputs(EventBits_t mask);
void fsm_set_input(EventBits_t mask);
uint16_t fsm_get_inputs(void);

bool fsm_is_over_rollover_speed(void);

void FSM_TaskInit();
void Task_FSM(void *args);

float map_to_percent(uint8_t input, uint8_t in_min, uint8_t in_max, uint8_t out_min,
                     uint8_t out_max);
