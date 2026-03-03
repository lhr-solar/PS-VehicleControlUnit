#include "MotorSafeBits.h"

// Event group handle to store fault state bits
EventGroupHandle_t motorSafeBits;

// Static buffer to store the event handle
StaticEventGroup_t motorSafeBitsBuffer;


BaseType_t MotorSafeBits_Init(){

    motorSafeBits = xEventGroupCreateStatic( &motorSafeBitsBuffer );
    if(motorSafeBits == NULL){
        return pdFALSE;
    }
    return pdTRUE;
}

void set_MotorSafeBit(motor_status_bit_t bit){
    // not a valid fault
    if(bit >= NUM_MOTOR_STATUS_BITS){ 
        return;
    }
    
    // set the bit
    xEventGroupSetBits(motorSafeBits, MOTOR_STATUS_BIT(bit));
}

EventBits_t MotorSafeBits_Wait(uint32_t bitsToWaitFor, TickType_t delay_ticks){
    
    EventBits_t pending = xEventGroupWaitBits(
        motorSafeBits,
        bitsToWaitFor,      // wait for any defined fault
        pdFALSE,            // fault bits are not reset
        pdTRUE,             // wait for all bits to be set
        delay_ticks 
    );
    
    return pending;
}