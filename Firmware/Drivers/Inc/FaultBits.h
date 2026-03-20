// #pragma once

// #include "inits.h"
// #include <event_groups.h>
// #include <stdint.h>

// #define ALL_FAULT_BITS ((1UL << NUM_FAULTS) - 1UL)

// // The max number of fault bits is dependent on the configUSE_16_BIT_TICKS defined in FreeRTOS.h
// #if (configUSE_16_BIT_TICKS == 0)
// #define MAX_FAULT_BITS 24U
// #else
// #define MAX_FAULT_BITS 8U
// #endif

// typedef enum
// {
//     MOTOR_GREATER_THAN_BATTERY_FAULT, // Motor voltage is greater than battery voltage
//     BATTERY_OVERVOLTAGE_FAULT,        // Battery voltage is greater than OVERVOLTAGE_THRESHOLD_MV
//     BATTERY_UNDERVOLTAGE_FAULT,       // Battery voltage is less than UNDERVOLTAGE_THRESHOLD_MV
//     MOTOR_SENSE_TIMEOUT_FAULT,        // Motor contactor didn't close within expected time
//     PRECHARGE_SENSE_TIMEOUT_FAULT,    // Precharge contactor didn't close within expected time
//     PRECHARGE_TIMEOUT_FAULT,          // Precharge sequence took too long
//     CALLBACK_FAULT,                   // Contactor state did not match expected state after being set
//     MOTOR_SENSE_MISMATCH_FAULT,       // Motor contactor sense pin reading does not match contactor state
//     PRECHARGE_SENSE_MISMATCH_FAULT,   // Precharge contactor sense pin reading does not match contactor state
//     NUM_FAULTS
// } fault_bit_t;

// /* Convert enum to bitmask */
// #define FAULT_BIT(fault) (1UL << (fault))

// /* Mask containing only the actual fault bits (exclude precharge state bits)
//     Precharge state enum values are the first entries, so keep bits from
//     MOTOR_GREATER_THAN_BATTERY_FAULT upwards. */
// #define FAULTS_ONLY_MASK ((EventBits_t)(ALL_FAULT_BITS & ~((1UL << (MOTOR_GREATER_THAN_BATTERY_FAULT)) - 1UL)))

// /* Legacy name kept for callers that expect a mask of fault bits */
// #define FAULT_BITMASK (FAULTS_ONLY_MASK)

// _Static_assert(NUM_FAULTS <= MAX_FAULT_BITS, "Too many fault bits for EventGroup");

// /**
//  * @brief Initializes fault bitmap
//  *
//  * @param none
//  * @return 0 on failure, 1 on success
//  */
// uint8_t faultBits_init(void);

// /**
//  * @brief Set a fault in the fault bitmap
//  *
//  * @param bit which fault is being set
//  * @return none
//  */
// void set_faultBit(fault_bit_t bit);

// /**
//  * @brief Wait for a fault to be set
//  *
//  * @param bit which fault to wait for, pass NUM_FAULTS if waiting for any fault
//  * @param xTicksToWait delay when waiting
//  * @return the event bit that was set
//  */
// EventBits_t faultBit_wait(fault_bit_t bit, TickType_t xTicksToWait);

// /**
//  * @brief Set a fault in the fault bitmap from an ISR
//  *
//  * @param bit which fault is being set
//  * @return none
//  */
// void set_faultBitFromISR(fault_bit_t bit);

#pragma once

#include "FreeRTOS.h"
#include "event_groups.h"
#include <stdint.h>
#include <stdbool.h>

#define FAULT_ID_LIST(X) \
    /* ============= MOTOR CONTROLLER ============ */ \
    X(MOTOR_HARDWARE_OVERCURRENT) \
    X(MOTOR_SOFTWARE_OVERCURRENT) \
    X(MOTOR_DC_BUS_OVERVOLTAGE) \
    X(MOTOR_BAD_HALL_SEQUENCE) \
    X(MOTOR_WD_RESET) \
    X(MOTOR_CONFIG_READ) \
    X(MOTOR_15V_UNDERVOLTAGE) \
    X(MOTOR_DESATURATION) \
    X(MOTOR_OVERSPEED) \
    \
    /* ================ PRECHARGE ================ */                                              \
    X(PRECHARGE_TIMEOUT)        /* Prech sequence took too long */                                 \
    X(PRECHARGE_SENSE_TIMEOUT)  /* Prech contactor didn't close within expected time */            \
    X(PRECHARGE_SENSE_MISMATCH) /* Prech contactor sense reading doesnt match contactor state */   \
    X(MOTOR_SENSE_TIMEOUT)      /* Motor contactor didn't close within expected time */            \
    X(MOTOR_SENSE_MISMATCH)     /* Motor contactor sense reading doesnt match contactor state */   \
    X(BATTERY_OVERVOLTAGE)      /* Battery voltage is greater than OVERVOLTAGE_THRESHOLD_MV */     \
    X(BATTERY_UNDERVOLTAGE)     /* Battery voltage is less than UNDERVOLTAGE_THRESHOLD_MV */       \
    X(MOTOR_GT_BATTERY)         /* Motor voltage is greater than battery voltage */                \
    X(CONTACTOR_CALLBACK)       /* Contactor state didnt match expected state after being set */   \
    \
    /* ============== OTHER BOARDS =============== */                                              \
    X(STEERING_SENSOR_FAULT)    /* Sensor not OK or data invalid */                                \
    X(PEDAL_BOARD_FAULT)        /* Pedal board fault or data invalid */                            \
    X(CONTROLS_FAULT)           /* Fault in controls status */                                     \
    X(BPS_FAULT)                /* Fault in BPS status */                                          \
    X(GENERIC_WATCHDOG_FAULT)   /* A generic fault triggereed by all dogs */                      

