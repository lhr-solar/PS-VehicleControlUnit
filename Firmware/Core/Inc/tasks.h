#pragma once
#include "FreeRTOS.h" /* Must come first. */
#include "stm32xx_hal.h"
#include "common.h"

// Task Priority 
#define TASK_INIT_PRIO                  tskIDLE_PRIORITY + 1

// Task Stack Size 
#define TASK_INIT_STACK_SIZE            configMINIMAL_STACK_SIZE

// Task Stack Arrays 

// Task Buffers

// Task Function Prototypes
void Task_Init();