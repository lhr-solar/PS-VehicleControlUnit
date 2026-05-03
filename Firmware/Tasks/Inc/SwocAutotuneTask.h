#pragma once

#include "SwocAutotune_config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SwocAutotune_Init(void);

/**
 * @brief SWOC-limit fraction [0..1] for current speed during autotune (else 1.0 when disabled).
 */
float SwocAutotune_MaxCurrentFraction(float speed_mph);

/**
 * @brief Invoke once each UpdateVCUInputs cycle after g_data_read and FSM inputs are current.
 */
void SwocAutotune_OnInputCycleComplete(void);

#ifdef __cplusplus
}
#endif
