#pragma once

#include "MotorCAN_can_msgs.h"
#include "stm32xx_hal.h"
#include <string.h>
#include "CANbus.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"
#include "InitTask.h"
#include "slcanFormat.h"
#include "ESP32.h"


void MotorTelemetryTask_Init(void);

void Task_MotorTelemetry(void *args);

