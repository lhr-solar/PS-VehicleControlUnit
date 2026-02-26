#ifndef VCU_TASKS_H
#define VCU_TASKS_H

#include "FreeRTOS.h" /* Must come first. */
#include "task.h" 
#include <event_groups.h>
#include "SendTritium.h"


extern EventGroupHandle_t xWDogEventGroup_handle;
extern EventGroupHandle_t xFaultEventGroup_handle;


/* ================= FSM TASK ================= */

StaticTask_t Task_FSM_Buffer;
StackType_t Task_FSM_Stack_Array[configMINIMAL_STACK_SIZE];

/* ================= WATCHDOG TASK ================= */

StaticEventGroup_t xWdEventGroupBuffer;
EventGroupHandle_t xWdEventGroup;

// StaticTask_t Task_Watchdog_Buffer;
// StackType_t Task_Watchdog_Stack_Array[configMINIMAL_STACK_SIZE];
// EventGroupHandle_t xWDogEventGroup_handle;

/* ================= FAULT TASK ================= */

StaticTask_t Task_Fault_Buffer;
StackType_t Task_Fault_Stack_Array[configMINIMAL_STACK_SIZE];


/* ================ CAN TASK ====================*/



#endif /* vcu_tasks.h */