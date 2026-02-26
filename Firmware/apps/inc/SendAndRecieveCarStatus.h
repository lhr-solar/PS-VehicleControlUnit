#ifndef SEND_AND_RECIEVE_CAR_STATUS_H
#define SEND_AND_RECIEVE_CAR_STATUS_H

#include <stdint.h>
#include "stdbool.h"
#include "faults.h"
#include "semphr.h"

#define BRAKE_THRESH 42
#define BRAKE_THRESH_HYST 30

extern SemaphoreHandle_t vcu_status_lock;
extern SemaphoreHandle_t controls_lock;

// CAN MSG VARIABLES
extern gear_t gear;
extern bool regenButtonPressed;
extern bool cruiseControlButton;
extern bool regenEnabled;
extern bool okToRegen;
extern bool bpsTripped;
extern float brakePedalPercent;
extern float accelPedalPercent;

extern ignitionState_t ignitionState = IGN_OFF;

extern float thresholdBrake = BRAKE_THRESH;

#endif /* SEND_AND_RECIEVE_CAR_STATUS_H */
