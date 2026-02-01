#ifndef PRECHARGE_H
#define PRECHARGE_H

#include <stdint.h>

// Precharge thresholds
#define OVERVOLTAGE_THRESHOLD_MV   140000  // 140 V
#define UNDERVOLTAGE_THRESHOLD_MV  1000    // 1.0 V

// Fixed-point scaling for ratio comparisons
#define RATIO_SCALE 1000

// 900/1000 = 0.900, 800/1000 = 0.800
#define THRESHOLD_1 900
#define THRESHOLD_2 800

// Public globals
extern int Precharge_Threshold;

void PrechargeStart(void);

#endif