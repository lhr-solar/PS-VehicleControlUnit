#pragma once

#include "FreeRTOS.h"
#include "inits.h"
#include "stm32xx_hal.h"
#include <event_groups.h>
#include <stdint.h>
#include <stdbool.h>

// The max number of fault bits is dependent on the configUSE_16_BIT_TICKS defined in FreeRTOS.h
#if (configUSE_16_BIT_TICKS == 0)
#define MAX_MOTOR_SAFE_BITS 24U
#else
#define MAX_MOTOR_SAFE_BITS 8U
#endif

typedef enum {
    BPS_SAFE = 0,                          // BPS is clear and high voltage is on
    PEDALS_READING_ACCELERATOR = 1,        // We're getting correct accelerator pedal messages
    PEDALS_READING_BRAKE = 2,              // We're getting correct brake pedal messages
    MOTOR_CONTACTOR_ENABLED = 3,           // The Motor contactor is enabled
    MOTOR_PRECHARGE_CONTACTOR_ENABLED = 4, // The Motor precharge Contactor is enabled
    DASHBOARD_IGNITION_MOTOR = 5,          // Ignition switch is set to motor
    NUM_MOTOR_STATUS_BITS = 6
} motor_status_bit_t;

_Static_assert(NUM_MOTOR_STATUS_BITS <= MAX_MOTOR_SAFE_BITS,
               "Too many motor safe bits for EventGroup");

/* Convert enum to bitmask */
#define MOTOR_STATUS_BIT(motorBit) (1UL << (motorBit))

BaseType_t MotorSafeBits_Init();

EventBits_t MotorSafeBits_WaitMask(EventBits_t bitsToWait, TickType_t delay_ticks);

void set_MotorSafeBit(motor_status_bit_t bit);

void clear_MotorSafeBit(motor_status_bit_t bit);

EventBits_t MotorSafeBits_Wait(motor_status_bit_t bitsToWaitFor, TickType_t delay_ticks);