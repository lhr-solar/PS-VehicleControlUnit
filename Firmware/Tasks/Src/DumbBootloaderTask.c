/**
 * @file DumbBootloaderTask.c
 * @brief Minimal "dumb" bootloader trigger + hardfault recovery (see header).
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#include "DumbBootloaderTask.h"

#include "stm32xx_hal.h"
#include "UART.h"
#include "FreeRTOS.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* STM32G4 system memory (built-in ROM bootloader) base address. See RM0440. */
#define DUMB_BOOTLOADER_SYSTEM_MEMORY_BASE 0x1FFF0000UL

/* Backup-register flag: survives the soft reset between request and entry. */
#define DUMB_BOOTLOADER_BOOT_MAGIC 0xB00710ADUL

/* Magic command (rolling substring match) and the reply the host waits for. */
static const char DUMB_BOOTLOADER_COMMAND[] = "BOOT";
static const char DUMB_BOOTLOADER_ACK[] = "\r\nBOOTACK\r\n";

#define DUMB_BOOTLOADER_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE * 2U)

/* Vector table copy for the relocated (RAM) table. STM32G473 has < 112 IRQs;
 * 16 system exceptions + IRQs fit in 128 entries (512 bytes). VTOR must be
 * aligned to the table size rounded up to a power of two -> 512 bytes. */
#define DUMB_BOOTLOADER_VECTOR_COUNT 128U
#define EXC_HARDFAULT  3U
#define EXC_MEMMANAGE  4U
#define EXC_BUSFAULT   5U
#define EXC_USAGEFAULT 6U

static StaticTask_t s_dumb_bootloader_task_buf;
static StackType_t s_dumb_bootloader_task_stack[DUMB_BOOTLOADER_TASK_STACK_SIZE];

static uint32_t s_ram_vectors[DUMB_BOOTLOADER_VECTOR_COUNT] __attribute__((aligned(512)));

static size_t s_match_len;

/* ---- backup-register boot request --------------------------------------- */

static volatile uint32_t *dumb_bootloader_magic_reg(void) {
#if defined(TAMP)
    return &TAMP->BKP0R;
#elif defined(RTC)
    return &RTC->BKP0R;
#else
    return NULL;
#endif
}

static void dumb_bootloader_enable_backup_access(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
#if defined(__HAL_RCC_RTCAPB_CLK_ENABLE)
    __HAL_RCC_RTCAPB_CLK_ENABLE();
#endif
}

void dumb_bootloader_request_reset(void) {
    volatile uint32_t *reg = dumb_bootloader_magic_reg();
    if (reg != NULL) {
        dumb_bootloader_enable_backup_access();
        *reg = DUMB_BOOTLOADER_BOOT_MAGIC;
        __DSB();
    }
    NVIC_SystemReset();
    for (;;) {
        /* Never reached. */
    }
}

bool dumb_bootloader_consume_request(void) {
    volatile uint32_t *reg = dumb_bootloader_magic_reg();
    if (reg == NULL) {
        return false;
    }
    dumb_bootloader_enable_backup_access();
    if (*reg != DUMB_BOOTLOADER_BOOT_MAGIC) {
        return false;
    }
    *reg = 0U;
    __DSB();
    return true;
}

/* ---- jump into the ROM bootloader (clean thread-mode context) ------------ */

void dumb_bootloader_enter_rom(void) {
    /* Read the bootloader's initial SP and entry point before tearing down. */
    const uint32_t boot_msp = *(volatile uint32_t *)(DUMB_BOOTLOADER_SYSTEM_MEMORY_BASE);
    void (*const boot_entry)(void) =
        (void (*)(void))(*(volatile uint32_t *)(DUMB_BOOTLOADER_SYSTEM_MEMORY_BASE + 4U));

    /* Reset clocks/peripherals while the tick is still running (uses HAL timeouts). */
    HAL_RCC_DeInit();
    HAL_DeInit();

    __disable_irq();

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    for (uint32_t i = 0U; i < (sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0])); i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

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

/* ---- fault handling ------------------------------------------------------ */

/* Installed in the RAM vector table for HardFault/MemManage/BusFault/Usage.
 * We are in handler mode here, so we do NOT jump directly; instead request a
 * reset and let main() enter the bootloader from a clean thread-mode context. */
static void dumb_bootloader_fault_handler(void) {
    dumb_bootloader_request_reset();
}

void dumb_bootloader_install_fault_vectors(void) {
    const uint32_t *src = (const uint32_t *)SCB->VTOR;
    for (uint32_t i = 0U; i < DUMB_BOOTLOADER_VECTOR_COUNT; i++) {
        s_ram_vectors[i] = src[i];
    }

    const uint32_t handler = (uint32_t)&dumb_bootloader_fault_handler;
    s_ram_vectors[EXC_HARDFAULT] = handler;
    s_ram_vectors[EXC_MEMMANAGE] = handler;
    s_ram_vectors[EXC_BUSFAULT] = handler;
    s_ram_vectors[EXC_USAGEFAULT] = handler;

    __DSB();
    SCB->VTOR = (uint32_t)s_ram_vectors;
    __DSB();
    __ISB();
}

/* ---- UART magic-command task --------------------------------------------- */

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

static void Task_DumbBootloader(void *pvParameters) {
    (void)pvParameters;

    uint8_t byte = 0U;
    for (;;) {
        if (uart_recv(husart3, &byte, 1U, portMAX_DELAY) != UART_OK) {
            continue;
        }
        if (dumb_bootloader_feed_byte(byte)) {
            /* Acknowledge so the host knows the command was received... */
            (void)uart_send(husart3, (const uint8_t *)DUMB_BOOTLOADER_ACK,
                            (uint16_t)(sizeof(DUMB_BOOTLOADER_ACK) - 1U), pdMS_TO_TICKS(200));
            /* ...let the ACK drain, then reboot into the ROM bootloader. */
            vTaskDelay(pdMS_TO_TICKS(100));
            dumb_bootloader_request_reset();
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
