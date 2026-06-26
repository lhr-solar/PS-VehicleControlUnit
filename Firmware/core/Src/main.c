#include "inits.h"
#include "UART_Init.h"
#include "UART.h"
#include "InitTask.h"
#include "DumbBootloaderTask.h"

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  // If a bootloader entry was requested (UART "BOOT" or a hardfault), a
  // backup-register flag survives the reset; jump now from a clean context.
  if (dumb_bootloader_consume_request()) {
    dumb_bootloader_enter_rom();
  }
  // From here on, route HardFault/MemManage/BusFault/UsageFault to the ROM bootloader.
  dumb_bootloader_install_fault_vectors();

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
