/**
 * @file faults.c
 * @brief Fault management implementation
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#include "faults.h"
#include "fsm.h"
#include "watchdogs.h"
#include <stdio.h>

static const char *fault_names[FAULT_ID_COUNT] = {
#define X(name) [FAULT_ID_##name] = #name,
    FAULT_ID_LIST(X)
#undef X
};


static StaticEventGroup_t xFaultEventGroupBuffer;
EventGroupHandle_t        xFaultEventGroup = NULL;


void Faults_Init(void) {
    xFaultEventGroup = xEventGroupCreateStatic(&xFaultEventGroupBuffer);
    configASSERT(xFaultEventGroup != NULL);
    xEventGroupClearBits(xFaultEventGroup, FAULT_MASK_ALL);
}

void Faults_ThrowFault(FaultID_e fault_id) {
    configASSERT(fault_id < FAULT_ID_COUNT);
    xEventGroupSetBits(xFaultEventGroup, 1U << fault_id);
}

void Faults_ThrowFaultsUsingBitfield(EventBits_t fault_bits) {
    xEventGroupSetBits(xFaultEventGroup, fault_bits & FAULT_MASK_ALL);
}

// for if we want recoverable faults maybe??
void Faults_ClearFault(FaultID_e fault_id) {
    configASSERT(fault_id < FAULT_ID_COUNT);
    xEventGroupClearBits(xFaultEventGroup, 1U << fault_id);
}

bool Faults_AnyActive(void) {
    return (xEventGroupGetBits(xFaultEventGroup) & FAULT_MASK_ALL) != 0;
}

bool Faults_IsActive(FaultID_e fault_id) {
    configASSERT(fault_id < FAULT_ID_COUNT);
    return (xEventGroupGetBits(xFaultEventGroup) & (1U << fault_id)) != 0;
}

EventBits_t Faults_GetCurrentFaults(void) {
    return xEventGroupGetBits(xFaultEventGroup) & FAULT_MASK_ALL;
}



/**
 * Fault handler task
 * 
 * should be highest priority and blocks waiting for any fault bit
 * then responds immediately once a bit is set
 */
void Task_FaultHandler() {

    while (true) {
        // Block until any fault bit is set (do not clear on exit
        // faults are sticky until explicitly cleared)
        EventBits_t active = xEventGroupWaitBits(
            xFaultEventGroup,
            FAULT_MASK_ALL,
            pdFALSE,        // do not clear on exit
            pdFALSE,        // wait for any bit
            portMAX_DELAY
        );

        
        for (int i = 0; i < FAULT_ID_COUNT; i++) {
            if (active & (1U << i)) {
                printf("[FAULT] %s (bit %d) \n",
                       fault_names[i], i);
            }
        }

        // eventually maybe respond based on worst active severity
        bool has_critical = true;
        // for (int i = 0; i < FAULT_ID_COUNT; i++) {
        //     if ((active & (1U << i)) &&
        //         fault_severity[i] == FAULT_SEV_CRITICAL) {
        //         has_critical = true;
        //         break;
        //     }
        // }

        if (has_critical) {
            fsm_disable();
            watchdog_stop_all();
        } else {
            fsm_recover();
        }

        // yield briefly so lower-priority tasks can run before we
        // loop back and re-check the event group
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
