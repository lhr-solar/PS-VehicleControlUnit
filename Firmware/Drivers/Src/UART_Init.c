#include "UART_Init.h"

void Init_UART_Printf() {
    husart3->Instance = USART3;
    husart3->Init.BaudRate = 115200;
    husart3->Init.WordLength = UART_WORDLENGTH_8B;
    husart3->Init.StopBits = UART_STOPBITS_1;
    husart3->Init.Parity = UART_PARITY_NONE;
    husart3->Init.Mode = UART_MODE_TX_RX;
    husart3->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    husart3->Init.OverSampling = UART_OVERSAMPLING_16;
    husart3->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    husart3->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    husart3->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    printf_init(husart3);
}

void HAL_UART_MspGPIOInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // for the ESP32
    if (huart->Instance == LPUART1) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**LPUART1 GPIO Configuration
        PB10     ------> LPUART1_RX
        PB11     ------> LPUART1_TX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    // for printf
    if (huart->Instance == USART3) {

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**USART3 GPIO Configuration
        PC10     ------> USART3_TX
        PC11     ------> USART3_RX
        */
        GPIO_InitStruct.Pin = USART3_TX_PIN | USART3_RX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(USART3_PORT, &GPIO_InitStruct);
    }
    
    setvbuf(stdout, NULL, _IONBF, 0);
    printf_init(husart3);
}