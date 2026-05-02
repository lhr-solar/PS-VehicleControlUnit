#pragma once

#if defined(FIRMWARE_USES_BOOTLOADER)

/** Creates a low-priority task that services the ESBLT UART bootloader command on USART3. */
void BootloaderUartTask_Init(void);

#endif
