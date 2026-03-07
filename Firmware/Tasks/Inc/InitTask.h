#pragma once

#include "FaultHandlerTask.h"
#include "PrechargeTask.h"
#include "MotorSafeBits.h"


extern StaticTask_t FaultHandlerTask_Buffer;
extern StackType_t FaultHandlerTask_Stack[configMINIMAL_STACK_SIZE];

extern StaticTask_t Precharge_Task_Buffer;
extern StackType_t Precharge_Task_Stack[configMINIMAL_STACK_SIZE];

extern StaticTask_t Init_Task_Buffer;
extern StackType_t Init_Task_Stack[configMINIMAL_STACK_SIZE];

extern StaticTask_t Motor_Control_Task_Buffer;
extern StackType_t Motor_Control_Task_Stack[configMINIMAL_STACK_SIZE];

extern StaticTask_t Motor_Telemetry_Task_Buffer;
extern StackType_t Motor_Telemetry_Task_Stack[configMINIMAL_STACK_SIZE];

void Task_Init();