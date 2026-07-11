#include "MotorSafeBits.h"

EventGroupHandle_t motorSafeBits;
StaticEventGroup_t motorSafeBitsBuffer;

bool MotorSafeBits_Init() {
    motorSafeBits = xEventGroupCreateStatic(&motorSafeBitsBuffer);
    if (motorSafeBits == NULL) {
        return false;
    }
    return true;
}

void set_MotorSafeBit(motor_status_bit_t bit) {
    // not a valid fault
    if (bit >= NUM_MOTOR_STATUS_BITS || motorSafeBits == NULL) {
        return;
    }

    // set the bit
    xEventGroupSetBits(motorSafeBits, MOTOR_STATUS_BIT(bit));
}

void clear_MotorSafeBit(motor_status_bit_t bit) {
    // not a valid fault
    if (bit >= NUM_MOTOR_STATUS_BITS || motorSafeBits == NULL) {
        return;
    }

    // set the bit
    xEventGroupClearBits(motorSafeBits, MOTOR_STATUS_BIT(bit));
}

EventBits_t MotorSafeBits_WaitMask(EventBits_t bitsToWait, TickType_t delay_ticks) {
    if (motorSafeBits == NULL) {
        return 0;
    }
    EventBits_t pending = xEventGroupWaitBits(motorSafeBits,
                                              bitsToWait, // wait for the given bits to be set
                                              pdFALSE,    // fault bits are not reset
                                              pdTRUE,     // wait for all bits to be set
                                              delay_ticks);

    return pending;
}

EventBits_t MotorSafeBits_Wait(motor_status_bit_t motor_status_bit, TickType_t delay_ticks) {
    if (motorSafeBits == NULL) {
        return 0;
    }
    EventBits_t bitsToWaitFor = MOTOR_STATUS_BIT(motor_status_bit);
    EventBits_t pending = xEventGroupWaitBits(motorSafeBits,
                                              bitsToWaitFor, // wait for the given bits to be set
                                              pdFALSE,       // fault bits are not reset
                                              pdTRUE,        // wait for all bits to be set
                                              delay_ticks);

    return pending;
}