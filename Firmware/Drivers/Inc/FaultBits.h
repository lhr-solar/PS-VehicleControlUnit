#pragma once

#include "inits.h"
#include <event_groups.h>
#include <stdint.h>

#define ALL_FAULT_BITS ((1UL << NUM_FAULTS) - 1UL)

// The max number of fault bits is dependent on the configUSE_16_BIT_TICKS defined in FreeRTOS.h
#if (configUSE_16_BIT_TICKS == 0)
#define MAX_FAULT_BITS 24U
#else
#define MAX_FAULT_BITS 8U
#endif

typedef enum
{
    MOTOR_GREATER_THAN_BATTERY_FAULT, // Motor voltage is greater than battery voltage
    BATTERY_OVERVOLTAGE_FAULT,        // Battery voltage is greater than OVERVOLTAGE_THRESHOLD_MV
    BATTERY_UNDERVOLTAGE_FAULT,       // Battery voltage is less than UNDERVOLTAGE_THRESHOLD_MV
    MOTOR_SENSE_TIMEOUT_FAULT,        // Motor contactor didn't close within expected time
    PRECHARGE_SENSE_TIMEOUT_FAULT,    // Precharge contactor didn't close within expected time
    PRECHARGE_TIMEOUT_FAULT,          // Precharge sequence took too long
    CALLBACK_FAULT,                   // Contactor state did not match expected state after being set
    MOTOR_SENSE_MISMATCH_FAULT,       // Motor contactor sense pin reading does not match contactor state
    PRECHARGE_SENSE_MISMATCH_FAULT,   // Precharge contactor sense pin reading does not match contactor state
    BPS_FAULT,                        // Fault bit for any fault reported by BPS
    NUM_FAULTS
} fault_bit_t;

typedef enum
{
    PRECHARGE_WAITING_STATE,
    PRECHARGE_INITIAL_STATE,     // Indiciates we are in the inital state when set
    PRECHARGE_PRECHARGING_STATE, // Indicates we are in the precharging state when set
    PRECHARGE_RUN_STATE,         // Indicates we are in the run state when set
    NUM_STATES
} state_bit_t;

#define FAULT_NUM 10

/* Convert enum to bitmask */
#define FAULT_BIT(fault) (1UL << (fault))
#define FAULT_BITMASK ((EventBits_t)((1UL << NUM_FAULTS) - 1UL))

#define STATE_BIT(state) (1UL << (state))
#define STATE_BITMASK ((EventBits_t)((1UL << NUM_STATES) - 1UL))

_Static_assert(NUM_FAULTS <= MAX_FAULT_BITS, "Too many fault bits for EventGroup");

/**
 * @brief Initializes fault bitmap
 *
 * @param none
 * @return 0 on failure, 1 on success
 */
uint8_t faultBits_init(void);

/**
 * @brief Set a fault in the fault bitmap
 *
 * @param bit which fault is being set
 * @return none
 */
void set_faultBit(fault_bit_t bit);

/**
 * @brief Wait for a fault to be set
 *
 * @param bit which fault to wait for, pass NUM_FAULTS if waiting for any fault
 * @param xTicksToWait delay when waiting
 * @return the event bit that was set
 */
EventBits_t faultBit_wait(fault_bit_t bit, TickType_t xTicksToWait);

/**
 * @brief Initializes state bitmap
 *
 * @param none
 * @return 0 on failure, 1 on success
 */
uint8_t stateBits_init(void);

/**
 * @brief Set a state in the state bitmap
 *
 * @param bit which state is being set
 * @return none
 */
void set_stateBit(state_bit_t bit);

/**
 * @brief Set a fault in the fault bitmap from an ISR
 *
 * @param bit which fault is being set
 * @return none
 */
void set_faultBitFromISR(fault_bit_t bit);

extern EventGroupHandle_t faultStateBits;
extern EventGroupHandle_t stateBits;