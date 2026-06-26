/**
 * @file DumbBootloaderTask.c
 * @brief Thin USART3 listener for bootloader_lite (see header).
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#include "DumbBootloaderTask.h"

#include "bootloader_lite.h"
#include "UART.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>

#define DUMB_BOOTLOADER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 2U)

static StaticTask_t s_dumb_bootloader_task_buf;
static StackType_t s_dumb_bootloader_task_stack[DUMB_BOOTLOADER_TASK_STACK_SIZE];

static void Task_DumbBootloader(void *pvParameters) {
    (void)pvParameters;

    uint8_t byte = 0U;
    for (;;) {
        if (uart_recv(husart3, &byte, 1U, portMAX_DELAY) != UART_OK) {
            continue;
        }
        if (bootloader_lite_feed_byte(byte)) {
            /* Acknowledge so the host knows we heard it... */
            (void)uart_send(husart3, (const uint8_t *)BOOTLOADER_LITE_ACK,
                            (uint16_t)(sizeof(BOOTLOADER_LITE_ACK) - 1U), pdMS_TO_TICKS(200));
            /* ...let the ACK drain, then jump into the ROM bootloader. */
            vTaskDelay(pdMS_TO_TICKS(100));
            bootloader_lite_enter_rom();
        }
    }
}

void DumbBootloaderTask_Init(void) {
    (void)xTaskCreateStatic(
        Task_DumbBootloader,
        "DumbBoot",
        DUMB_BOOTLOADER_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1U,
        s_dumb_bootloader_task_stack,
        &s_dumb_bootloader_task_buf);
}
