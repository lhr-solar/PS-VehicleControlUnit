#include "inits.h"
#include "stm32xx_hal.h"
#include "PrechargeTask.h"
#include "StatusLEDs.h"

StaticTask_t    Precharge_Task_Buffer;
StackType_t     Precharge_Task_Stack[configMINIMAL_STACK_SIZE];

int main()
{
    if (HAL_Init() != HAL_OK)
        Error_Handler();
    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // Initialize all LED GPIOs
    LEDs_init();

    MX_UART_INIT(husart3);

    MX_USART3_UART_Init();

    // Task
    xTaskCreateStatic(
        Task_Precharge,             // Task function
        "Precharge",                // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,   // Stack size in words
        NULL,                       // Task input parameter
        tskIDLE_PRIORITY + 1,       // Task priority
        Precharge_Task_Stack,       // Task handle
        &Precharge_Task_Buffer      // Static task buffer (optional)
    );

    vTaskStartScheduler();

    return 0;
}