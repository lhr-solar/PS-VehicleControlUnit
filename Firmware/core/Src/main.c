#include "inits.h"
#include "UART_Init.h"
#include "UART.h"
#include "InitTask.h"
#include "bootloader_lite.h"

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  // Route HardFault/MemManage/BusFault/UsageFault to the ROM bootloader (bl-lite).
  bootloader_lite_init();

  LED_init();

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


  /* Infinite loop */
  while (1)
  {
  }

  return 0;
}
