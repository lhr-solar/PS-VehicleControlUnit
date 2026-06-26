#include "inits.h"
#include "UART_Init.h"
#include "UART.h"
#include "InitTask.h"

// Opt into Embedded-Sharepoint bootloader_lite (see BootloaderTask.h).
#define USE_BOOTLOADER
#define BOOTLOADER_ON_HARDFAULT
#include "BootloaderTask.h"

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  // Enable bootloader_lite: USART3 "BOOT" listener + HardFault -> ROM bootloader.
  BOOTLOADER_LITE_SETUP(husart3);

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
