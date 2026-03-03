#pragma once

#include "pinDefs.h"
#include "printf.h"
#include "FreeRTOS.h"

void SystemClock_Config(void);

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
void MX_GPIO_Init(void);

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
void MX_USART3_UART_Init(void);

/**
 * @brief USART Initialization Function
 * @param uartHandle Pointer to UART handle struct
 * @retval None
 */
void MX_UART_INIT(UART_HandleTypeDef *uartHandle);

/**
 * @brief ADC Initialization Function
 * @param adcHandle Pointer to ADC handle struct
 * @retval None
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *adcHandle);

/**
 * @brief Printf Initialization Function
 * @param None
 * @retval None
 */
void Init_UART_Printf();

/**
 * @brief Does nothing at the moment
 * @param None
 * @retval None
 */
void Error_Handler(void);