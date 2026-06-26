#pragma once

/**
 * @file DumbBootloaderTask.h
 * @brief Minimal "dumb" bootloader trigger.
 *
 * A low-priority task watches the console UART (USART3 / husart3) for the magic
 * command "BOOT". On a match it jumps into the STM32 built-in system-memory (ROM)
 * bootloader, after which the chip can be reflashed over the same UART with
 * STM32CubeProgrammer (-c port=...) or stm32flash.
 *
 * The jump-to-system-memory method is used instead of toggling BOOT0: a pin
 * driven high in software does not survive the reset before BOOT0 is sampled,
 * whereas the software jump is deterministic and needs no extra hardware.
 *
 * Send the magic packet with Firmware/scripts/enter_bootloader.py.
 */

/** Create the task that listens for the magic command on the console UART. */
void DumbBootloaderTask_Init(void);

/**
 * @brief Jump into the STM32 system-memory ROM bootloader. Does not return.
 *
 * Tears down clocks/peripherals/interrupts, switches to MSP, then branches to
 * the bootloader reset vector. Safe to call from a FreeRTOS task context.
 */
void dumb_bootloader_enter_rom(void);
