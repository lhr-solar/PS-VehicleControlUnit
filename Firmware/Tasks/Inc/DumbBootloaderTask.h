#pragma once

/**
 * @file DumbBootloaderTask.h
 * @brief Thin UART listener that hands off to Embedded-Sharepoint's bootloader_lite.
 *
 * Starts a low-priority task on USART3 that watches for the "BOOT" magic word and,
 * on a match, replies "BOOTACK" then jumps into the STM32 ROM bootloader.
 * Hard-fault -> ROM-bootloader routing is set up separately via
 * bootloader_lite_init() in main(). See Embedded-Sharepoint/bootloader_lite.
 */

void DumbBootloaderTask_Init(void);
