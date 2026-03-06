/**
 * @file tasks.c
 * @brief FreeRTOS static task and event group storage
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#include "FreeRTOS.h"
#include "stm32xx_hal.h"
#include "tasks.h"
#include "CAN_FD.h"
#include "watchdogs.h"

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

    hfdcan1->Instance = FDCAN1;
    hfdcan1->Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan1->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan1->Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
    hfdcan1->Init.AutoRetransmission = DISABLE;
    hfdcan1->Init.TransmitPause = DISABLE;
    hfdcan1->Init.ProtocolException = DISABLE;
    hfdcan1->Init.NominalPrescaler = 20;
    hfdcan1->Init.NominalSyncJumpWidth = 1;
    hfdcan1->Init.NominalTimeSeg1 = 13;
    hfdcan1->Init.NominalTimeSeg2 = 2;
    hfdcan1->Init.DataPrescaler = 1;
    hfdcan1->Init.DataSyncJumpWidth = 1;
    hfdcan1->Init.DataTimeSeg1 = 1;
    hfdcan1->Init.DataTimeSeg2 = 1;
    hfdcan1->Init.StdFiltersNbr = 1;
    hfdcan1->Init.ExtFiltersNbr = 0;
    hfdcan1->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;


    // FDCAN1 Filter Config
    FDCAN_FilterTypeDef sFilterConfig;
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; // directs frames to FIFO0
    sFilterConfig.FilterID1 = 0x000;
    sFilterConfig.FilterID2 = 0x000;

    can_fd_init(hfdcan1, &sFilterConfig);
    can_fd_init(hfdcan2, &sFilterConfig);

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
    can_fd_start(hfdcan1);
    can_fd_start(hfdcan2);
}


int main(void) {
    HAL_Init();
    SystemClock_Config();
    // HAL_FDCAN_DeInit(hfdcan3);

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
