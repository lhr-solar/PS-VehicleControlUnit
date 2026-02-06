#ifndef PRECHARGE_H
#define PRECHARGE_H

#include <stdint.h>

// Precharge thresholds
#define OVERVOLTAGE_THRESHOLD_MV   140000  // 140 V
#define UNDERVOLTAGE_THRESHOLD_MV  1000    // 1.0 V

// Fixed-point scaling for ratio comparisons
#define RATIO_SCALE 1000

// 900/1000 = 0.900, 800/1000 = 0.800
#define PRECHARGE_GOOD_THRESHOLD 900
#define PRECHARGE_TRANSITION_THRESHOLD 800

typedef enum {
    PRECHARGE_IN_PROGRESS = 0,
    PRECHARGE_OK,
    PRECHARGE_ERR_ADC,
    PRECHARGE_ERR_OVERVOLTAGE,
    PRECHARGE_ERR_UNDERVOLTAGE,
    PRECHARGE_ERR_TIMEOUT,
} Precharge_Status;

/**
 * @brief   Run/advance the precharge sequence and return current status
 *
 * Intended to be called periodically from a task loop. This function:
 *  - Latches the start time on first entry
 *  - Reads ADC voltages via Read_ADC()
 *  - Checks battery over/undervoltage limits
 *  - Checks precharge completion condition (Motor/Battery ratio threshold)
 *  - Enforces a timeout
 *
 * Fault behavior:
 *  - On any fault, status is latched internally and future calls return the same fault
 *    until a reset function is implemented/called (see TODO in Precharge.c).
 *
 * @return  PRECHARGE_IN_PROGRESS while running,
 *          PRECHARGE_OK when complete,
 *          PRECHARGE_ERR_* on fault
 */
Precharge_Status PrechargeStart(void);

#ifndef PRECHARGE_TIMEOUT_MS
#define PRECHARGE_TIMEOUT_MS 5000
#endif

#endif