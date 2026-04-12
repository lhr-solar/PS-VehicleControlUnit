#include "FaultBits.h"
#include <stdio.h>

static StaticEventGroup_t faultBuffer;
static EventGroupHandle_t faultGroup;

EventGroupHandle_t stateBits;
StaticEventGroup_t stateBitsBuffer;


const char *fault_names[FAULT_ID_COUNT] = {
#define X(name) [FAULT_ID_##name] = #name,
    FAULT_ID_LIST(X)
#undef X
};

bool faults_init(void) {
    faultGroup = xEventGroupCreateStatic(&faultBuffer);
    if (faultGroup == NULL) {
        return false;
    }
    xEventGroupClearBits(faultGroup, FAULT_MASK_ALL);
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
        pdFALSE,   // don't clear
        pdFALSE,   // wait for any
        ticks
    );

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

uint8_t stateBits_init(void){
    stateBits = xEventGroupCreateStatic( &stateBitsBuffer );
    if(stateBits == NULL){
        return 0;
    }
    return 1;
}

void set_stateBit(state_bit_t bit){
    // not a valid state    
    if(bit >= NUM_STATES){ 
        return;
    }

    // Clear all state bits first
    xEventGroupClearBits(stateBits, STATE_BITMASK);

    // Set only the requested bit
    xEventGroupSetBits(stateBits, STATE_BIT(bit));

    taskYIELD();
}

EventBits_t faultBit_wait(fault_bit_t bit, TickType_t xTicksToWait){

    // NUM_FAULTS indiciates you want to wait for all bits
    if(bit > NUM_FAULTS){
        return 0;
    }

    EventBits_t uxBitsToWaitFor = bit == NUM_FAULTS ? FAULT_BITMASK : (FAULT_BIT(bit));

    EventBits_t pending = xEventGroupWaitBits(
        faultStateBits,
        uxBitsToWaitFor,  // wait for any defined fault
        pdFALSE,          // fault bits are not reset
        pdFALSE,          // wait for ANY bit to be set
        xTicksToWait 
    );
    return pending;
}