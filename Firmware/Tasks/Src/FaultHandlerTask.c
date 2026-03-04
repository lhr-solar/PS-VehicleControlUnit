#include "FaultHandlerTask.h"
#include "PrechargeTask.h" // for hprecharge_task handle

EventBits_t fault_bits = 0;

void Init_FaultHandlerTask()
{
    if (faultBits_init() != 1)
    {
        // Fault bit initialization failed
        Error_Handler();
    }
}

void Kill_Precharge_Task()
{
    if (hprecharge_task != NULL)
    {
        vTaskDelete(hprecharge_task);
    }
}

void Fault_Loop()
{
    while (1)
    {
        switch (fault_bits) // compare against individual bitmasks
        {
        case FAULT_BIT(MOTOR_GREATER_THAN_BATTERY_FAULT):
            printf("Fault: Motor Voltage Greater Than Battery Voltage\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(BATTERY_OVERVOLTAGE_FAULT):
            printf("Fault: Overvoltage\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(BATTERY_UNDERVOLTAGE_FAULT):
            printf("Fault: Undervoltage\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(MOTOR_SENSE_TIMEOUT_FAULT):
            printf("Fault: Motor Sense Timeout\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(PRECHARGE_SENSE_TIMEOUT_FAULT):
            printf("Fault: Precharge Sense Timeout\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(PRECHARGE_TIMEOUT_FAULT):
            printf("Fault: Precharge Sequence Timeout\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(CALLBACK_FAULT):
            printf("Fault: Contactor Sense Fault\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(MOTOR_SENSE_MISMATCH_FAULT):
            printf("Fault: Motor Sense Mismatch\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(PRECHARGE_SENSE_MISMATCH_FAULT):
            printf("Fault: Precharge Sense Mismatch\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        default:
            break;
        }
    }
}

void Set_Fault_LED()
{
    switch (fault_bits) // compare against individual bitmasks
    {
    case FAULT_BIT(MOTOR_GREATER_THAN_BATTERY_FAULT):
        LED_set(CAR_BPSFAULT, ON);
        break;
    case FAULT_BIT(BATTERY_OVERVOLTAGE_FAULT):
        LED_set(CAR_BPSFAULT, ON);
        break;
    case FAULT_BIT(BATTERY_UNDERVOLTAGE_FAULT):
        LED_set(CAR_BPSFAULT, ON);
        break;
    case FAULT_BIT(MOTOR_SENSE_TIMEOUT_FAULT):
        LED_set(MOTOR_SENSE_TIMEOUT, ON);
        break;
    case FAULT_BIT(PRECHARGE_SENSE_TIMEOUT_FAULT):
        LED_set(PRECHARGE_SENSE_TIMEOUT, ON);
        break;
    case FAULT_BIT(PRECHARGE_TIMEOUT_FAULT):
        LED_set(PRECHARGE_TIMEOUT, ON);
        break;
    case FAULT_BIT(CALLBACK_FAULT):
        LED_set(CAR_BPSFAULT, ON);
        break;
    case FAULT_BIT(MOTOR_SENSE_MISMATCH_FAULT):
        LED_set(CAR_BPSFAULT, ON);
        break;
    case FAULT_BIT(PRECHARGE_SENSE_MISMATCH_FAULT):
        LED_set(CAR_BPSFAULT, ON);
        break;
    default:
        break;
    }
}

void Task_FaultHandler()
{
    Init_FaultHandlerTask();

    while (1)
    {
        {
            LED_set(HB, ON);
            vTaskDelay(500);
            LED_set(HB, OFF);
            vTaskDelay(500);

            fault_bits = faultBit_wait(NUM_FAULTS, portMAX_DELAY);

            if (fault_bits != 0)
            {
                Kill_Precharge_Task();
                contactor_emergency_open_all();

                printf("Fault Handler triggered with bitmask: 0x%02lX\r\n", fault_bits);

                Set_Fault_LED();
                Fault_Loop();
            }
        }

        vTaskDelay(1000);
    }
}