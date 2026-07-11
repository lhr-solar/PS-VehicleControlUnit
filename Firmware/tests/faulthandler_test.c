#include "FaultHandlerTask.h"
#include "PrechargeTask.h"
#include "StatusLEDs.h"
#include "InitTask.h"
#include "inits.h"

int main()
{
    if (HAL_Init() != HAL_OK)
        Error_Handler();
    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    Init_UART_Printf();
    LED_init();

    faults_init();


    // Task
    xTaskCreateStatic(
        Task_FaultHandler,
        "FaultHandler",
        FAULT_HANDLER_TASK_STACK_SIZE,
        NULL,
        FAULT_HANDLER_THREAD_PRIO,
        FaultHandler_Task_Stack,
        &FaultHandler_Task_Buffer
    );

    precharge_task_handle = xTaskCreateStatic(
        Task_Precharge,
        "Precharge",
        PRECHARGE_TASK_STACK_SIZE,
        NULL,
        PRECHARGE_THREAD_PRIO,
        Precharge_Task_Stack,
        &Precharge_Task_Buffer
    );

    vTaskStartScheduler();

    return 0;
}