#include "UART_Init.h"

// // Initializes the UART specified
// void MX_UART_INIT(UART_HandleTypeDef *uartHandle)
// {

//   GPIO_InitTypeDef GPIO_InitStruct = {0};
//   RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

//   if (uartHandle->Instance == USART3)
//   {
//     /** Initializes the peripherals clocks
//      */
//     PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
//     PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
//     if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
//     {
//       Error_Handler();
//     }

//     /* USART3 clock enable */
//     __HAL_RCC_USART3_CLK_ENABLE();

//     __HAL_RCC_GPIOC_CLK_ENABLE();
//     /**USART3 GPIO Configuration
//     PC10     ------> USART3_TX
//     PC11     ------> USART3_RX
//     */
//     GPIO_InitStruct.Pin = USART3_TX_PIN | USART3_RX_PIN;
//     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
//     GPIO_InitStruct.Pull = GPIO_NOPULL;
//     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//     GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
//     HAL_GPIO_Init(USART3_PORT, &GPIO_InitStruct);

//     if (HAL_UART_Init(husart3) != HAL_OK)
//     {
//       Error_Handler();
//     }
//     if (HAL_UARTEx_SetTxFifoThreshold(husart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
//     {
//       Error_Handler();
//     }
//     if (HAL_UARTEx_SetRxFifoThreshold(husart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
//     {
//       Error_Handler();
//     }
//     if (HAL_UARTEx_DisableFifoMode(husart3) != HAL_OK)
//     {
//       Error_Handler();
//     }
//   }
// }


void HAL_UART_MspGPIOInit(UART_HandleTypeDef *huart){
    GPIO_InitTypeDef init = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* enable port A USART2 gpio
    PA2 -> USART2_TX
    PA3 -> USART2_RX
    */
    init.Pin = GPIO_PIN_10|GPIO_PIN_11;
    init.Mode = GPIO_MODE_AF_PP;
    init.Pull = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    init.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOC, &init);
}

void Init_UART_Printf()
{
    husart3->Init.BaudRate = 115200;
    husart3->Init.WordLength = UART_WORDLENGTH_8B;
    husart3->Init.StopBits = UART_STOPBITS_1;
    husart3->Init.Parity = UART_PARITY_NONE;
    husart3->Init.Mode = UART_MODE_TX_RX;
    husart3->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    husart3->Init.OverSampling = UART_OVERSAMPLING_16;
    
    printf_init(husart3);
}

// int _write(int file, char *ptr, int len) {
//     (void)file;
//     if (husart3 == NULL) return -1;
//     if (HAL_UART_Transmit(husart3, (uint8_t*)ptr, (uint16_t)len, HAL_MAX_DELAY) != HAL_OK) {
//         return -1;
//     }
//     return len;
// }