typedef enum {
#define X(name) FAULT_ID_##name,
    FAULT_ID_LIST(X)
#undef X
    FAULT_ID_COUNT
} FaultID_e;

#if (configUSE_16_BIT_TICKS == 0)
#define MAX_FAULT_BITS 24U
#else
#define MAX_FAULT_BITS 8U
#endif

_Static_assert(FAULT_ID_COUNT <= MAX_FAULT_BITS,
               "Too many fault bits for EventGroup");

// Names for faults for printing/debugging purposes. Indexed by FaultID_e values
extern const char *fault_names[];


#define FAULT_BIT(id)        (1UL << (id))
#define FAULT_MASK_ALL       ((1UL << FAULT_ID_COUNT) - 1)


#define FAULT_MASK_MOTOR_ALL ( \
    FAULT_BIT(FAULT_ID_MOTOR_HARDWARE_OVERCURRENT) | \
    FAULT_BIT(FAULT_ID_MOTOR_SOFTWARE_OVERCURRENT) | \
    FAULT_BIT(FAULT_ID_MOTOR_DC_BUS_OVERVOLTAGE) | \
    FAULT_BIT(FAULT_ID_MOTOR_BAD_HALL_SEQUENCE) | \
    FAULT_BIT(FAULT_ID_MOTOR_WD_RESET) | \
    FAULT_BIT(FAULT_ID_MOTOR_CONFIG_READ) | \
    FAULT_BIT(FAULT_ID_MOTOR_15V_UNDERVOLTAGE) | \
    FAULT_BIT(FAULT_ID_MOTOR_DESATURATION) | \
    FAULT_BIT(FAULT_ID_MOTOR_OVERSPEED) \
)

#define FAULT_MASK_PRECHARGE_ALL ( \
    FAULT_BIT(FAULT_ID_PRECHARGE_TIMEOUT) | \
    FAULT_BIT(FAULT_ID_PRECHARGE_SENSE_TIMEOUT) | \
    FAULT_BIT(FAULT_ID_PRECHARGE_SENSE_MISMATCH) | \
    FAULT_BIT(FAULT_ID_MOTOR_SENSE_TIMEOUT) | \
    FAULT_BIT(FAULT_ID_MOTOR_SENSE_MISMATCH) | \
    FAULT_BIT(FAULT_ID_BATTERY_OVERVOLTAGE) | \
    FAULT_BIT(FAULT_ID_BATTERY_UNDERVOLTAGE) | \
    FAULT_BIT(FAULT_ID_MOTOR_GT_BATTERY) | \
    FAULT_BIT(FAULT_ID_CONTACTOR_CALLBACK) \
)

/**
 * @brief Initializes fault bitmap and associated data structures. Must be 
 * called before using any other functions in this module.
 * 
 * @return true if initialization succeeded, false if it failed 
 */
bool faults_init(void);

/**
 * @brief Sets a fault bit.
 * 
 * @param id The fault ID to set.
 */
void faults_set(FaultID_e id);

/**
 * @brief Sets a fault bit from an ISR context. 
 * 
 * This is a separate function from the non-ISR version because it needs to use 
 * FromISR FreeRTOS APIs and handle the pxHigherPriorityTaskWoken parameter.
 * 
 * @param id The fault ID to set.
 */
void faults_set_from_isr(FaultID_e id);

/**
 * @brief Sets multiple fault bits at once using a mask. This is useful for 
 * cases where multiple faults need to be set simultaneously.
 * 
 * @param mask A bitmask of fault bits to set. Only bits corresponding to valid 
 * fault IDs will be set; other bits will be ignored.
 */
void faults_set_mask(EventBits_t mask);

/**
 * @brief Clears a fault bit.
 * 
 * @param id The fault ID to clear.
 */
void faults_clear(FaultID_e id);

/**
 * @brief Checks if a specific fault bit is active.
 * 
 * @param id The fault ID to check.
 * @return true if the specified fault bit is active, false otherwise.
 */
bool faults_is_active(FaultID_e id);

/**
 * @brief Checks if any fault bits are active.
 * 
 * @return true if any fault bits are active, false if no fault bits are active.
 */
bool faults_any_active(void);

/**
 * @brief Gets the current active fault bits as a bitmask.
 * 
 * @return A bitmask where each bit corresponds to an active fault. Only bits
 * corresponding to valid fault IDs will be set; other bits will be 0.
 */
EventBits_t faults_get(void);


/**
 * @brief Waits for specific fault bits to become active.
 * 
 * @param id The fault ID to wait for, or NUM_FAULTS to wait for any fault.
 * @param ticks The maximum time to wait in ticks. Use portMAX_DELAY to wait 
 * indefinitely
 * @return A bitmask of the fault bits that are currently active. If waiting for 
 * a specific fault ID, the returned bitmask will have that bit set if it became
 * active. If waiting for any fault, the returned bitmask will have all active 
 * fault bits set.
 */
EventBits_t faults_wait(FaultID_e id, TickType_t ticks);
