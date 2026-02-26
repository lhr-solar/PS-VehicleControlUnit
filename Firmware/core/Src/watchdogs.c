/**
 * @file watchdogs.c
 * @brief Implementation of CAN message watchdog monitoring
 * 
 * Uses FreeRTOS software timers to monitor periodic CAN messages.
 * Each timer checks if its corresponding message was received since the
 * last timeout period. Missing messages trigger fault handling.
 * 
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#include "watchdogs.h"
#include "timers.h"
#include "faults.h"
#include <stdio.h>
#include <assert.h>


static StaticEventGroup_t xWdEventGroupBuffer; // Event group backing
EventGroupHandle_t xWdEventGroup = NULL; // Event group handle for recv msgs
static StaticTimer_t xCANWdTimerBuffers[MAX_CAN_WD_TIMERS]; // software timer backing storage
static TimerHandle_t xCANWdTimers[MAX_CAN_WD_TIMERS]; // Software timer handles
static int wd_timer_count = 0;



/**
 * @brief Generic callback for all CAN watchdog timers
 * 
 * Called by FreeRTOS timer daemon when a watchdog timer expires.
 * Checks if the corresponding message was received; if not, throws a fault.
 * 
 * @param xTimer Handle to the timer that expired
 */
static void wd_callback_generic(TimerHandle_t xTimer);



void watchdog_init(void) {
    xWdEventGroup = xEventGroupCreateStatic(&xWdEventGroupBuffer);
    configASSERT(xWdEventGroup != NULL);

    // make sure we start at 0
    xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS);
    
    wd_timer_count = 0;
    
    #ifdef DEBUG
    printf("Watchdog system initialized\n");
    #endif
}

void watchdog_received_can_message(FSM_Signal_t signal) {
    configASSERT(xWdEventGroup != NULL);
    configASSERT(signal < FSM_SIGNAL_COUNT);

    // set the bit corresponding to the received message
    BaseType_t result = xEventGroupSetBits(xWdEventGroup, (EventBits_t)(1U << signal));
    
    // Check if called from ISR (should use xEventGroupSetBitsFromISR instead) */
    if (result == pdFAIL) {
        /* This shouldn't happen with event groups, but log it for debug */
        printf("Warning: Failed to set watchdog bit for signal %d\n", signal);
    }
}

void watchdog_create_can(const char *timer_name,
                         uint16_t can_id,
                         uint32_t timeout_ms) {
    configASSERT(wd_timer_count < MAX_CAN_WD_TIMERS);
    configASSERT(xWdEventGroup != NULL);
    configASSERT(timer_name != NULL);
    configASSERT(signal < FSM_SIGNAL_COUNT);
    configASSERT(timeout_ms > 0);

    /* Create a software timer with auto-reload enabled */
    TimerHandle_t timer = xTimerCreateStatic(
        timerName,
        pdMS_TO_TICKS(timeout_ms),
        pdTRUE,  /* Auto-reload: timer restarts after expiring */
        (void *)(uintptr_t)signal,  /* Store signal as timer ID */
        generic_can_wd_callback,
        &xCANWdTimerBuffers[wd_timer_count]
    );

    configASSERT(timer != NULL);

    /* Store timer handle */
    xCANWdTimers[wd_timer_count] = timer;
    wd_timer_count++;

    printf("Created watchdog timer '%s' for signal %d (timeout: %lu ms)\n",
           timerName, signal, timeout_ms);
}

void CAN_MSG_Watchdog_StartAll(void)
{
    configASSERT(xWdEventGroup != NULL);
    
    int started = 0;
    
    for (int i = 0; i < wd_timer_count; i++) {
        if (xCANWdTimers[i] != NULL) {
            BaseType_t result = xTimerStart(xCANWdTimers[i], portMAX_DELAY);
            if (result == pdPASS) {
                started++;
            } else {
                printf("Error: Failed to start watchdog timer %d\n", i);
            }
        }
    }
    
    printf("Started %d/%d watchdog timers\n", started, wd_timer_count);
}

void CAN_MSG_Watchdog_StopAll(void)
{
    int stopped = 0;
    
    for (int i = 0; i < wd_timer_count; i++) {
        if (xCANWdTimers[i] != NULL) {
            BaseType_t result = xTimerStop(xCANWdTimers[i], portMAX_DELAY);
            if (result == pdPASS) {
                stopped++;
            }
        }
    }
    
    printf("Stopped %d/%d watchdog timers\n", stopped, wd_timer_count);
}

