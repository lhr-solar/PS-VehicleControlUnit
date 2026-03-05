/**
 * @file watchdogs.c
 * @brief CAN message watchdog implementation
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#include "watchdogs.h"
#include "faults.h"
#include <stdio.h>
#include <string.h>


static StaticTimer_t  wd_buffers[MAX_WD_TIMERS];
static TimerHandle_t  wd_timers[MAX_WD_TIMERS];
static bool           wd_alive[MAX_WD_TIMERS];
static int            wd_count = 0;

static void wd_callback(TimerHandle_t xTimer) {
    int idx = (int)(uintptr_t)pvTimerGetTimerID(xTimer);
    wd_alive[idx] = false;
    printf("WD timeout: signal %d\n", idx);
    Faults_ThrowFault(wd_fault_ids[idx]);
}

void watchdog_init(void) {
    memset(wd_timers, 0, sizeof(wd_timers));
    memset(wd_alive,  0, sizeof(wd_alive));
    wd_count = 0;

}

void watchdog_create(const char *name, uint8_t idx, uint32_t timeout_ms, FaultID_e fault_id) {
    configASSERT(wd_count < MAX_WD_TIMERS);
    configASSERT(idx < MAX_WD_TIMERS);

    TimerHandle_t t = xTimerCreateStatic(
        name,
        pdMS_TO_TICKS(timeout_ms),
        pdFALSE, // one-shot
        (void *)(uintptr_t)idx,
        wd_callback,
        &wd_buffers[wd_count]
    );
    configASSERT(t != NULL);
    wd_timers[wd_count] = t;
    wd_alive[idx] = true;
    wd_fault_ids[idx] = fault_id;
    wd_count++;
}

void watchdog_start_all(void) {
    for (int i = 0; i < wd_count; i++) {
        configASSERT(xTimerStart(wd_timers[i], portMAX_DELAY) == pdPASS);
    }
}

void watchdog_stop_all(void) {
    for (int i = 0; i < wd_count; i++) {
        xTimerStop(wd_timers[i], portMAX_DELAY);
    }
}

void watchdog_received_can_message(int idx) {
    configASSERT(idx < MAX_WD_TIMERS && wd_timers[idx] != NULL);
    wd_alive[idx] = true;
    xTimerReset(wd_timers[idx], 0);
}

void watchdog_received_can_message_ISR(int idx, BaseType_t *pxHigherPriorityTaskWoken) {
    configASSERT(idx < MAX_WD_TIMERS && wd_timers[idx] != NULL);
    wd_alive[idx] = true;
    xTimerResetFromISR(wd_timers[idx], pxHigherPriorityTaskWoken);
}

bool watchdog_is_alive(int idx) {
    if (idx < 0 || idx >= MAX_WD_TIMERS) return false;
    return wd_alive[idx];
}

int watchdog_count(void) { return wd_count; }
