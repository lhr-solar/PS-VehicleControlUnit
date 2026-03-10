#pragma once

#include "FaultBits.h"
#include "StatusLEDs.h"
#include "Contactors.h"
#include "task.h"
#include "InitTask.h"
#include "MotorSafeBits.h"


/** @brief Initialize the fault handler task. */
void Init_FaultHandlerTask();

/** @brief Run the fault handler task. */
void Task_FaultHandler();

/** @brief Kill the precharge task. */
void Kill_Precharge_Task();

void Fault_Loop();

void Set_Fault_LED();