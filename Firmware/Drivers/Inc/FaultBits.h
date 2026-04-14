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
    X(MOTOR_LT_BATTERY)         /* Motor voltage dropped below 80% of battery voltage */           \
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
    FAULT_BIT(FAULT_ID_CONTACTOR_CALLBACK) | \
    FAULT_BIT(FAULT_ID_MOTOR_LT_BATTERY) \
)

#define WARNING_ID_LIST(X) \
    X(MOTOR_DIRECTION_CHANGE_LOCKOUT) \
    X(TIPPING_LIMIT_ACTIVE) \
    X(REGEN_NOT_ALLOWED) \
    X(REGEN_NOT_ENABLED)                      

typedef enum {
#define X(name) WARNING_ID_##name,
    WARNING_ID_LIST(X)
#undef X
    WARNING_ID_COUNT
} WarningID_e;

#if (configUSE_16_BIT_TICKS == 0)
#define MAX_WARNING_BITS 24U
#else
#define MAX_WARNING_BITS 8U
#endif

_Static_assert(WARNING_ID_COUNT <= MAX_WARNING_BITS,
               "Too many warning bits for EventGroup");

// Names for warnings for printing/debugging purposes. Indexed by WarningID_e values
extern const char *warning_names[];

#define WARNING_MASK_ALL       ((1UL << WARNING_ID_COUNT) - 1)

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

/**
 * @brief Set a warning. Non critical and just informational for driver.
 * 
 * @param id The warning to set.
 */
void warning_set(WarningID_e id);

/**
 * @brief Get the state of all warnings. 
 * @return A bitmask of fault bits that are currently active. 
 */
EventBits_t warning_get(void);