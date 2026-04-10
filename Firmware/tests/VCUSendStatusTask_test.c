#include "stm32xx_hal.h"
#include "StatusLEDs.h"
#include "VCUSendStatusTask.h"

#define PRINTF_DEBUG

StaticTask_t VCUStatus_buffer;
StackType_t VCUStatus_stack[512];

void can_error_handler()
{
    while (1)
    {
        HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
        HAL_Delay(500);
    }
}

int main()
{
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LEDs_init();

    if (CAN_Init() != CAN_OK)
    {
        can_error_handler();
    }


    VCUSendStatusTask_Init();

    Init_UART_Printf();

    xTaskCreateStatic(
        Task_VCUSendStatus,
        "Send VCU Status Task",
        512,
        NULL,
        tskIDLE_PRIORITY + 2, // TODO: set appropriate priority
        VCUStatus_stack,
        &VCUStatus_buffer);

    vTaskStartScheduler();

    while (1)
    {
    }
}
