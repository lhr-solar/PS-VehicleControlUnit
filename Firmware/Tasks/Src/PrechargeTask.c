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

void Fault_Checker()
{
    if (Motor_Voltage > (Battery_Voltage * VOLTAGE_TOLERANCE))
    {
        // Fault handler
        printf("Fault: Motor Voltage > Battery Voltage\r\n");
        LED_set(MOTOR_FAULT, ON);   // TODO: Figure out which LED to turn on since there isn't a dedicated fault LED
        Fault_Handler();
    }

    if (Battery_Voltage > OVERVOLTAGE_THRESHOLD_MV)
    {
        /* BATTERY ABOUT TO GO BOOM */
        // Fault handler
        printf("Fault: Battery Voltage > 140 V\r\n");
        LED_set(MOTOR_FAULT, ON);   // TODO: Figure out which LED to turn on since there isn't a dedicated fault LED
        Fault_Handler();
    }

    if (Battery_Voltage < UNDERVOLTAGE_THRESHOLD_MV)
    {
        /* Battery voltage is too low or battery is disconnected, treat as fault */
        // Fault handler
        printf("Fault: Battery Voltage < 80 V\r\n");
        LED_set(MOTOR_FAULT, ON);   // TODO: Figure out which LED to turn on since there isn't a dedicated fault LED
        Fault_Handler();
    }
}

void Task_Precharge()
{
    Init_PrechargeTask();

    static Precharge_State_t State = PRECHARGE_STATE_INITIAL;
    static TickType_t Start_Tick = 0;

    while (1)
    {
        // TODO: Check fault bits -> call fault handler
        // TODO: Create a separate task to togggle heartbeat LED
        ADC_Sense_Result ADC_Result = {0};
        if (Read_ADC(ADC_TIMEOUT_MS, &ADC_Result) != ADC_SENSE_OK)
        {
            // TODO: open contactors / safe state
            printf("Fault: ADC Sense Error\n");
            LED_set(MOTOR_FAULT, ON);   // TODO: Figure out which LED to turn on since there isn't a dedicated fault LED
            Fault_Handler();
        }

        uint32_t Battery_Voltage = ADC_Result.Battery_Voltage;
        uint32_t Motor_Voltage = ADC_Result.Motor_Voltage;

        printf("Motor: %ld mV | Battery: %ld mV\r\n",
               Motor_Voltage,
               Battery_Voltage);

        switch (State)
        {
        case PRECHARGE_STATE_INITIAL:   // Startup state: Closes main contactor and moves to precharging state
            printf("Precharge State: Initial\r\n");
            if (contactor_set(MOTOR_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, NORMAL) != SUCCESS)
            {
                // TODO: Fault handler
                printf("Fault: Main contactor failed to close\r\n");
                LED_set(MOTOR_SENSE_TIMEOUT, ON);                
                Fault_Handler();
            }
            State = PRECHARGE_STATE_PRECHARGING;

            // Start a timer for precharging
            Start_Tick = xTaskGetTickCount();
            break;
        case PRECHARGE_STATE_PRECHARGING:   // Precharging state: Waits for battery voltage to reach 90% of motor voltage, then closes precharge contactor and moves to run state
            
            Fault_Checker();  // Check for faults while precharging, if any fault conditions are met, will call fault handler and not proceed with precharge sequence

            const TickType_t Current_Tick = xTaskGetTickCount();    // Check how long we've been precharging for, fault if not precharged after PRECHARGE_TIMEOUT_MS
            printf("Precharge State: Precharging\r\n");
            if ((Current_Tick - Start_Tick) > pdMS_TO_TICKS(PRECHARGE_TIMEOUT_MS)) // Faults if precharging takes too long
            {
                // Check if motor voltage is within 90% of battery voltage (precharge complete)
                if (Motor_Voltage * RATIO_SCALE >= Battery_Voltage * PRECHARGE_THRESHOLD_90)
                {
                    if (contactor_set(MOTOR_PRE_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, false) != SUCCESS)
                    {
                        // TODO: Fault handler
                        printf("Fault: Precharge contactor failed to close\r\n");
                        LED_set(PRECHARGE_SENSE_TIMEOUT, ON);
                        Fault_Handler();
                    }
                    State = PRECHARGE_STATE_RUN;
                }
                else
                {
                    // Precharging took too long
                    // Fault handler
                    printf("Fault: Precharge timeout\r\n");
                    LED_set(PRECHARGE_TIMEOUT, ON);
                    Fault_Handler();
                }
            }
            break;
        case PRECHARGE_STATE_RUN:   // Run state: Continuously checks that motor voltage stays within 80% of battery voltage
        
            Fault_Checker();  // Check for faults while precharging, if any fault conditions are met, will call fault handler and not proceed with precharge sequence

            // Use 80% threshold for hysteresis
            printf("Precharge State: Run\r\n");
            if (Motor_Voltage * RATIO_SCALE < Battery_Voltage * PRECHARGE_THRESHOLD_80)
            {
            }
            break;
        default:
            break;
        }

        vTaskDelay(1000);
    }
}