#include "FreeRTOS.h"
#include "stm32xx_hal.h"

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

void MX_UART_INIT(UART_HandleTypeDef *uartHandle);

void HAL_ADC_MspInit(ADC_HandleTypeDef *adcHandle);
