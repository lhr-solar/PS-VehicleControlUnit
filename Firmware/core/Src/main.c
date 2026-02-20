#include "FreeRTOS.h"
#include "stm32xx_hal.h"
#include "ADC_Sense.h"
#include "inits.h"

/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

UART_HandleTypeDef huart3;

/* Private function prototypes -----------------------------------------------*/

int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_USART3_UART_Init();

  /* Infinite loop */
  while (1)
  {
    
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}