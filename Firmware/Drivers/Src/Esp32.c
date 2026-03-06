#include "ESP32.h"


UART_HandleTypeDef* esp32Uart;

uart_status_t ESP32_UART_Init(void){
    esp32Uart = hlpuart1;
    esp32Uart->Instance = LPUART1;

    esp32Uart->Init.BaudRate = 115200;
    esp32Uart->Init.WordLength = UART_WORDLENGTH_8B;
    esp32Uart->Init.StopBits = UART_STOPBITS_1;
    esp32Uart->Init.Parity = UART_PARITY_NONE;
    esp32Uart->Init.Mode = UART_MODE_TX_RX;
    esp32Uart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    esp32Uart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    esp32Uart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    esp32Uart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    uart_status_t status = uart_init(hlpuart1);
    if (status != UART_OK) {
        return UART_ERR;
    }
    return UART_OK;
}

void ESP32_mspUartInit(UART_HandleTypeDef* huart){
    GPIO_InitTypeDef GPIO_InitStruct = {0}; 
    if(huart->Instance == LPUART1){
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**LPUART1 GPIO Configuration
        PB10     ------> LPUART1_RX
        PB11     ------> LPUART1_TX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

uart_status_t ESP32_Send(const uint8_t* data, uint8_t length, TickType_t delay_ticks){
    return uart_send(esp32Uart, data, length, delay_ticks);
}

uart_status_t ESP32_Recv(uint8_t* data, uint8_t length, TickType_t delay_ticks){
    return uart_recv(esp32Uart, data, length, delay_ticks);
}