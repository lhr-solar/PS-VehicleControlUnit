/**
 * @file DumbBootloaderTask.c
 * @brief Minimal "dumb" bootloader trigger (see DumbBootloaderTask.h).
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#include "DumbBootloaderTask.h"

#include "stm32xx_hal.h"
#include "UART.h"
#include "printf.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* STM32G4 system memory (built-in ROM bootloader) base address. See RM0440. */
#define DUMB_BOOTLOADER_SYSTEM_MEMORY_BASE 0x1FFF0000UL

/* Magic command, matched as a rolling substring on the RX byte stream. */
static const char DUMB_BOOTLOADER_COMMAND[] = "BOOT";

#define DUMB_BOOTLOADER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 2U)

static StaticTask_t s_dumb_bootloader_task_buf;
static StackType_t s_dumb_bootloader_task_stack[DUMB_BOOTLOADER_TASK_STACK_SIZE];

static size_t s_match_len;

/**
 * @brief Feed one received byte to the rolling matcher.
 * @return true when the full magic command has just been matched.
 */
static bool dumb_bootloader_feed_byte(uint8_t byte) {
    const size_t command_len = sizeof(DUMB_BOOTLOADER_COMMAND) - 1U;

    if (byte == (uint8_t)DUMB_BOOTLOADER_COMMAND[s_match_len]) {
        s_match_len++;
        if (s_match_len == command_len) {
            s_match_len = 0U;
            return true;
        }
        return false;
    }

    /* Mismatch: restart, but let this byte start a fresh match (e.g. "BBOOT"). */
    s_match_len = (byte == (uint8_t)DUMB_BOOTLOADER_COMMAND[0]) ? 1U : 0U;
    return false;
}

void dumb_bootloader_enter_rom(void) {
    /* Read the bootloader's initial SP and entry point before tearing anything down. */
    const uint32_t boot_msp = *(volatile uint32_t *)(DUMB_BOOTLOADER_SYSTEM_MEMORY_BASE);
    void (*const boot_entry)(void) =
        (void (*)(void))(*(volatile uint32_t *)(DUMB_BOOTLOADER_SYSTEM_MEMORY_BASE + 4U));

    __disable_irq();

    /* Stop the RTOS tick so no SysTick exception fires mid-jump. */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    /* Disable and clear every maskable interrupt. */
    for (uint32_t i = 0U; i < (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    /* Return clocks/peripherals to reset state so the ROM starts clean. */
    HAL_RCC_DeInit();
    HAL_DeInit();

    /* Point the vector table at system memory, then move to MSP (we may be on PSP). */
    SCB->VTOR = DUMB_BOOTLOADER_SYSTEM_MEMORY_BASE;
    __DSB();
    __ISB();

    __set_MSP(boot_msp);
    __set_CONTROL(0U); /* privileged, use MSP */
    __ISB();

    __enable_irq();

    boot_entry();

    for (;;) {
        /* Never reached. */
    }
}

static void Task_DumbBootloader(void *pvParameters) {
    (void)pvParameters;

    uint8_t byte = 0U;
    for (;;) {
        if (uart_recv(husart3, &byte, 1U, portMAX_DELAY) != UART_OK) {
            continue;
        }
        if (dumb_bootloader_feed_byte(byte)) {
            printf("\r\nBOOT received: entering ROM bootloader...\r\n");
            /* Let the interrupt-driven UART TX queue drain before clocks go down. */
            vTaskDelay(pdMS_TO_TICKS(50));
            dumb_bootloader_enter_rom();
        }
    }
}

void DumbBootloaderTask_Init(void) {
    s_match_len = 0U;
    (void)xTaskCreateStatic(
        Task_DumbBootloader,
        "DumbBoot",
        DUMB_BOOTLOADER_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1U,
        s_dumb_bootloader_task_stack,
        &s_dumb_bootloader_task_buf);
}
