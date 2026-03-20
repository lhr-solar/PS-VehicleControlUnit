#pragma once

#include "FaultBits.h"
#include "StatusLEDs.h"
#include "Contactors.h"
#include "task.h"
#include "InitTask.h"
#include "MotorSafeBits.h"


/** @brief Run the fault handler task. */
void Task_FaultHandler();
