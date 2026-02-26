#ifndef TASKS_H
#define TASKS_H

#include "FreeRTOS.h"
#include "task.h" 
#include <event_groups.h>


extern EventGroupHandle_t xWDogEventGroup_handle;
extern EventGroupHandle_t xFaultEventGroup_handle;


/* ====================== FSM TASK ====================== */
StaticTask_t Task_FSM_Buffer;
StackType_t Task_FSM_Stack_Array[configMINIMAL_STACK_SIZE];
#define TASK_FSM_STACK_SIZE configMINIMAL_STACK_SIZE


/* ================= UPDATE STATUS TASK ================= */
StaticTask_t Task_UpdateControlStatus_Buffer; 
StackType_t Task_UpdateControlStatus_Stack_Array[configMINIMAL_STACK_SIZE];


/* ============= BROADCAST MOTOR STATUS TASK ============= */
StaticTask_t Task_BroadcastMotorStatus_Buffer;
StackType_t Task_BroadcastMotorStatus_Stack_Array[configMINIMAL_STACK_SIZE];


/* ==================== WATCHDOG TASK ==================== */

StaticEventGroup_t xWdEventGroupBuffer;
EventGroupHandle_t xWdEventGroup;


/* ====================== FAULT TASK ===================== */

StaticTask_t Task_Fault_Buffer;
StackType_t Task_Fault_Stack_Array[configMINIMAL_STACK_SIZE];



#endif /* TASKS_H */