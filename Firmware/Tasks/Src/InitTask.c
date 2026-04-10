#include "InitTask.h"

StaticTask_t FaultHandlerTask_Buffer;
StackType_t FaultHandlerTask_Stack[configMINIMAL_STACK_SIZE];

StaticTask_t Precharge_Task_Buffer;
StackType_t Precharge_Task_Stack[configMINIMAL_STACK_SIZE];

StaticTask_t Init_Task_Buffer;
StackType_t Init_Task_Stack[configMINIMAL_STACK_SIZE];

StaticTask_t Driver_Input_Task_Buffer;
StackType_t Driver_Input_Task_Stack[configMINIMAL_STACK_SIZE];

void Task_Init()
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    MX_UART_INIT(husart3);
    Init_UART_Printf();
    CAN_Init();

    xTaskCreateStatic(
        Task_FaultHandler,        // Task function
        "FaultHandler",           // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE, // Stack size in words
        NULL,                     // Task input parameter
        tskIDLE_PRIORITY + 3,     // Task priority
        FaultHandlerTask_Stack,   // Task handle
        &FaultHandlerTask_Buffer  // Static task buffer (optional)
    );

    hprecharge_task = xTaskCreateStatic(
        Task_Precharge,           // Task function
        "Precharge",              // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE, // Stack size in words
        NULL,                     // Task input parameter
        tskIDLE_PRIORITY + 2,     // Task priority
        Precharge_Task_Stack,     // Task handle
        &Precharge_Task_Buffer    // Static task buffer (optional)
    );

    xTaskCreateStatic(
        Task_DriverInputTest,
        "DriverInputTest",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 2,
        Driver_Input_Task_Stack,
        &Driver_Input_Task_Buffer
    );

    vTaskDelete(NULL);
}