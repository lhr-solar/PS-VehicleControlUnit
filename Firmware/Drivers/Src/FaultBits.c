// #include "FaultBits.h"

// // Event group handle to store fault state bits
// EventGroupHandle_t faultStateBits;

// // Static buffer to store the event handle
// StaticEventGroup_t faultStateBitsBuffer;

// uint8_t faultBits_init(void) {
//     faultStateBits = xEventGroupCreateStatic(&faultStateBitsBuffer);
//     if (faultStateBits == NULL) {
//         return 0;
//     }
//     return 1;
// }

// void set_faultBit(fault_bit_t bit) {
//     // not a valid fault
//     if (bit >= NUM_FAULTS) {
//         return;
//     }

//     // chat we're cooked
//     xEventGroupSetBits(faultStateBits, FAULT_BIT(bit));
//     // should never return from here
//     taskYIELD();
// }

// void set_faultBitFromISR(fault_bit_t bit) {
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;

//     if (bit >= NUM_FAULTS) {
//         return;
//     }

//     xEventGroupSetBitsFromISR(faultStateBits, FAULT_BIT(bit), &xHigherPriorityTaskWoken);

//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// }

// EventBits_t faultBit_wait(fault_bit_t bit, TickType_t xTicksToWait) {
//     // NUM_FAULTS indiciates you want to wait for all bits
//     if (bit > NUM_FAULTS) {
//         return 0;
//     }

//     // if NUM
//     // EventBits_t uxBitsToWaitFor = bit == NUM_FAULTS ? ALL_FAULT_BITS : (FAULT_BIT(bit));
//     EventBits_t uxBitsToWaitFor = bit == NUM_FAULTS ? FAULT_BITMASK : (FAULT_BIT(bit));

//     EventBits_t pending = xEventGroupWaitBits(faultStateBits,
//                                               uxBitsToWaitFor, // wait for any defined fault
//                                               pdFALSE,         // fault bits are not reset
//                                               pdFALSE,         // wait for ANY bit to be set
//                                               xTicksToWait);
//     return pending;
// }


#include "FaultBits.h"
#include <stdio.h>

static StaticEventGroup_t faultBuffer;
static EventGroupHandle_t faultGroup;

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
}