#pragma once

#include "MotorCAN_can_msgs.h"
#include "stm32xx_hal.h"
#include <string.h>
#include "CANbus.h"
#include "inits.h"
#include "MotorSafeBits.h"
#include "InitTask.h"
#include "StatusLEDs.h"


void MotorControlTask_Init(void);

void Task_MotorControl();
