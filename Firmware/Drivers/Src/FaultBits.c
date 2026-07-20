#include "FaultBits.h"
#include <stdio.h>

static StaticEventGroup_t faultBuffer;
static EventGroupHandle_t faultGroup;

static StaticEventGroup_t warningBuffer;
static EventGroupHandle_t warningGroup;

const char *fault_names[FAULT_ID_COUNT] = {
#define X(name, persist) [FAULT_ID_##name] = #name,
    FAULT_ID_LIST(X)
#undef X
};

// Mandatory per-fault persistence threshold, from FAULT_ID_LIST.
static const uint16_t fault_persist_threshold[FAULT_ID_COUNT] = {
#define X(name, persist) [FAULT_ID_##name] = (persist),
    FAULT_ID_LIST(X)
#undef X
};

#define X(name, persist) \
    _Static_assert((persist) > 0, #name " fault persistence count must be > 0");
FAULT_ID_LIST(X)
#undef X

// Per-fault count of faults_set()/faults_set_mask() reports since the last
// faults_clear(). Not reset by anything else, so repeated reports across
// separate call sites/tasks all count toward the same fault's persistence.
static uint16_t fault_persist_counter[FAULT_ID_COUNT] = {0};

// Increments the counter for id (if not already at threshold) and returns
// true once persist_count has been reached. Caller holds a critical section.
static bool FB_report_locked(FaultID_e id) {
    if (fault_persist_counter[id] < fault_persist_threshold[id]) {
        fault_persist_counter[id]++;
    }
    return fault_persist_counter[id] >= fault_persist_threshold[id];
}

const char *warning_names[WARNING_ID_COUNT] = {
#define X(name) [WARNING_ID_##name] = #name,
    WARNING_ID_LIST(X)
#undef X
};

bool faults_init(void) {
    faultGroup = xEventGroupCreateStatic(&faultBuffer);
    if (faultGroup == NULL) {
        return false;
    }
    xEventGroupClearBits(faultGroup, FAULT_MASK_ALL);

    warningGroup = xEventGroupCreateStatic(&warningBuffer);
    if (warningGroup == NULL) {
        return false;
    }
    xEventGroupClearBits(warningGroup, WARNING_MASK_ALL);

    return true;
}

void faults_set(FaultID_e id) {
    configASSERT(id < FAULT_ID_COUNT);

    taskENTER_CRITICAL();
    bool reached = FB_report_locked(id);
    taskEXIT_CRITICAL();

    if (reached) {
        xEventGroupSetBits(faultGroup, FAULT_BIT(id));
    }
}

void faults_set_from_isr(FaultID_e id) {
    BaseType_t hpw = pdFALSE;
    configASSERT(id < FAULT_ID_COUNT);

    UBaseType_t saved = taskENTER_CRITICAL_FROM_ISR();
    bool reached = FB_report_locked(id);
    taskEXIT_CRITICAL_FROM_ISR(saved);

    if (reached) {
        xEventGroupSetBitsFromISR(faultGroup, FAULT_BIT(id), &hpw);
        portYIELD_FROM_ISR(hpw);
    }
}

void faults_set_mask(EventBits_t mask) {
    mask &= FAULT_MASK_ALL;
    EventBits_t to_latch = 0;

    taskENTER_CRITICAL();
    for (FaultID_e id = 0; id < FAULT_ID_COUNT; id++) {
        if ((mask & FAULT_BIT(id)) && FB_report_locked(id)) {
            to_latch |= FAULT_BIT(id);
        }
    }
    taskEXIT_CRITICAL();

    if (to_latch) {
        xEventGroupSetBits(faultGroup, to_latch);
    }
}

void faults_clear(FaultID_e id) {
    configASSERT(id < FAULT_ID_COUNT);

    taskENTER_CRITICAL();
    fault_persist_counter[id] = 0;
    taskEXIT_CRITICAL();

    xEventGroupClearBits(faultGroup, FAULT_BIT(id));
}

bool faults_any_active(void) {
    return (xEventGroupGetBits(faultGroup) & FAULT_MASK_ALL) != 0;
}

bool faults_is_active(FaultID_e id) {
    configASSERT(id < FAULT_ID_COUNT);
    return (xEventGroupGetBits(faultGroup) & FAULT_BIT(id)) != 0;
}

EventBits_t faults_get(void) {
    return xEventGroupGetBits(faultGroup) & FAULT_MASK_ALL;
}

EventBits_t faults_wait(FaultID_e id, TickType_t ticks) {
    EventBits_t mask = (id == FAULT_ID_COUNT) ? FAULT_MASK_ALL : FAULT_BIT(id);

    return xEventGroupWaitBits(
        faultGroup,
        mask,
        pdFALSE,    // don't clear
        pdFALSE,    // wait for any
        ticks
    );
}

void warning_set(WarningID_e id) {
    configASSERT(id < WARNING_ID_COUNT);
    xEventGroupSetBits(warningGroup, 1 << id);
}

void warning_clear(WarningID_e id) {
    configASSERT(id < WARNING_ID_COUNT);
    xEventGroupClearBits(warningGroup, 1 << id);
}

EventBits_t warning_get(void) {
    return xEventGroupGetBits(warningGroup) & WARNING_MASK_ALL;
}

bool warning_is_active(WarningID_e id) {
    configASSERT(id < WARNING_ID_COUNT);
    return (xEventGroupGetBits(warningGroup) & (1 << id)) != 0;
}