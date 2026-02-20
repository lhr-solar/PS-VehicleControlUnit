#include "FreeRTOS.h"
#include "stm32xx_hal.h"
#include "ADC_Sense.h"
#include "inits.h"

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
  MX_UART_INIT(huart3);
  MX_USART3_UART_Init();

  /* Infinite loop */
  while (1)
  {
    
  }
}
