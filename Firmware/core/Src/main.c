#include "inits.h"
#include "UART.h"
#include "InitTask.h"

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  LEDs_init();

  xTaskCreateStatic(
      Task_Init,                // Task function
      "Init",                   // Name of the task (for debugging)
      configMINIMAL_STACK_SIZE, // Stack size in words
      NULL,                     // Task input parameter
      tskIDLE_PRIORITY + 1,     // Task priority
      Init_Task_Stack,          // Task handle
      &Init_Task_Buffer         // Static task buffer (optional)
  );

  vTaskStartScheduler();

  Error_Handler();

  /* Infinite loop */
  while (1)
  {
  }

  return 0;
}
