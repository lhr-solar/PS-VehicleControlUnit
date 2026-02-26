#ifndef TASK_H__
#define TASK_H__

#include "stm32xx_hal.h"
#include "FreeRTOS.h" /* Must come first. */
#include "task.h" 
#include <event_groups.h>
#include "DriveMotor.h"
#include "ReceiveMotor.h"



extern EventGroupHandle_t xWDogEventGroup_handle;
extern EventGroupHandle_t xFaultEventGroup_handle;

/* ================= FSM TASK ================= */

StaticTask_t Task_FSM_Buffer;
StackType_t Task_FSM_Stack_Array[configMINIMAL_STACK_SIZE];

/* ================= UPDATE STATUS TASK ================= */
StaticTask_t Task_UpdateControlStatus_Buffer; 
StackType_t Task_UpdateControlStatus_Stack_Array[configMINIMAL_STACK_SIZE];

/* ================= BROADCAST MOTOR STATUS TASK ================= */
StaticTask_t Task_BroadcastMotorStatus_Buffer;
StackType_t Task_BroadcastMotorStatus_Stack_Array[configMINIMAL_STACK_SIZE];

/* ================= UPDATE VCU STATUS ================= */
StaticTask_t Task_SendVCUStatus_Buffer;
StackType_t Task_SendVCUStatus_Stack_Array[configMINIMAL_STACK_SIZE];

/* ================= WATCHDOG TASK ================= */

StaticEventGroup_t xWdEventGroupBuffer;
EventGroupHandle_t xWdEventGroup;

// StaticTask_t Task_Watchdog_Buffer;
// StackType_t Task_Watchdog_Stack_Array[configMINIMAL_STACK_SIZE];
// EventGroupHandle_t xWDogEventGroup_handle;

/* ================= FAULT TASK ================= */

StaticTask_t Task_Fault_Buffer;
StackType_t Task_Fault_Stack_Array[configMINIMAL_STACK_SIZE];


void Task_CANWatchdog(void *arg);
void Task_FaultHandler(void *arg);
void Task_BroadcastMotorStatus(void *p_arg);
void Task_SendVCUStatus(void *p_arg);
void watchdog_init(void);



#endif