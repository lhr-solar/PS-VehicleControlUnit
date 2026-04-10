#pragma once

#include "FaultBits.h"
#include "Contactors.h"
#include "StatusLEDs.h"
#include "ADC_Sense.h"
#include "InitTask.h"
#include "MotorSafeBits.h"
#include "CANbus.h"
#include "CarCAN_can_msgs.h"
#include <string.h>

void Init_DriverInputTestTask();
void Task_DriverInputTest();
