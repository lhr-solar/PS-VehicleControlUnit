#include "InitTask.h"
#include "FSM.h"
#include "VCUStatusTask.h"
#include "PrechargeTask.h"
#include "UpdateFSMInputsTask.h"
#include "StatusLEDs.h"

/* ===================== Stack Size Definitions ===================== */



/* ===================== Static Task Buffers ===================== */
StaticTask_t FaultHandlerTask_Buffer;
StackType_t FaultHandlerTask_Stack[FAULT_HANDLER_TASK_STACK_SIZE];

StaticTask_t Precharge_Task_Buffer;
StackType_t Precharge_Task_Stack[PRECHARGE_TASK_STACK_SIZE];

StaticTask_t Init_Task_Buffer;
StackType_t Init_Task_Stack[INIT_TASK_STACK_SIZE];

StaticTask_t FSM_Task_Buffer;
StackType_t FSM_Task_Stack[FSM_TASK_STACK_SIZE];

StaticTask_t VCUStatus_Task_Buffer;
StackType_t VCUStatus_Task_Stack[VCU_STATUS_TASK_STACK_SIZE];

StaticTask_t UpdateFSMInputs_Task_Buffer;
StackType_t UpdateFSMInputs_Task_Stack[UPDATE_FSM_INPUTS_STACK_SIZE];

// StaticTask_t Motor_Control_Task_Buffer;
// StackType_t Motor_Control_Task_Stack[MOTOR_CONTROL_TASK_STACK_SIZE];

StaticTask_t VCUReceiveCAN_Task_Buffer;
StackType_t VCUReceiveCAN_Task_Stack[configMINIMAL_STACK_SIZE];

StaticTask_t Driver_Input_Task_Buffer;
StackType_t Driver_Input_Task_Stack[configMINIMAL_STACK_SIZE];

void Task_Init()
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    Init_UART_Printf();
    CAN_Init();

    MotorSafeBits_Init();

    MotorCAN_Init();
    CarCAN_Init();

    FSM_TaskInit();

    // Required for VCU status testing
    faults_init();
    Init_PrechargeTask();

    xTaskCreateStatic(
        Task_FaultHandler,
        "FaultHandler",
        FAULT_HANDLER_TASK_STACK_SIZE,
        NULL,
        FAULT_HANDLER_THREAD_PRIO,
        FaultHandlerTask_Stack,
        &FaultHandlerTask_Buffer
    );

    xTaskCreateStatic(
        Task_Precharge,
        "Precharge",
        PRECHARGE_TASK_STACK_SIZE,
        NULL,
        PRECHARGE_THREAD_PRIO,
        Precharge_Task_Stack,
        &Precharge_Task_Buffer
    );

    xTaskCreateStatic(
        Task_FSM,
        "FSM Thread",
        FSM_TASK_STACK_SIZE,
        NULL,
        FSM_THREAD_PRIO,
        FSM_Task_Stack,
        &FSM_Task_Buffer
    );

    xTaskCreateStatic(
        Task_BroadcastVCUStatus,
        "VCU Status Thread",
        VCU_STATUS_TASK_STACK_SIZE,
        NULL,
        VCU_STATUS_THREAD_PRIO,
        VCUStatus_Task_Stack,
        &VCUStatus_Task_Buffer
    );

    xTaskCreateStatic(
        Task_UpdateFSMInputs,
        "Update FSM Inputs Thread",
        UPDATE_FSM_INPUTS_STACK_SIZE,
        NULL,
        UPDATE_FSM_INPUTS_THREAD_PRIO,
        UpdateFSMInputs_Task_Stack,
        &UpdateFSMInputs_Task_Buffer
    );

    // while (1) {
    //     LED_toggle(HB);
    //     HAL_Delay(100);
    // }
        Task_VCUReceiveCAN,        // Task function
        "VCUReceiveCAN",           // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,  // Stack size in words
        NULL,                      // Task input parameter
        tskIDLE_PRIORITY + 2,      // Task priority
        VCUReceiveCAN_Task_Stack,  // Task handle
        &VCUReceiveCAN_Task_Buffer // Static task buffer (optional)
    );

    xTaskCreateStatic(
        Task_DriverInputTest,
        "DriverInputTest",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 2,
        Driver_Input_Task_Stack,
        &Driver_Input_Task_Buffer);

    vTaskDelete(NULL);
}