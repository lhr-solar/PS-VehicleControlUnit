#include "ADC_Sense.h"
#include "PrechargeTask.h"
#include "Contactors.h"
#include "stm32xx_hal.h"
#include "pinDefs.h"
#include "FaultBits.h"

/* handle for the Precharge task, defined here */
TaskHandle_t hprecharge_task = NULL;

StaticEventGroup_t xPrechargeEventGroup;
EventGroupHandle_t xPrechargeEventGroup_handle;

void Init_PrechargeTask()
{
    // Event Group init
    xPrechargeEventGroup_handle = xEventGroupCreateStatic(&xPrechargeEventGroup);
    configASSERT(xPrechargeEventGroup_handle); // check if handle is set
    // xEventGroupClearBits(xReadADCEventGroup_handle,    /* The event group being updated. */
    //                      0xFF );                    /* The bits being cleared. */

    // Inits ADC & contactors
    ADC_Sense_Init();
    contactor_init();
}

void Fault_Checker(uint32_t Motor_Voltage, uint32_t Battery_Voltage)
{
    if (Motor_Voltage > (Battery_Voltage * VOLTAGE_TOLERANCE_NUMERATOR / VOLTAGE_TOLERANCE_DENOMINATOR))
    {
        // Fault handler
        set_faultBit(MOTOR_GREATER_THAN_BATTERY_FAULT);
    }

    if (Battery_Voltage > OVERVOLTAGE_THRESHOLD_MV)
    {
        /* BATTERY ABOUT TO GO BOOM */
        // Fault handler
        set_faultBit(BATTERY_OVERVOLTAGE_FAULT);
    }

    if (Battery_Voltage < UNDERVOLTAGE_THRESHOLD_MV)
    {
        /* Battery voltage is too low or battery is disconnected, treat as fault */
        // Fault handler
        set_faultBit(BATTERY_UNDERVOLTAGE_FAULT);
    }

    if (contactor_get_sense(MOTOR_CONTACTOR) != contactor_get_state(MOTOR_CONTACTOR))
    {
        // Fault handler
        set_faultBit(MOTOR_SENSE_MISMATCH_FAULT);
    }

    if (contactor_get_sense(MOTOR_PRE_CONTACTOR) != contactor_get_state(MOTOR_PRE_CONTACTOR))
    {
        // Fault handler
        set_faultBit(PRECHARGE_SENSE_MISMATCH_FAULT);
    }

    printf("Motor Sense Pin Reading: %d\r\n", contactor_get_sense(MOTOR_CONTACTOR));
    printf("Precharge Sense Pin Reading: %d\r\n", contactor_get_sense(MOTOR_PRE_CONTACTOR));
    printf("Motor Contactor State: %d\r\n", contactor_get_state(MOTOR_CONTACTOR));
    printf("Precharge Contactor State: %d\r\n", contactor_get_state(MOTOR_PRE_CONTACTOR));
}

void Task_Precharge()
{
    Init_PrechargeTask();

    static Precharge_State_t State = PRECHARGE_STATE_INITIAL;
    static TickType_t Start_Tick = 0;

    while (1)
    {
        ADC_Sense_Result ADC_Result = {0};
        if (Read_ADC(ADC_TIMEOUT_MS, &ADC_Result) != ADC_SENSE_OK)
        {
            Error_Handler();
        }

        uint32_t Battery_Voltage = ADC_Result.Battery_Voltage;
        uint32_t Motor_Voltage = ADC_Result.Motor_Voltage;

        printf("Motor: %ld mV | Battery: %ld mV\r\n",
               Motor_Voltage,
               Battery_Voltage);

        switch (State)
        {
        case PRECHARGE_STATE_INITIAL: // Startup state: Closes main contactor and moves to precharging state
            printf("Precharge State: Initial\r\n");
            if (contactor_set(MOTOR_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, NORMAL) != SUCCESS)
            {
                set_faultBit(MOTOR_SENSE_TIMEOUT_FAULT);
            }
            State = PRECHARGE_STATE_PRECHARGING;

            // Start a timer for precharging
            Start_Tick = xTaskGetTickCount();
            break;
        case PRECHARGE_STATE_PRECHARGING: // Precharging state: Waits for battery voltage to reach 90% of motor voltage, then closes precharge contactor and moves to run state

            Fault_Checker(Motor_Voltage, Battery_Voltage); // Check for faults while precharging, if any fault conditions are met, will call fault handler and not proceed with precharge sequence

            const TickType_t Current_Tick = xTaskGetTickCount(); // Check how long we've been precharging for, fault if not precharged after PRECHARGE_TIMEOUT_MS
            printf("Precharge State: Precharging\r\n");
            if ((Current_Tick - Start_Tick) > pdMS_TO_TICKS(PRECHARGE_TIMEOUT_MS)) // Faults if precharging takes too long
            {
                // Check if motor voltage is within 90% of battery voltage (precharge complete)
                if (Motor_Voltage * RATIO_SCALE >= Battery_Voltage * PRECHARGE_THRESHOLD_90)
                {
                    if (contactor_set(MOTOR_PRE_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, false) != SUCCESS)
                    {
                        set_faultBit(PRECHARGE_SENSE_TIMEOUT_FAULT);
                    }
                    State = PRECHARGE_STATE_RUN;
                }
                else
                {
                    // Precharging took too long
                    set_faultBit(PRECHARGE_TIMEOUT_FAULT);
                }
            }
            break;
        case PRECHARGE_STATE_RUN: // Run state: Continuously checks that motor voltage stays within 80% of battery voltage

            Fault_Checker(Motor_Voltage, Battery_Voltage); // Check for faults while precharging, if any fault conditions are met, will call fault handler and not proceed with precharge sequence

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