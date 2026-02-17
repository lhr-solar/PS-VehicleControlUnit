/**
 * @file    ADC_Test.c
 * @brief   Simple ADC test task: prints raw ADC counts and converted voltages
 */

 // I made chat do this bc i dont know if i can keep it halal anymore im going to fucking die

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#include "ADC_Sense.h"
#include "pinDefs.h"

/* How often to sample ADCs */
#define ADC_TEST_PERIOD_MS 500

void ADC_Test_Task(void *arg)
{
    (void)arg;

    printf("ADC test task starting...\r\n");

    if (ADC_Sense_Init() != ADC_SENSE_OK) {
        printf("ADC init failed! Error mask = 0x%08lx\r\n",
               ADC_Sense_GetErrorMask());
        vTaskDelete(NULL);
    }

    for (;;)
    {
        ADC_Sense_Result result = {0};
        uint32_t updated = 0;

        ADC_Sense_Status status =
            Read_ADC(pdMS_TO_TICKS(50), &result, &updated);

        if (status == ADC_SENSE_OK)
        {
            printf("ADC OK | ");

            printf("Motor: %ld mV | ",
                   (long)result.Motor_Voltage);

            printf("Battery: %ld mV\r\n",
                   (long)result.Battery_Voltage);
        }
        else
        {
            uint32_t err = ADC_Sense_GetErrorMask();
            printf("ADC ERR | mask=0x%08lx\r\n", (long)err);
        }

        vTaskDelay(pdMS_TO_TICKS(ADC_TEST_PERIOD_MS));
    }
}

