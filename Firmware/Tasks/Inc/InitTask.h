#pragma once

#include "StatusLEDs.h"
#include "FaultHandlerTask.h"
#include "PrechargeTask.h"
#include "PrechargeTask.h"
#include "FaultHandlerTask.h"

extern StaticTask_t FaultHandlerTask_Buffer;
extern StackType_t FaultHandlerTask_Stack[configMINIMAL_STACK_SIZE];

extern StaticTask_t Precharge_Task_Buffer;
extern StackType_t Precharge_Task_Stack[configMINIMAL_STACK_SIZE];

extern StaticTask_t Init_Task_Buffer;
extern StackType_t Init_Task_Stack[configMINIMAL_STACK_SIZE];

void Task_Init();