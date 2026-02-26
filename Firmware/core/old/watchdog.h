// vcu_watchdog.h
/**
 * @copyright Copyright (c) 2018-2023 UT Longhorn Racing Solar
 * @file vcu_watchdog.h
 * @brief Watchdog timer management for CAN message reception
 * 
 * Monitors all expected CAN messages and triggers faults if messages
 * arrive too late or not at all. Uses a single monitor task instead
 * of per-message timers for efficiency.
 * 
 * @defgroup VCUWATCHDOG
 * @addtogroup VCUWATCHDOG
 * @{
 */

#ifndef VCU_WATCHDOG_H
#define VCU_WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "can_ids.h"
#include "vcu_fsm.h"  // For FSM_Signal_e enum

// ============================================================================
// WATCHDOG CONFIGURATION
// ============================================================================

/**
 * Watchdog entry for a single CAN message
 * Tracks last reception time and timeout threshold
 */
typedef struct {
    uint32_t can_id;                // CAN message ID to monitor
    const char *name;               // Human-readable name for logging
    uint16_t timeout_ms;            // Timeout threshold in milliseconds
    TickType_t last_received_tick;  // Last reception time (FreeRTOS tick)
    bool is_faulted;                // Current fault state
    uint32_t miss_count;            // Number of consecutive misses (telemetry)
} WatchdogEntry_t;

/**
 * Watchdog manager - tracks all watched messages
 */
typedef struct {
    WatchdogEntry_t entries[FSM_SIGNAL_COUNT];
    uint16_t count;
    TaskHandle_t monitor_task;
    uint32_t check_cycles;          // Telemetry counter
} WatchdogManager_t;

// WATCHDOG TIMEOUT CONSTANTS
#define WD_TIMEOUT_PEDALS 150           // Pedals: 100ms nominal → 150ms timeout
#define WD_TIMEOUT_GEARS 200            // Gears: ~150ms nominal → 200ms timeout
#define WD_TIMEOUT_REGEN_BUTTON 300     // Button: ~200ms nominal → 300ms timeout
#define WD_TIMEOUT_REGEN_ENABLED 300
#define WD_TIMEOUT_CRUISE_CONTROL 300
#define WD_TIMEOUT_BPS_OK_TO_REGEN 200  // BPS: ~100ms nominal → 200ms timeout
#define WD_TIMEOUT_BPS_TRIP 200
#define WD_TIMEOUT_IGNITION 500         // Ignition: slower → 500ms timeout

// Critical messages (must not timeout) vs optional
#define CRITICAL_WD_MESSAGES (WD_MASK_PEDALS | WD_MASK_GEARS | \
                              WD_MASK_BPS_TRIP | WD_MASK_IGNITION)







/**
 * @brief Initialize watchdog manager
 * Must be called once during system initialization
 * @param mgr Watchdog manager to initialize
 * @return true if initialization successful
 */
bool vcu_watchdog_init(WatchdogManager_t *mgr);

/**
 * @brief Mark a CAN message as received (call from CAN RX task)
 * @param mgr Watchdog manager
 * @param signal FSM_Signal_e indicating which message was received
 */
void vcu_watchdog_mark_received(WatchdogManager_t *mgr, FSMSignal_e signal);

/**
 * @brief Get current watchdog fault state
 * @param mgr Watchdog manager
 * @param signal Which message to check
 * @return true if that message is currently faulted (timeout expired)
 */
bool vcu_watchdog_is_faulted(WatchdogManager_t *mgr, FSMSignal_e signal);

/**
 * @brief Get all active watchdog faults as a bitmask
 * @param mgr Watchdog manager
 * @return Bitmask of faulted messages (bit N = faulted if set)
 */
uint32_t vcu_watchdog_get_faults_bitmask(WatchdogManager_t *mgr);

/**
 * @brief Clear watchdog fault for a specific message (recovery)
 * @param mgr Watchdog manager
 * @param signal Which message fault to clear
 */
void vcu_watchdog_clear_fault(WatchdogManager_t *mgr, FSMSignal_e signal);

/**
 * @brief Clear all watchdog faults (after system recovery)
 * @param mgr Watchdog manager
 */
void vcu_watchdog_clear_all_faults(WatchdogManager_t *mgr);

/**
 * @brief Watchdog monitor task - runs periodically to check message timeouts
 * Priority: 1 (high, but below error task)
 * @param pvParameters Pointer to WatchdogManager_t
 */
void vcu_watchdog_monitor_task(void *pvParameters);

/**
 * @brief Get telemetry for a specific watched message
 * @param mgr Watchdog manager
 * @param signal Which message
 * @return Number of consecutive misses
 */
uint32_t vcu_watchdog_get_miss_count(WatchdogManager_t *mgr, FSMSignal_e signal);


#endif  /* vcu_watchdog.h */

