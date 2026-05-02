#if defined(FIRMWARE_USES_BOOTLOADER)

#include "BootloaderUartTask.h"
#include "UART.h"
#include "FreeRTOS.h"
#include "task.h"

#define BOOTLOADER_UART_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE)

static StaticTask_t s_bootloader_uart_task_buf;
static StackType_t s_bootloader_uart_task_stack[BOOTLOADER_UART_TASK_STACK_SIZE];

static void Task_BootloaderUart(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        (void)uart_bootloader_service(husart3, portMAX_DELAY);
    }
}

void BootloaderUartTask_Init(void) {
    (void)xTaskCreateStatic(
        Task_BootloaderUart,
        "BootloaderUart",
        BOOTLOADER_UART_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        s_bootloader_uart_task_stack,
        &s_bootloader_uart_task_buf);
}

#endif /* FIRMWARE_USES_BOOTLOADER */
