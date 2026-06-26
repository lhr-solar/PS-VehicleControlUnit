#pragma once

#include <stdbool.h>

/**
 * @file DumbBootloaderTask.h
 * @brief Minimal "dumb" bootloader trigger + hardfault recovery.
 *
 * A low-priority task watches the console UART (USART3 / husart3) for the magic
 * command "BOOT". On a match it replies with "BOOTACK" (so the host can confirm
 * the board heard it), then reboots into the STM32 system-memory (ROM)
 * bootloader so the chip can be reflashed over the same UART.
 *
 * A relocated (RAM) vector table also routes HardFault/MemManage/BusFault/
 * UsageFault into the same ROM-bootloader entry, so a crashed board ends up
 * reflashable instead of wedged.
 *
 * Entry is always performed from a clean, freshly-reset thread-mode context:
 * the UART command and the fault handler both set a backup-register flag and
 * NVIC_SystemReset(); main() consumes the flag early and jumps. Send the magic
 * packet with Firmware/scripts/enter_bootloader.py.
 */

/** Create the task that listens for the magic command on the console UART. */
void DumbBootloaderTask_Init(void);

/** Relocate the vector table to RAM and route faults to the ROM bootloader. */
void dumb_bootloader_install_fault_vectors(void);

/** True (and clears the flag) if a bootloader entry was requested before reset. */
bool dumb_bootloader_consume_request(void);

/** Set the backup-register flag and reset; bootloader is entered after reboot. Does not return. */
void dumb_bootloader_request_reset(void);

/** Jump straight into the STM32 system-memory ROM bootloader. Does not return. */
void dumb_bootloader_enter_rom(void);
