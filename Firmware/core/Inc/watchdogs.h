/**
 * @file watchdogs.h
 * @brief CAN message watchdog monitoring
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#pragma once

#include "FreeRTOS.h"
#include "timers.h"
#include "faults.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_WD_TIMERS  20

#define WATCHDOG_LIST(X) \
    X(DRIVER_INPUT,     "wd_driver_input",     150U, FAULT_ID_PEDAL_BOARD_WATCHDOG) \
    X(ACCEL_BRAKE,      "wd_accel_brake",      150U, FAULT_ID_PEDAL_BOARD_WATCHDOG) \
    X(STEERING_ANGLE,   "wd_steering_angle",   300U, FAULT_ID_STEERING_SENSOR_WATCHDOG) \
    X(CONTROLS_STATUS,  "wd_controls_status",  300U, FAULT_ID_CONTROLS_STATUS_WATCHDOG) \
    X(BPS_STATUS,       "wd_bps_status",       300U, FAULT_ID_BPS_STATUS_WATCHDOG)

typedef enum {
#define X(name, str, timeout, fault) WD_IDX_##name,
    WATCHDOG_LIST(X)
#undef X
        WD_IDX_COUNT
} WatchdogIndex_e;

_Static_assert(WD_IDX_COUNT <= MAX_WD_TIMERS, "Too many watchdogs; keep under the max");

#define X(name, str, timeout, fault) \
    enum { WD_TIMEOUT_##name##_MS = timeout };
WATCHDOG_LIST(X)
#undef X

void watchdog_init(void);

// Create a one-shot watchdog timer for a given index and timeout
void watchdog_create(const char *name, uint8_t idx, uint32_t timeout_ms, FaultID_e fault_id);

// Start all created timers (call once after all watchdog_create calls)
void watchdog_start_all(void);
void watchdog_stop_all(void);

// Pet the dog so call on every successful CAN receive (task context)
void watchdog_received_can_message(int idx);

// pet from ISR context
void watchdog_received_can_message_ISR(int idx, BaseType_t *pxHigherPriorityTaskWoken);


bool watchdog_is_alive(int idx);
int  watchdog_count(void);

// create all the watchdogs macro
#define X(name, str, timeout, fault) \
    watchdog_create(str, WD_IDX_##name, timeout, fault)
#define WATCHDOG_INIT_ALL_FSM_SIGNALS()  \
    do {                                 \
        WATCHDOG_LIST(X);                \
    } while (0);