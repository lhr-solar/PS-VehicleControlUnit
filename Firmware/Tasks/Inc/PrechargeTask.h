#pragma once

#include <event_groups.h>

// Precharge thresholds
#define OVERVOLTAGE_THRESHOLD_MV 140000 // 140 V
#define UNDERVOLTAGE_THRESHOLD_MV 1000  // 1.0 V

// Fixed-point scaling for ratio comparisons
#define RATIO_SCALE 1000

// 900/1000 = 0.900, 800/1000 = 0.800
#define PRECHARGE_THRESHOLD_90 900
#define PRECHARGE_THRESHOLD_80 800

#define PRECHARGE_TIMEOUT_MS 400
#define ADC_TIMEOUT_MS 20

typedef enum
{
    PRECHARGE_STATE_INITIAL = 0,
    PRECHARGE_STATE_PRECHARGING, // Precharge sequence started successfully, close contactor and check hysterisis
    PRECHARGE_STATE_RUN          // Precharge got through hysterisis, now continuously polling ADC
} Precharge_State_t;

void Init_PrechargeTask();

void Task_Precharge();
