#include "inits.h"
#include "UART_Init.h"
#include "UART.h"
#include "InitTask.h"
#include "bootloader_bringup.h"
#include "StatusLEDs.h"
#include "uart_bootloader.h"

int main(void)
{
  uart_bootloader_init_app_vector_table();
#if defined(VCU_BOOTLOADER_BRINGUP)
#if (VCU_BOOTLOADER_BRINGUP_LEVEL == 0)
  VCU_BootloaderBringup_EarlyIndicator();
#endif
  VCU_BootloaderBringup_Run();
#endif

#if !defined(VCU_BOOTLOADER_BRINGUP)
  HAL_Init();

  SystemClock_Config();

  LEDs_init();
#if defined(VCU_BOOTLOADER_FULL_DIAG)
  LED_set(PRECHARGE_COMPLETE, LED_ON);
#endif

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
#endif

  return 0;
}
