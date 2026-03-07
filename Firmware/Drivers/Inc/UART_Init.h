#pragma once

#include "pinDefs.h"
#include "printf.h"

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


//Redirects syscall write to a blocking write so UART prints fully
// int _write(int file, char *ptr, int len);