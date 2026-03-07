#pragma once

#include "FaultHandlerTask.h"
#include "PrechargeTask.h"
#include "MotorSafeBits.h"
#include "MotorControlTask.h"
#include "MotorTelemetryTask.h"


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

#define FAULTER_HANDLER_THREAD_PRIO     (tskIDLE_PRIORITY + 4)
#define PRECHARGE_THREAD_PRIO           (tskIDLE_PRIORITY + 3)
#define MOTOR_CONTROL_THREAD_PRIO       (tskIDLE_PRIORITY + 2)
#define MOTOR_TELEMETRY_THREAD_PRIO     (tskIDLE_PRIORITY + 1)


void Task_Init();