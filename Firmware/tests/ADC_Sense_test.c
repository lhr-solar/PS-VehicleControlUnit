/**
 * @file    ADC_Test.c
 * @brief   Simple ADC test task: prints raw ADC counts and converted voltages
 */

// I made chat do this bc i dont know if i can keep it halal anymore im going to fucking die

#include "inits.h"
#include "pinDefs.h"
#include "stm32xx_hal.h"
#include "StatusLEDs.h"
#include "UART.h"
#include "printf.h"
#include "ADC_Sense.h"
#include "ADC.h"

int main()
{
    if (HAL_Init() != HAL_OK)
        Error_Handler();
    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // Initialize all LED GPIOs
    LEDs_init();

    Toggle_LED(7, ON);
    HAL_Delay(1000);
    Toggle_LED(7, OFF);
    HAL_Delay(1000);

    if (ADC_Sense_Init() == ADC_SENSE_ERR_0)
    {
        Toggle_LED(6, OFF);
    }
    else if (ADC_Sense_Init() == ADC_SENSE_ERR_1)
    {
        Toggle_LED(8, OFF);
    }
    else
    {
        Toggle_LED(9, OFF);
    }

    Toggle_LED(7, ON);
    HAL_Delay(1000);
    Toggle_LED(7, OFF);
    HAL_Delay(1000);

    MX_UART_INIT(husart3);

    MX_USART3_UART_Init();

    // Initialize printf to use UART3
    printf_init(husart3);

    Toggle_LED(7, ON);
    HAL_Delay(1000);
    Toggle_LED(7, OFF);
    HAL_Delay(1000);

    // if (ADC_Sense_Init() != ADC_SENSE_OK) {
    //     printf("ADC init failed! Error mask = 0x%08lx\r\n",
    //            (unsigned long)ADC_Sense_GetErrorMask());
    //     return -1;
    // }

    // ADC_Sense_Result test = {0};
    // uint32_t updated = 0;

    // while (1)
    // {
    //     ADC_Sense_Status status = Read_ADC(pdMS_TO_TICKS(50), &test, &updated);

    //     if (status == ADC_SENSE_OK)
    //     {
    //         printf("Motor: %ld mV | Battery: %ld mV\r\n",
    //                test.Motor_Voltage,
    //                test.Battery_Voltage);
    //     }
    //     else
    //     {
    //         uint32_t err = ADC_Sense_GetErrorMask();
    //         printf("Read_ADC failed; error mask=0x%08lx\r\n", err);
    //     }
    // }

    return 0;
}