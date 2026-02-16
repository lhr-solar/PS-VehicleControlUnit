#include "DriveMotor.h"
#include <stdint.h>
#include "stdbool.h"


// CAN MSG VARIABLES
static gear_t gear = DASH_NEU;
static bool regenButtonPressed = false;
static bool cruiseControlButton = false;
static bool regenEnabled = false;
static bool okToRegen = false;
static bool bpsTripped = false;
static float brakePedalPercent = 0.0f;
static float accelPedalPercent = 0.0f;

static ignitionState_t ignitionState = IGN_OFF;

static float thresholdBrake = BRAKE_THRESH;
