#pragma once

#include "pinDefs.h"
#include "printf.h"

// Overiding the USART tx queue size to be bigger than the defaukt
#define USART3_TX_QUEUE_SIZE 100

/**
 * @brief USART Initialization Function
 * @param uartHandle Pointer to UART handle struct
 * @retval None
 */
void MX_UART_INIT(UART_HandleTypeDef *uartHandle);

/**
 * @brief Printf Initialization Function
 * @param None
 * @retval None
 */
void Init_UART_Printf();