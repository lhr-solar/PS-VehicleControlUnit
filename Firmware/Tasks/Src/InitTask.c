#include "InitTask.h"

StaticTask_t FaultHandlerTask_Buffer;
StackType_t FaultHandlerTask_Stack[FAULT_HANDLER_TASK_STACK_SIZE];

StaticTask_t Precharge_Task_Buffer;
StackType_t Precharge_Task_Stack[PRECHARGE_TASK_STACK_SIZE];

StaticTask_t Init_Task_Buffer;
StackType_t Init_Task_Stack[INIT_TASK_STACK_SIZE];


StaticTask_t Motor_Control_Task_Buffer;
StackType_t Motor_Control_Task_Stack[MOTOR_CONTROL_TASK_STACK_SIZE];

StaticTask_t Motor_Telemetry_Task_Buffer;
StackType_t Motor_Telemetry_Task_Stack[MOTOR_TELEMETRY_TASK_STACK_SIZE];

StaticTask_t Can_Tx_Telemetry_Task_Buffer;
StackType_t Can_Tx_Telemetry_Task_Stack[CAN_TX_TELEMETRY_STACK_SIZE];

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

    xTaskCreateStatic(
        Task_FaultHandler,          // Task function
        "FaultHandler",             // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,   // Stack size in words
        NULL,                       // Task input parameter
        FAULT_HANDLER_THREAD_PRIO,  // Task priority
        FaultHandlerTask_Stack,     // Task handle
        &FaultHandlerTask_Buffer    // Static task buffer (optional)
    );

    hprecharge_task = xTaskCreateStatic(
        Task_Precharge,                 // Task function
        "Precharge",                    // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,       // Stack size in words
        NULL,                           // Task input parameter
        PRECHARGE_THREAD_PRIO,          // Task priority
        Precharge_Task_Stack,           // Task handle
        &Precharge_Task_Buffer          // Static task buffer (optional)
    );

    xTaskCreateStatic(
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