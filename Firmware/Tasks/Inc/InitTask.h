#pragma once

#include "FaultHandlerTask.h"
#include "PrechargeTask.h"
#include "MotorSafeBits.h"
#include "MotorControlTask.h"
#include "MotorTelemetryTask.h"
#include "CanTxTelemetryTask.h"
#include "CANbus.h"


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

extern StaticTask_t Can_Tx_Telemetry_Task_Buffer;
extern StackType_t Can_Tx_Telemetry_Task_Stack[configMINIMAL_STACK_SIZE];

#define FAULTER_HANDLER_THREAD_PRIO     (tskIDLE_PRIORITY + 4)
#define PRECHARGE_THREAD_PRIO           (tskIDLE_PRIORITY + 3)
#define MOTOR_CONTROL_THREAD_PRIO       (tskIDLE_PRIORITY + 2)
#define MOTOR_TELEMETRY_THREAD_PRIO     (tskIDLE_PRIORITY + 1)
#define CAN_TX_TELEMETRY_THREAD_PRIO    (tskIDLE_PRIORITY + 1)

// Period the fault thread runs at (once a fault is active)
#define FAULT_LOOP_PERIOD_MS 500

// Period the motor control thread runs at
#define MOTOR_CONTROL_TASK_PERIOD_MS 100

void Task_Init();