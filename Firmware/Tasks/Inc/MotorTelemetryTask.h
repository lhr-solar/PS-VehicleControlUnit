#pragma once

#include "MotorCAN_can_msgs.h"
#include "stm32xx_hal.h"
#include <string.h>
#include "CANbus.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"

void MotorTelemetryTask_Init(void);

void Task_MotorTelemetry();

