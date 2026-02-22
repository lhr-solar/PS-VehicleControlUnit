#ifndef PRECHARGE_H
#define PRECHARGE_H

#include "ADC_Sense.h"

// Precharge thresholds
#define OVERVOLTAGE_THRESHOLD_MV   140000  // 140 V
#define UNDERVOLTAGE_THRESHOLD_MV  1000    // 1.0 V

// Fixed-point scaling for ratio comparisons
#define RATIO_SCALE 1000

// 900/1000 = 0.900, 800/1000 = 0.800
#define PRECHARGE_GOOD_THRESHOLD 900
#define PRECHARGE_TRANSITION_THRESHOLD 800

#ifndef PRECHARGE_TIMEOUT_MS
#define PRECHARGE_TIMEOUT_MS 5000
#endif

#define PRECHARGE_ADC_TIMEOUT_MS 20  // ADC read timeout for precharge monitoring

typedef enum {
    PRECHARGE_STATE_IDLE = 0,
    PRECHARGE_STATE_RUNNING,    // Precharge sequence is active, waiting for completion or fault
    PRECHARGE_STATE_DONE,       // Precharge sequence completed successfully, main contactor can be closed
    PRECHARGE_STATE_FAULT,      // Precharge sequence failed, contactors should be open and fault reason can be queried via PrechargeStart() return value
} Precharge_State;

typedef enum {
    PRECHARGE_IN_PROGRESS = 0,
    PRECHARGE_OK,
    PRECHARGE_ERR_ADC,              // Returns when Read_ADC() fails
    PRECHARGE_ERR_OVERVOLTAGE,      // Returns when battery voltage exceeds OVERVOLTAGE_THRESHOLD_MV
    PRECHARGE_ERR_UNDERVOLTAGE,     // Returns when battery voltage is below UNDERVOLTAGE_THRESHOLD_MV
    PRECHARGE_ERR_TIMEOUT,          // Returns when precharge sequence exceeds PRECHARGE_TIMEOUT_MS
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

#endif