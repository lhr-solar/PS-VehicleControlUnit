#pragma once

/**
 * Build-time gate for SWOC autotune firmware.
 * Enable 1 while collecting table data on-track; merge with 0 for production.
 *
 * When enabled:
 * - Software over-current does NOT raise FAULT_ID_MOTOR_SOFTWARE_OVERCURRENT (fault handler idle).
 * - Drive-current SWOC limit follows the autotuner’s running table / trial row.
 */
#ifndef ENABLE_SWOC_AUTOTUNE
#define ENABLE_SWOC_AUTOTUNE 0
#endif
