#pragma once
#include "FreeRTOS.h"
#include "stm32xx_hal.h"
#include "UART.h"

uart_status_t ESP32_UART_Init(void);

void ESP32_mspUartInit(UART_HandleTypeDef* huart);

uart_status_t ESP32_Send(const uint8_t* data, uint8_t length, TickType_t delay_ticks);

uart_status_t ESP32_Recv(uint8_t* data, uint8_t length, TickType_t delay_ticks);
