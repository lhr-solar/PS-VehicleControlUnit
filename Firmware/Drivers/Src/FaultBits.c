#include "FaultBits.h"
#include <stdio.h>

static StaticEventGroup_t faultBuffer;
static EventGroupHandle_t faultGroup;

static StaticEventGroup_t warningBuffer;
static EventGroupHandle_t warningGroup;

const char *fault_names[FAULT_ID_COUNT] = {
#define X(name) [FAULT_ID_##name] = #name,
    FAULT_ID_LIST(X)
#undef X
};

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
    xEventGroupSetBits(faultGroup, FAULT_BIT(id));
}

void faults_set_from_isr(FaultID_e id) {
    BaseType_t hpw = pdFALSE;
    configASSERT(id < FAULT_ID_COUNT);
    xEventGroupSetBitsFromISR(faultGroup, FAULT_BIT(id), &hpw);
    portYIELD_FROM_ISR(hpw);
}

void faults_set_mask(EventBits_t mask) {
    xEventGroupSetBits(faultGroup, mask & FAULT_MASK_ALL);
}

void faults_clear(FaultID_e id) {
    configASSERT(id < FAULT_ID_COUNT);
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
    configASSERT(id < FAULT_ID_COUNT);
    xEventGroupSetBits(warningGroup, FAULT_BIT(id));
}

EventBits_t warning_get(void) {
    return xEventGroupGetBits(warningGroup) & WARNING_MASK_ALL;
}