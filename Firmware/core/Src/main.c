/**
 * @file tasks.c
 * @brief FreeRTOS static task and event group storage
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#include "FreeRTOS.h"
#include "stm32xx_hal.h"
#include "tasks.h"
#include "watchdogs.h"
#include "can_utils.h"

// Init
static StaticTask_t  Task_Init_Buffer;
static StackType_t   Task_Init_Stack[TASK_INIT_STACK_SIZE];
TaskHandle_t         Task_Init_Handle;

// FSM
static StaticTask_t  Task_FSM_Buffer;
static StackType_t   Task_FSM_Stack[TASK_FSM_STACK_SIZE];
TaskHandle_t         Task_FSM_Handle;

// UpdateControlStatus
static StaticTask_t  Task_UpdateControlStatus_Buffer;
static StackType_t   Task_UpdateControlStatus_Stack[TASK_UPDATE_STATUS_STACK_SIZE];
TaskHandle_t         Task_UpdateControlStatus_Handle;

// BroadcastMotorStatus
static StaticTask_t  Task_BroadcastMotorStatus_Buffer;
static StackType_t   Task_BroadcastMotorStatus_Stack[TASK_BROADCAST_STACK_SIZE];
TaskHandle_t         Task_BroadcastMotorStatus_Handle;

// FaultHandler
static StaticTask_t  Task_Fault_Buffer;
static StackType_t   Task_Fault_Stack[TASK_FAULT_STACK_SIZE];
TaskHandle_t         Task_FaultHandler_Handle;


void Task_Init(void *args  __attribute__((unused))) {
    watchdog_init();
    WATCHDOG_INIT_ALL_FSM_SIGNALS();

    can_init_all();
    fsm_init();

    Task_FSM_Handle = xTaskCreateStatic(
        Task_FSM,
        "FSM",
        TASK_FSM_STACK_SIZE,
        NULL,
        TASK_FSM_PRIORITY,
        Task_FSM_Stack,
        &Task_FSM_Buffer
    );
    configASSERT(Task_FSM_Handle != NULL);

    Task_UpdateControlStatus_Handle = xTaskCreateStatic(
        Task_UpdateControlStatus,
        "UpdateStatus",
        TASK_UPDATE_STATUS_STACK_SIZE,
        NULL,
        TASK_UPDATE_STATUS_PRIORITY,
        Task_UpdateControlStatus_Stack,
        &Task_UpdateControlStatus_Buffer
    );
    configASSERT(Task_UpdateControlStatus_Handle != NULL);

    Task_BroadcastMotorStatus_Handle = xTaskCreateStatic(
        Task_BroadcastVCUStatus,
        "BroadcastMotor",
        TASK_BROADCAST_STACK_SIZE,
        NULL,
        TASK_BROADCAST_PRIORITY,
        Task_BroadcastMotorStatus_Stack,
        &Task_BroadcastMotorStatus_Buffer
    );
    configASSERT(Task_BroadcastMotorStatus_Handle != NULL);

    Task_FaultHandler_Handle = xTaskCreateStatic(
        Task_FaultHandler,
        "FaultHandler",
        TASK_FAULT_STACK_SIZE,
        NULL,
        TASK_FAULT_PRIORITY,
        Task_Fault_Stack,
        &Task_Fault_Buffer
    );
    configASSERT(Task_FaultHandler_Handle != NULL);

    watchdog_start_all();
    can_start_all();
}


int main(void) {
    HAL_Init();
    SystemClock_Config();

    Task_Init_Handle = xTaskCreateStatic(
        Task_Init,
        "Init",
        TASK_INIT_STACK_SIZE,
        NULL,
        TASK_INIT_PRIORITY,
        Task_Init_Stack,
        &Task_Init_Buffer
    );
    configASSERT(Task_Init_Handle != NULL);
    

    

    vTaskStartScheduler();

    while (1);  // should never reach here
}
