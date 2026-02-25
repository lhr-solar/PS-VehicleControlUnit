#include "ADC_Sense.h"
#include "PrechargeTask.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "Contactors.h"
#include "stm32xx_hal.h"
#include "pinDefs.h"

StaticEventGroup_t xPrechargeEventGroup;
EventGroupHandle_t xPrechargeEventGroup_handle;

void Init_PrechargeTask()
{
    // Event Group init
    xPrechargeEventGroup_handle = xEventGroupCreateStatic(&xPrechargeEventGroup);
    configASSERT(xPrechargeEventGroup_handle); // check if handle is set
    // xEventGroupClearBits(xReadADCEventGroup_handle,    /* The event group being updated. */
    //                      0xFF );                    /* The bits being cleared. */

    // Inits ADC & printf
    ADC_Sense_Init();
    Init_UART_Printf();
    contactor_init();
}

void Task_Precharge()
{
    Init_PrechargeTask();

    static Precharge_State_t State = PRECHARGE_STATE_INITIAL;
    static TickType_t Start_Tick = 0;

    while (1)
    {
        // TODO: check fault bits -> call fault handler
        vTaskDelay(1000);
        Toggle_LED(CAR_HB, ON);
        ADC_Sense_Result ADC_Result = {0};
        if (Read_ADC(ADC_TIMEOUT_MS, &ADC_Result) != ADC_SENSE_OK)
        {
            // TODO: open contactors / safe state
            printf("ADC Sense Error\n");
        }

        int32_t Battery_Voltage = ADC_Result.Battery_Voltage;
        int32_t Motor_Voltage = ADC_Result.Motor_Voltage;

        printf("Motor: %ld mV | Battery: %ld mV\r\n",
               Motor_Voltage,
               Battery_Voltage);

        switch (State)
        {
        case PRECHARGE_STATE_INITIAL:
            // Startup state: Closes main contactor and moves to precharging state
            printf("In Initial State\r\n");
            if (contactor_set(MOTOR_CONTACTOR, CLOSED, 100, NORMAL) != SUCCESS)
            {
                // TODO: Fault handler
                printf("Main contactor didn't close\r\n");
                Fault_Handler();
            }
            State = PRECHARGE_STATE_PRECHARGING;

            // Start a timer for precharging
            Start_Tick = xTaskGetTickCount();
            break;
        case PRECHARGE_STATE_PRECHARGING:
            if (Motor_Voltage > (Battery_Voltage * 21 / 20)) // +5% tolerance
            {
                // Fault handler
                printf("Motor Voltage > Battery Voltage\r\n");
                Fault_Handler();
            }

            if (Battery_Voltage > OVERVOLTAGE_THRESHOLD_MV)
            {
                /* BATTERY ABOUT TO GO BOOM */
                // Fault handler
                printf("Overvoltage\r\n");
                Fault_Handler();
            }

            if (Battery_Voltage < UNDERVOLTAGE_THRESHOLD_MV)
            {
                /* Battery voltage is too low or battery is disconnected, treat as fault */
                // Fault handler
                printf("Undervoltage\r\n");
                Fault_Handler();
            }

            const TickType_t Current_Tick = xTaskGetTickCount();
            printf("In Precharging State\r\n");
            if ((Current_Tick - Start_Tick) > pdMS_TO_TICKS(PRECHARGE_TIMEOUT_MS)) // Faults if precharging takes too long
            {
                // Check if motor voltage is within 90% of battery voltage (precharge complete)
                if ((int64_t)Motor_Voltage * RATIO_SCALE >= (int64_t)Battery_Voltage * PRECHARGE_THRESHOLD_90)
                {
                    if (contactor_set(MOTOR_PRE_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, false) != SUCCESS)
                    {
                        // TODO: Fault handler
                        printf("Precharge contactor didn't close\r\n");
                        Fault_Handler();
                    }
                    State = PRECHARGE_STATE_RUN;
                }
                else
                {
                    // Precharging took too long
                    // Fault handler
                    printf("Precharge timeout\r\n");
                    Fault_Handler();
                }
            }
            break;
        case PRECHARGE_STATE_RUN:
            if (Motor_Voltage > (Battery_Voltage * 21 / 20)) // +5% tolerance
            {
                // Fault handler
                printf("Motor Voltage > Battery Voltage\r\n");
                Fault_Handler();
            }

            if (Battery_Voltage > OVERVOLTAGE_THRESHOLD_MV)
            {
                /* BATTERY ABOUT TO GO BOOM */
                // Fault handler
                printf("Overvoltage\r\n");
                Fault_Handler();
            }

            if (Battery_Voltage < UNDERVOLTAGE_THRESHOLD_MV)
            {
                /* Battery voltage is too low or battery is disconnected, treat as fault */
                // Fault handler
                printf("Undervoltage\r\n");
                Fault_Handler();
            }

            // Use 80% threshold for hysteresis
            printf("In Run State\r\n");
            if ((int64_t)Motor_Voltage * RATIO_SCALE < (int64_t)Battery_Voltage * PRECHARGE_THRESHOLD_80)
            {
            }
            break;
        default:
            break;
        }
    }
}