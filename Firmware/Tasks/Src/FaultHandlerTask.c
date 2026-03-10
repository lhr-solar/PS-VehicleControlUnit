#include "FaultHandlerTask.h"
#include "PrechargeTask.h" // for hprecharge_task handle

#define FAULT_LOOP_PRINTF_DELAY_MS 10000

#define FAULT_PRINTF_COUNTER (FAULT_LOOP_PRINTF_DELAY_MS/FAULT_LOOP_PERIOD_MS)

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

static void print_fault(){
    switch (fault_bits) // compare against individual bitmasks
        {
            case FAULT_BIT(MOTOR_GREATER_THAN_BATTERY_FAULT):
                printf("Fault: Motor Voltage Greater Than Battery Voltage\r\n");
                break;
            case FAULT_BIT(BATTERY_OVERVOLTAGE_FAULT):
                printf("Fault: Battery Overvoltage\r\n");
                break;
            case FAULT_BIT(BATTERY_UNDERVOLTAGE_FAULT):
                printf("Fault: Battery Undervoltage\r\n");
                break;
            case FAULT_BIT(MOTOR_SENSE_TIMEOUT_FAULT):
                printf("Fault: Motor Sense Timeout\r\n");
                break;
            case FAULT_BIT(PRECHARGE_SENSE_TIMEOUT_FAULT):
                printf("Fault: Precharge Sense Timeout\r\n");
                break;
            case FAULT_BIT(PRECHARGE_TIMEOUT_FAULT):
                printf("Fault: Precharge Sequence Timeout\r\n");
                break;
            case FAULT_BIT(CALLBACK_FAULT):
                printf("Fault: Contactor Sense Fault\r\n");
                break;
            case FAULT_BIT(MOTOR_SENSE_MISMATCH_FAULT):
                printf("Fault: Motor Sense Mismatch\r\n");
                break;
            case FAULT_BIT(PRECHARGE_SENSE_MISMATCH_FAULT):
                printf("Fault: Precharge Sense Mismatch\r\n");
                break;
            default:
                printf("Fault: Unknown\r\n");
                break;
        }
}

void Fault_Loop()
{
    uint32_t fault_printf_debug_counter = 0;
    while (1)
    {
        fault_printf_debug_counter++;

        if(fault_printf_debug_counter >= FAULT_PRINTF_COUNTER){
            print_fault();
            fault_printf_debug_counter = 0;
        }

        vTaskDelay(FAULT_LOOP_PERIOD_MS);

    }
}

void Set_Fault_LED()
{
    switch (fault_bits) // compare against individual bitmasks
    {
    case FAULT_BIT(MOTOR_GREATER_THAN_BATTERY_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(BATTERY_OVERVOLTAGE_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(BATTERY_UNDERVOLTAGE_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(MOTOR_SENSE_TIMEOUT_FAULT):
        LED_set(MOTOR_SENSE_TIMEOUT, LED_ON);
        break;
    case FAULT_BIT(PRECHARGE_SENSE_TIMEOUT_FAULT):
        LED_set(PRECHARGE_SENSE_TIMEOUT, LED_ON);
        break;
    case FAULT_BIT(PRECHARGE_TIMEOUT_FAULT):
        LED_set(PRECHARGE_TIMEOUT, LED_ON);
        break;
    case FAULT_BIT(CALLBACK_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(MOTOR_SENSE_MISMATCH_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(PRECHARGE_SENSE_MISMATCH_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
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
        fault_bits = faultBit_wait(NUM_FAULTS, portMAX_DELAY);

        if (fault_bits != 0)
        {
            Kill_Precharge_Task();
            contactor_emergency_open_all();

            // prevents the motor from running
            clear_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);
            clear_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);


            printf("Fault Handler triggered with bitmask: 0x%02lX\r\n", fault_bits);

            Set_Fault_LED();
            Fault_Loop();
        }
        
        vTaskDelay(1000);
    }
}