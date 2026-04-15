/**
 * @file Watchdogs.c
 * @brief CAN message watchdog implementation
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#include "Watchdogs.h"
#include "FaultBits.h"
#include <stdio.h>
#include <string.h>


#ifndef NODAWG
static StaticEventGroup_t dogBuffer;
static EventGroupHandle_t dogGroup; // 1 = alive, 0 = dead
static StaticTimer_t  wd_buffers[MAX_WD_TIMERS] = {0};
static TimerHandle_t  wd_timers[MAX_WD_TIMERS] = {0};
static FaultID_e      wd_fault_ids[MAX_WD_TIMERS] = {0};
#endif

static uint8_t        wd_count = 0;

#define WDOG_MASK_ALL       ((1UL << WD_IDX_COUNT) - 1)
#define WDOG_BIT(idx)       (1UL << idx)

#ifndef NODAWG
// Callback function for when a watchdog timer expires 
static void wd_callback(TimerHandle_t xTimer) {
    uint8_t idx = (uint8_t)(uintptr_t)pvTimerGetTimerID(xTimer);
    xEventGroupClearBits(dogGroup, WDOG_BIT(idx));
    printf("WD timeout: signal %d\r\n", idx);
    faults_set(wd_fault_ids[idx]);
}
#endif

void watchdog_init(void) {
#ifndef NODAWG
#define X(name, str, timeout, fault) \
    watchdog_create(str, WD_IDX_##name, timeout, fault);
    WATCHDOG_LIST(X)
#undef X
    dogGroup = xEventGroupCreateStatic(&dogBuffer);
    if (dogGroup == NULL) {
        return;
    }
    xEventGroupSetBits(dogGroup, WDOG_MASK_ALL);
#endif
}

void watchdog_create(const char *name, uint8_t idx, uint32_t timeout_ms, FaultID_e fault_id) {
#ifndef NODAWG

    configASSERT(wd_count < MAX_WD_TIMERS);
    configASSERT(idx < MAX_WD_TIMERS);

    TimerHandle_t t = xTimerCreateStatic(
        name,
        pdMS_TO_TICKS(timeout_ms),
        pdFALSE,            // one-shot
        (void *)(uintptr_t)idx,
        wd_callback,
        &wd_buffers[wd_count]
    );
    configASSERT(t != NULL);
    wd_timers[idx] = t;
    wd_fault_ids[idx] = fault_id;
    wd_count++;
#endif
}

void watchdog_start_all(void) {
#ifndef NODAWG
    for (uint8_t i = 0; i < wd_count; i++) {
        configASSERT(xTimerStart(wd_timers[i], portMAX_DELAY) == pdPASS);
    }
#endif
}

void watchdog_stop_all(void) {
#ifndef NODAWG
    for (uint8_t i = 0; i < wd_count; i++) {
        xTimerStop(wd_timers[i], portMAX_DELAY);
    }
#endif
}

void watchdog_received_can_message(uint8_t idx) {
#ifndef NODAWG
    configASSERT(idx < MAX_WD_TIMERS && wd_timers[idx] != NULL);
    xEventGroupSetBits(dogGroup, (0x1 << idx));
    xTimerReset(wd_timers[idx], 0);
#endif
}

void watchdog_received_can_message_ISR(uint8_t idx, BaseType_t *pxHigherPriorityTaskWoken) {
#ifndef NODAWG
    configASSERT(idx < MAX_WD_TIMERS && wd_timers[idx] != NULL);
    xEventGroupClearBitsFromISR(dogGroup, WDOG_BIT(idx));
    xTimerResetFromISR(wd_timers[idx], pxHigherPriorityTaskWoken);
#endif
}

bool watchdog_is_alive(int idx) {
#ifndef NODAWG
    if (idx < 0 || idx >= MAX_WD_TIMERS) return false;
    return ((xEventGroupGetBits(dogGroup) >> idx) & 0x1);
#else 
    return true;
#endif
}

uint8_t watchdog_count(void) { return wd_count; }
