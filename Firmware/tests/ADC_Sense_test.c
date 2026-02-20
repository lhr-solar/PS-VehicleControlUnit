/**
 * @file    ADC_Test.c
 * @brief   Simple ADC test task: prints raw ADC counts and converted voltages
 */

// I made chat do this bc i dont know if i can keep it halal anymore im going to fucking die

#include "ADC_Sense.h"
#include "inits.h"
#include "pinDefs.h"
#include <stdio.h>
#include "task.h"

int main()
{
    // Initialize hardware, clocks, etc. here
    SystemClock_Config();
    ADC_Sense_Init();
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_ADC2_Init();

    if (ADC_Sense_Init() != ADC_SENSE_OK) {
        printf("ADC init failed! Error mask = 0x%08lx\r\n",
               (unsigned long)ADC_Sense_GetErrorMask());
        return -1;
    }

    ADC_Sense_Result test = {0};
    uint32_t updated = 0;

    for (;;) {
        ADC_Sense_Status status = Read_ADC(pdMS_TO_TICKS(50), &test, &updated);

        if (status == ADC_SENSE_OK) {
            printf("Motor: %ld mV | Battery: %ld mV\r\n",
                   (long)test.Motor_Voltage,
                   (long)test.Battery_Voltage);
        } else {
            uint32_t err = ADC_Sense_GetErrorMask();
            printf("Read_ADC failed; error mask=0x%08lx\r\n", (unsigned long)err);
        }

        vTaskDelay(pdMS_TO_TICKS(ADC_TEST_PERIOD_MS));
    }

    return 0;
}