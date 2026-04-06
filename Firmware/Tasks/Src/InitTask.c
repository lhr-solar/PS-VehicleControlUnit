#include "InitTask.h"

StaticTask_t FaultHandlerTask_Buffer;
StackType_t FaultHandlerTask_Stack[FAULT_HANDLER_TASK_STACK_SIZE];

StaticTask_t Precharge_Task_Buffer;
StackType_t Precharge_Task_Stack[PRECHARGE_TASK_STACK_SIZE];

StaticTask_t Init_Task_Buffer;
StackType_t Init_Task_Stack[INIT_TASK_STACK_SIZE];

// StaticTask_t Motor_Control_Task_Buffer;
// StackType_t Motor_Control_Task_Stack[MOTOR_CONTROL_TASK_STACK_SIZE];

// StaticTask_t Motor_Telemetry_Task_Buffer;
// StackType_t Motor_Telemetry_Task_Stack[MOTOR_TELEMETRY_TASK_STACK_SIZE];

// StaticTask_t Can_Tx_Telemetry_Task_Buffer;
// StackType_t Can_Tx_Telemetry_Task_Stack[CAN_TX_TELEMETRY_STACK_SIZE];

StaticTask_t FSM_Task_Buffer;
StackType_t FSM_Task_Stack[FSM_TASK_STACK_SIZE];

StaticTask_t VCUStatus_Task_Buffer;
StackType_t VCUStatus_Task_Stack[VCU_STATUS_TASK_STACK_SIZE];

// void Task_Init() {
//     __HAL_RCC_SYSCFG_CLK_ENABLE();
//     __HAL_RCC_PWR_CLK_ENABLE();

//     Init_UART_Printf();

//     MotorSafeBits_Init();

//     MotorCAN_Init();
//     CarCAN_Init();

    xTaskCreateStatic(
        Task_FaultHandler,          // Task function
        "FaultHandler",             // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,   // Stack size in words
        NULL,                       // Task input parameter
        FAULT_HANDLER_THREAD_PRIO,  // Task priority
        FaultHandlerTask_Stack,     // Task handle
        &FaultHandlerTask_Buffer    // Static task buffer (optional)
    );

//     hprecharge_task = xTaskCreateStatic(
//         Task_Precharge,                 // Task function
//         "Precharge",                    // Name of the task (for debugging)
//         configMINIMAL_STACK_SIZE,       // Stack size in words
//         NULL,                           // Task input parameter
//         PRECHARGE_THREAD_PRIO,          // Task priority
//         Precharge_Task_Stack,           // Task handle
//         &Precharge_Task_Buffer          // Static task buffer (optional)
//     );

//     // xTaskCreateStatic(
//     //     Task_MotorControl,              // Task function
//     //     "Motor Control Thread",         // Name of the task (for debugging)
//     //     configMINIMAL_STACK_SIZE,       // Stack size in words
//     //     NULL,                           // Task input parameter
//     //     MOTOR_CONTROL_THREAD_PRIO,      // Task priority
//     //     Motor_Control_Task_Stack,       // Task handle
//     //     &Motor_Control_Task_Buffer      // Static task buffer (optional)
//     // );

//     xTaskCreateStatic(
//         Task_MotorTelemetry,            // Task function
//         "Motor Telemetry Thread",       // Name of the task (for debugging)
//         configMINIMAL_STACK_SIZE,       // Stack size in words
//         NULL,                           // Task input parameter
//         MOTOR_TELEMETRY_THREAD_PRIO,    // Task priority
//         Motor_Telemetry_Task_Stack,     // Task handle
//         &Motor_Telemetry_Task_Buffer    // Static task buffer (optional)
//     );

//     // xTaskCreateStatic(
//     //     Task_CanTxTelemetry,            // Task function
//     //     "Can TX Telemetry Thread",      // Name of the task (for debugging)
//     //     configMINIMAL_STACK_SIZE,       // Stack size in words
//     //     NULL,                           // Task input parameter
//     //     CAN_TX_TELEMETRY_THREAD_PRIO,   // Task priority
//     //     Can_Tx_Telemetry_Task_Stack,     // Task handle
//     //     &Can_Tx_Telemetry_Task_Buffer    // Static task buffer (optional)
//     // );

//     vTaskDelete(NULL);
// }


void Task_Init() {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    Init_UART_Printf();

    MotorSafeBits_Init();

    MotorCAN_Init();
    CarCAN_Init();

    xTaskCreateStatic(
        Task_FaultHandler,          // Task function
        "FaultHandler",             // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,   // Stack size in words
        NULL,                       // Task input parameter
        FAULT_HANDLER_THREAD_PRIO,  // Task priority
        FaultHandlerTask_Stack,     // Task handle
        &FaultHandlerTask_Buffer    // Static task buffer (optional)
    );

    xTaskCreateStatic(
        Task_Precharge,                 // Task function
        "Precharge",                    // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,       // Stack size in words
        NULL,                           // Task input parameter
        PRECHARGE_THREAD_PRIO,          // Task priority
        Precharge_Task_Stack,           // Task handle
        &Precharge_Task_Buffer          // Static task buffer (optional)
    );

    xTaskCreateStatic(
        Task_FSM,              // Task function
        "FSM Thread",         // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,       // Stack size in words
        NULL,                           // Task input parameter
        FSM_THREAD_PRIO,      // Task priority
        FSM_Task_Stack,       // Task handle
        &FSM_Task_Buffer      // Static task buffer (optional)
    );

    xTaskCreateStatic(
        Task_BroadcastVCUStatus,              // Task function
        "VCU Status Thread",                  // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,             // Stack size in words
        NULL,                                 // Task input parameter
        VCU_STATUS_THREAD_PRIO,               // Task priority
        VCUStatus_Task_Stack,                 // Task handle
        &VCUStatus_Task_Buffer                // Static task buffer (optional)
    );

    vTaskDelete(NULL);
}