/* ========================================================================== */
/* Private implementation                                                     */
/* ========================================================================== */

static void generic_can_wd_callback(TimerHandle_t xTimer)
{
    /* 
     * IMPORTANT: This runs in the FreeRTOS timer daemon task context.
     * Keep this function short and non-blocking.
     * Avoid heavy processing, blocking calls, or printf in production.
     */
    
    /* Retrieve which signal this timer is monitoring */
    FSM_Signal_t signal = (FSM_Signal_t)(uintptr_t)pvTimerGetTimerID(xTimer);
    EventBits_t bitMask = (EventBits_t)(1U << signal);

    /* Check if message was received since last timeout */
    EventBits_t currentBits = xEventGroupGetBits(xWdEventGroup);

    if (currentBits & bitMask) {
        /* 
         * Message was received in the last period.
         * Clear the bit for the next window.
         */
        xEventGroupClearBits(xWdEventGroup, bitMask);
        return;  /* No timeout - all good */
    }

    /*
     * Timeout detected: message not received within timeout period.
     * This is a critical fault condition.
     */
    
    /* Get CAN ID for logging (bounds check signal) */
    uint16_t can_id = 0xFFF;  /* Invalid CAN ID as fallback */
    if (signal < FSM_SIGNAL_COUNT) {
        can_id = fsm_signal_to_can_id[signal];
    }

    /* Log the timeout (disable this printf in production for performance) */
#ifdef DEBUG
    printf("WATCHDOG TIMEOUT: CAN ID 0x%03X (signal %d) not received\n",
           (unsigned int)can_id, (int)signal);
#endif

    /* 
     * Throw a system fault.
     * This should transition the FSM to a safe state and disable drive.
     */
    Faults_ThrowFault(FAULT_ID_WATCHDOG_FSM);
    
    /*
     * Note: We don't stop the timer here. The fault handler should
     * call CAN_MSG_Watchdog_StopAll() to prevent repeated faults.
     * This allows the system to log which messages are missing.
     */
}

/* ========================================================================== */
/* Optional: ISR-safe variant                                                 */
/* ========================================================================== */

/**
 * @brief ISR-safe version of watchdog_received_can_message
 * 
 * Call this from CAN RX interrupt handlers instead of the regular version.
 * 
 * @param signal The FSM signal corresponding to the received message
 * @param pxHigherPriorityTaskWoken Set to pdTRUE if a context switch is needed
 */
void watchdog_received_can_message_ISR(FSM_Signal_t signal,
                                       BaseType_t *pxHigherPriorityTaskWoken)
{
    configASSERT(xWdEventGroup != NULL);
    configASSERT(signal < FSM_SIGNAL_COUNT);

    /* Use ISR-safe version of event group set */
    xEventGroupSetBitsFromISR(
        xWdEventGroup,
        (EventBits_t)(1U << signal),
        pxHigherPriorityTaskWoken
    );
}

/* ========================================================================== */
/* Optional: Diagnostic functions                                             */
/* ========================================================================== */

/**
 * @brief Get current watchdog status for all signals
 * 
 * Useful for debugging - shows which messages have been received
 * in the current window.
 * 
 * @return Bitfield where each bit represents one FSM_Signal_t
 */
EventBits_t Watchdog_GetStatus(void)
{
    if (xWdEventGroup == NULL) {
        return 0;
    }
    return xEventGroupGetBits(xWdEventGroup);
}

/**
 * @brief Check if a specific signal's watchdog is fed
 * 
 * @param signal Signal to check
 * @return true if message was received in current window, false otherwise
 */
bool Watchdog_IsSignalAlive(FSM_Signal_t signal)
{
    if (xWdEventGroup == NULL || signal >= FSM_SIGNAL_COUNT) {
        return false;
    }
    
    EventBits_t bits = xEventGroupGetBits(xWdEventGroup);
    return (bits & (1U << signal)) != 0;
}

/**
 * @brief Get number of active watchdog timers
 * 
 * @return Number of timers created
 */
int Watchdog_GetTimerCount(void)
{
    return wd_timer_count;
}

/**
 * @brief Reset all watchdog bits (mark all as not received)
 * 
 * Useful for testing or fault recovery procedures.
 */
void Watchdog_ResetAllBits(void) {
    if (xWdEventGroup != NULL) {
        xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS);
    }
}
