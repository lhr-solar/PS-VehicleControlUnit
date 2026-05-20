/**
 * @file Watchdogs.h
 * @brief CAN message watchdog monitoring
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#pragma once

#include "FreeRTOS.h"
#include "timers.h"
#include "FaultBits.h"
#include <stdbool.h>
#include <stdint.h>

#define MAX_WD_TIMERS 10

#define NODAWG 1

/**
 * @brief List of watchdogs to create, along with their timeouts and fault IDs
 * Format: X(name, string_name, timeout_ms, fault_id)
 * - name: enum name for the watchdog index (e.g. WD_IDX_DRIVER_INPUT)
 * - string_name: name to show in printf when the dog times out (e.g. "wd_driver_input")
 * - timeout_ms: how long the watchdog should wait before timing out in milliseconds (e.g. 150)
 * - fault_id: which fault to throw when the watchdog times out
 * 
 * Currently all watchdogs throw the same generic fault, but you can specify different 
 * faults for each one if you want to get more specific information about which signal 
 * caused the fault. Just make sure to add the new fault IDs to FaultBits.h as well.
 */
#define WATCHDOG_LIST(X)                                                                           \
    X(DRIVER_INPUT, "wd_driver_input", 1000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)                     \
    X(ACCEL_BRAKE, "wd_accel_brake", 1000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)                       \
    X(BRAKE_PRESSURE_1, "wd_brake_pressure_1", 1000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)             \
    X(BRAKE_PRESSURE_2, "wd_brake_pressure_2", 1000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)             \
    X(STEERING_ANGLE, "wd_steering_angle", 1000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)                 \
    X(CONTROLS_STATUS, "wd_controls_status", 1000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)               \
    X(BPS_STATUS, "wd_bps_status", 2000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)                         \
    X(MOCO_STATUS, "wd_moco_status", 1000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)                       \
    X(MOCO_VELOCITY, "wd_moco_velocity", 1000U, FAULT_ID_GENERIC_WATCHDOG_FAULT)

typedef enum {
#define X(name, str, timeout, fault) WD_IDX_##name,
    WATCHDOG_LIST(X)
#undef X
        WD_IDX_COUNT
} WatchdogIndex_e;

_Static_assert(WD_IDX_COUNT <= MAX_WD_TIMERS, "Too many watchdogs; keep under the max");

#define X(name, str, timeout, fault) enum { WD_TIMEOUT_##name##_MS = timeout };
WATCHDOG_LIST(X)
#undef X


/**
 * @brief Initialize all watchdogs. This must be called before using 
 * any watchdog timers.
 */
void watchdog_init(void);

/**
 * @brief Create a watchdog timer for a given index. The timer will be created 
 * in a stopped state, so you need to call watchdog_start_all to start it after 
 * creating all the timers.
 */
void watchdog_create(const char *name, uint8_t idx, uint32_t timeout_ms, FaultID_e fault_id);

/**
 * @brief Start all watchdog timers. Should be called after creating all timers 
 * with watchdog_create, and also after handling a fault to restart the timers 
 * once the fault condition has been resolved. 
 * 
 * Note that if you call this function while a timer is already running, it will
 * reset the timer back to its full timeout.
 */
void watchdog_start_all(void);

/**
 * @brief Stop all watchdog timers. Should be called when entering a fault state
 * to prevent any watchdog timeouts from triggering additional faults while
 * we're already handling a fault.
 */
void watchdog_stop_all(void);

/**
 * @brief Pet the watchdog for a given index. Should be called from the task 
 * context when a CAN message is received.
 */
void watchdog_received_can_message(uint8_t idx);

/**
 * @brief Pet the watchdog from an ISR context. This is a separate function from
 * the non-ISR version because it needs to use FromISR FreeRTOS APIs and handle 
 * the pxHigherPriorityTaskWoken parameter.
 */
void watchdog_received_can_message_ISR(uint8_t idx, BaseType_t *pxHigherPriorityTaskWoken);

/**
 * @brief Check if the watchdog for a given index is alive (i.e. has been petted
 * recently enough that it hasn't timed out).
 */
bool watchdog_is_alive(int idx);

/**
 * @brief Get the number of watchdog timers that have been created. Useful for 
 * iterating over all timers or for debugging.
 */
uint8_t watchdog_count(void);
