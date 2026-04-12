#pragma once

#include "FaultBits.h"
#include "StatusLEDs.h"
#include "Contactors.h"
#include "task.h"
#include "InitTask.h"
#include "CANbus.h"
#include "CarCAN_can_msgs.h"
#include "MotorSafeBits.h"


/** @brief Run the fault handler task. */
void Task_FaultHandler();
