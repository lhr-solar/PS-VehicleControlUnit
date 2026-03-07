#pragma once

#include "FreeRTOS.h"
#include "stm32xx_hal.h"
#include "inits.h"
#include <event_groups.h>
#include <stdint.h>

// The max number of fault bits is dependent on the configUSE_16_BIT_TICKS defined in FreeRTOS.h
#if ( configUSE_16_BIT_TICKS == 0 )
    #define MAX_MOTOR_SAFE_BITS   24U
#else
    #define MAX_MOTOR_SAFE_BITS   8U
#endif

typedef enum
{
    BPS_SAFE,                           // BPS is clear and high voltage is on
    PEDALS_READING_ACCELERATOR,
    PEDALS_READING_BRAKE,
    MOTOR_CONTACTOR_ENABLED,            // The Motor Contactor is enabled
    MOTOR_PRECHARGE_CONTACTOR_ENABLED,   // The Motor precharge Contactor is enabled
    NUM_MOTOR_STATUS_BITS
} motor_status_bit_t;

_Static_assert(NUM_MOTOR_STATUS_BITS <= MAX_MOTOR_SAFE_BITS, "Too many motor safe bits for EventGroup");

/* Convert enum to bitmask */
#define MOTOR_STATUS_BIT(motorBit)   (1UL << (motorBit))

static const EventBits_t motorSafeToRunBits = MOTOR_STATUS_BIT(MOTOR_CONTACTOR_ENABLED) 
                                    || MOTOR_STATUS_BIT(MOTOR_PRECHARGE_CONTACTOR_ENABLED);

BaseType_t MotorSafeBits_Init();

void set_MotorSafeBit(motor_status_bit_t bit);

void clear_MotorSafeBit(motor_status_bit_t bit);

EventBits_t MotorSafeBits_Wait(EventBits_t bitsToWaitFor, TickType_t delay_ticks);