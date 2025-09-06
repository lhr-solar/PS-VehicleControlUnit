#pragma once
#ifndef COMMON_H
#define COMMON_H

#include "stm32xx_hal.h"
#include "CAN.h"
#include <stdint.h>

// Delay between each fault CAN message (200ms)
#define FAULT_MESSAGE_DELAY pdMS_TO_TICKS(200);

// bitmap of current fault state
extern uint32_t fault_bitmap;

// Fault state bitmap enum
typedef enum {
    FAULT_NONE = 0,
    FAULT_MOTOR_PRECHARGE_SENSE = 1 << 3,
    FAULT_MOTOR_PRECHARGE_TIMEOUT = 1 << 4,
    FAULT_ARRAY_PRECHARGE_SENSE = 1 << 8,
    FAULT_ARRAY_PRECHARGE_TIMEOUT = 1 << 9,
    FAULT_BPS = 1 << 10,
    FAULT_CONTROLS = 1 << 11,
    FAULT_MOTOR = 1 << 12
} fault_state_t;

typedef enum State_e {OFF = 0, ON} State;

typedef struct {
    uint32_t port;
    uint16_t pin;
} GPIO_Pin_t;

// Define a struct type for configs
struct VCUConfig {
    uint32_t mask;
    uint32_t can_id;
};

void gpio_clock_enable(uint32_t port);

void sys_clock_config(void);

void can_filter_config(CAN_FilterTypeDef* filter);
void can1_config(void);

void error_handler(void);
void success_handler(void);

/**
 * @brief   Reads fault bitmap - sets status LEDs and sends CAN message based on the fault type
 */
void fault_handler(void);

#endif