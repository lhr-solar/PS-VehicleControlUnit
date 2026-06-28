#pragma once

/**
 * @file HeartbeatTask.h
 * @brief Liveness heartbeat: blinks the HB LED at ~1 Hz from a dedicated task.
 *
 * Independent of the control/CAN tasks so the blink reflects RTOS liveness:
 * if the firmware hangs or hardfaults the LED stops (and a hardfault also routes
 * to the ROM bootloader, see Embedded-Sharepoint bootloader_lite / BootloaderTask).
 */

/** Create the heartbeat task. */
void HeartbeatTask_Init(void);
