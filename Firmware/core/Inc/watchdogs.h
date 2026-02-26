/**
 * @file watchdogs.h
 * @brief CAN message watchdog monitoring for FSM signals
 * 
 * Implements software timer-based watchdogs for critical CAN messages.
 * Each FSM signal has a dedicated watchdog that monitors message reception.
 * If a message is not received within the timeout period, a fault is thrown.
 * 
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#pragma once

#include "FreeRTOS.h"
#include "event_groups.h"
#include <stdint.h>

/**
 * Watchdog timeouts in milliseconds.
 * timeout ~= 3x message cycle time
 * 
 * Messages with 100ms cycle time --> 300ms timeout
 */
#define PEDALS_TIMEOUT_MS          300U
#define GEARS_TIMEOUT_MS           300U
#define REGEN_BUTTON_TIMEOUT_MS    300U
#define REGEN_ENABLED_TIMEOUT_MS   300U
#define CRUISE_TIMEOUT_MS          300U
#define BPS_OK_TIMEOUT_MS          600U  // based on actual BPS message rate?
#define BPS_TRIP_TIMEOUT_MS        600U  // based on actual BPS message rate?
#define IGN_STATE_TIMEOUT_MS       300U

/* Maximum number of CAN watchdog timers */
#define MAX_CAN_WD_TIMERS          20

/**
 * Event group for tracking received CAN messages.
 * Each bit corresponds to one FSM_Signal_t.
 * Bit is set when message received, cleared by watchdog callback.
 */
extern EventGroupHandle_t xWdEventGroup;



/**
 * @brief Initialize the watchdog system
 * 
 * Creates the event group for tracking message reception.
 * Must be called before creating any watchdog timers.
 * Call once during system initialization.
 */
void watchdog_init(void);

/**
 * @brief Mark a CAN message as received
 * 
 * Called from CAN receive handlers when a valid message is decoded.
 * Sets the corresponding bit in the watchdog event group.
 * 
 * @param signal The FSM signal corresponding to the received message
 */
void watchdog_received_msg(FSM_Signal_t signal);

/**
 * @brief Create a CAN message watchdog timer
 * 
 * Creates a FreeRTOS software timer to monitor a specific CAN message.
 * The timer auto-reloads and checks for message reception on each period.
 * 
 * @param timer_name name for debugging
 * @param signal The FSM signal this watchdog monitors
 * @param timeout_ms Watchdog timeout in milliseconds
 * 
 * @note must call only after watchdog_init()
 * @note at  most `MAX_CAN_WD_TIMERS` timers
 */
void watchdog_create_can(const char *timer_name,
                         uint16_t can_id,
                         uint32_t timeout_ms);

/**
 * @brief Start all created watchdog timers
 * 
 * Starts monitoring all CAN messages.
 * Call once after creating all watchdog timers, before entering main loop.
 */
void watchdog_start_all(void);

/**
 * @brief Stop all watchdog timers
 * 
 * Stops all CAN message monitoring.
 * Use during shutdown or when entering a safe/disabled state.
 */
void watchdog_stop_all(void);

/* ========================================================================== */
/* Initialization helper macro                                                */
/* ========================================================================== */

/**
 * @brief Initialize all FSM signal watchdogs with default timeouts
 * 
 * Call this macro after watchdog_init() to create all standard watchdogs.
 * Timers must be started afterward with CAN_MSG_Watchdog_StartAll().
 */
#define WATCHDOG_INIT_ALL_FSM_SIGNALS()                                        \
    do {                                                                       \
        CAN_MSG_Watchdog_Create("wd_PEDALS",          FSM_PEDALS,          PEDALS_TIMEOUT_MS);          \
        CAN_MSG_Watchdog_Create("wd_GEARS",           FSM_GEARS,           GEARS_TIMEOUT_MS);           \
        CAN_MSG_Watchdog_Create("wd_REGEN_BUTTON",    FSM_REGEN_BUTTON,    REGEN_BUTTON_TIMEOUT_MS);    \
        CAN_MSG_Watchdog_Create("wd_REGEN_ENABLED",   FSM_REGEN_ENABLED,   REGEN_ENABLED_TIMEOUT_MS);   \
        CAN_MSG_Watchdog_Create("wd_CRUISE_CONTROL",  FSM_CRUISE_CONTROL,  CRUISE_TIMEOUT_MS);          \
        CAN_MSG_Watchdog_Create("wd_BPS_OK_TO_REGEN", FSM_BPS_OK_TO_REGEN, BPS_OK_TIMEOUT_MS);          \
        CAN_MSG_Watchdog_Create("wd_BPS_TRIP",        FSM_BPS_TRIP,        BPS_TRIP_TIMEOUT_MS);        \
        CAN_MSG_Watchdog_Create("wd_IGN_STATE",       FSM_IGNITION_STATE,  IGN_STATE_TIMEOUT_MS);       \
    } while (0)

#endif /* WATCHDOGS_H */
