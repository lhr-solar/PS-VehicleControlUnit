#pragma once

#include "inits.h"

/* Timing Definitions */
/** Time to wait for the physical contactor to settle before reading feedback */
#define CONTACTOR_SENSE_DELAY  pdMS_TO_TICKS(250)
/** Maximum time allowed for callback execution to prevent task starvation */
#define CALLBACK_BLOCKING_TIME pdMS_TO_TICKS(20)

/**
 * @brief Represents the logical and physical state of a contactor.
 */
typedef enum {
    OPEN = 0,  /**< Circuit is disconnected */
    CLOSED = 1 /**< Circuit is connected */
} contactor_state_t;

/**
 * @brief Identification for specific contactors within the HV system.
 */
typedef enum {
    MOTOR_CONTACTOR,     /* Battery to Motor Contactor */
    MOTOR_PRE_CONTACTOR, /* Post-Precharge Bypass Contactor */
    NUM_CONTACTORS       /* Total count helper */
} contactor_num_t;

/**
 * @brief Contactor hardware abstraction object.
 */
typedef struct {
    contactor_state_t state; /* Current state */
    GpioPin_t sense_pin;     /* Digital input for auxiliary feedback loop */
    GpioPin_t control_pin;   /* Digital output to coil driver/relay */

    /* RTOS Resources */
    TimerHandle_t senseTimer;       /* Handle for non-blocking state verification */
    StaticTimer_t senseTimerBuffer; /* Memory buffer for static timer allocation */
} contactor_t;

/**
 * @brief Hardware/RTOS init for all contactors, timers, and mutexes.
 */
void contactor_init(void);

/**
 * @brief Reads the physical sense pin for a specific contactor.
 * @return 1 if CLOSED, 0 if OPEN.
 */
contactor_state_t contactor_get_sense(contactor_num_t contactor_num);

/**
 * @brief Reads the current logical state for a specific contactor.
 * @return 1 if CLOSED, 0 if OPEN.
 */
contactor_state_t contactor_get_commanded_state(contactor_num_t contactor_num);

/**
 * @brief Commands a contactor state change with safety verification via callback function.
 * @param wait_ms  Wait time for sense delay before returning.
 * @param emergency Immediate execution; bypasses safety callbacks.
 * @return SUCCESS or hardware ERROR code.
 */
ErrorStatus contactor_set(contactor_num_t contactor_num, contactor_state_t state, uint32_t wait_ms,
                          fault_state_t emergency);

/** @brief When fault is detected, opens all contactors. */
void contactor_emergency_open_all();