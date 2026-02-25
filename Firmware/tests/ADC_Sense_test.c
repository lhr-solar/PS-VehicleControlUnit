#include "inits.h"
#include "stm32xx_hal.h"
#include "StatusLEDs.h"
#include "ADC_Sense.h"
#include "ReadADCTask.h"

StaticTask_t    ReadADC_Task_Buffer;
StackType_t     ReadADC_Task_Stack[configMINIMAL_STACK_SIZE];

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

    xTaskCreateStatic(
        Task_ReadADC,               // Task function
        "ReadADC",                  // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,   // Stack size in words
        NULL,                       // Task input parameter
        tskIDLE_PRIORITY + 1,       // Task priority
        ReadADC_Task_Stack,         // Task handle
        &ReadADC_Task_Buffer        // Static task buffer (optional)
    );

    vTaskStartScheduler();

    return 0;
}