#include "ADC_Sense.h"
#include "ReadADCTask.h"
#include "inits.h"
#include "StatusLEDs.h"

TimerHandle_t xWindowTimer;
StaticTimer_t xTimerBuffer;

StaticEventGroup_t xReadADCEventGroup;
EventGroupHandle_t xReadADCEventGroup_handle;

void Init_ReadADCTask() {
    // Event Group init
    xReadADCEventGroup_handle = xEventGroupCreateStatic( &xReadADCEventGroup );
    configASSERT( xReadADCEventGroup_handle );         // check if handle is set 
    // xEventGroupClearBits(xReadADCEventGroup_handle,    /* The event group being updated. */
    //                      0xFF );                    /* The bits being cleared. */

    // Inits ADC & printf
    ADC_Sense_Init();
    Init_UART_Printf();
}

void Task_ReadADC() 
{
    Init_ReadADCTask();
    Toggle_LED(CAR_CRUISE,ON);

    ADC_Sense_Result values = {0};

    while(1)
    {
        if (Read_ADC(pdMS_TO_TICKS(50), &values) == ADC_SENSE_OK)
        {
            printf("Motor: %ld mV | Battery: %ld mV\r\n",
                values.Motor_Voltage,
                values.Battery_Voltage);
            Toggle_LED(CAR_REGEN, ON);
        }
        else
        {
            printf("Read_ADC failed\r\n");
            Toggle_LED(CAR_BPSFAULT, ON);
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}