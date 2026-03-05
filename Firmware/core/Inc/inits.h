#pragma once

#include "pinDefs.h"
#include "UART_Init.h"
#include "FreeRTOS.h"

void SystemClock_Config(void);

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
void MX_GPIO_Init(void);

/**
 * @brief Does nothing at the moment
 * @param None
 * @retval None
 */
void Error_Handler(void);