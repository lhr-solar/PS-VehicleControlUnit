#pragma once

#include "common.h"
#include "stm32xx_hal.h"

//--------------------------------------------------------------------------------
// Pin configuration for LEDs and heartbeat
// fix the pin numbers later
#define HEARTBEAT_PORT     GPIOB
#define HEARTBEAT_PIN      GPIO_PIN_3

#define PSOM_LED1_PORT     GPIOA
#define PSOM_LED1_PIN      GPIO_PIN_7
//--------------------------------------------------------------------------------
