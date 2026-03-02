#include "FaultHandlerTask.h"
#include "StatusLEDs.h"
#include "Contactors.h"
#include "FreeRTOS.h"
#include "task.h"
#include "PrechargeTask.h"  // for hprecharge_task handle

void Init_FaultHandlerTask() {
    if (faultBits_init() != 1) 
    {
        // Fault bit initialization failed
        while(1);
    }
}

void Kill_Precharge_Task() {
    if (hprecharge_task != NULL) {
        vTaskDelete(hprecharge_task);
    }
}

void Task_FaultHandler() {
    Init_FaultHandlerTask();

    while(1) {
        {
            EventBits_t result = faultBit_wait(NUM_FAULTS, portMAX_DELAY);

            printf("Fault Handler triggered with bitmask: 0x%02lX\r\n", result);
            vTaskDelay(100);

            switch (result) // compare against individual bitmasks
            {
                case FAULT_BIT(MOTOR_GREATER_THAN_BATTERY_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Motor Voltage Greater Than Battery Voltage\r\n");
                        LED_set(CAR_BPSFAULT, ON);
                        vTaskDelay(250);
                        LED_set(CAR_BPSFAULT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                case FAULT_BIT(BATTERY_OVERVOLTAGE_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Overvoltage\r\n");
                        LED_set(CAR_BPSFAULT, ON);
                        vTaskDelay(250);
                        LED_set(CAR_BPSFAULT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                case FAULT_BIT(BATTERY_UNDERVOLTAGE_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Undervoltage\r\n");
                        LED_set(CAR_BPSFAULT, ON);
                        vTaskDelay(250);
                        LED_set(CAR_BPSFAULT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                case FAULT_BIT(MOTOR_SENSE_TIMEOUT_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Motor Sense Timeout\r\n");
                        LED_set(MOTOR_SENSE_TIMEOUT, ON);
                        vTaskDelay(250);
                        LED_set(MOTOR_SENSE_TIMEOUT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                case FAULT_BIT(PRECHARGE_SENSE_TIMEOUT_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Precharge Sense Timeout\r\n");
                        LED_set(PRECHARGE_SENSE_TIMEOUT, ON);
                        vTaskDelay(250);
                        LED_set(PRECHARGE_SENSE_TIMEOUT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                case FAULT_BIT(PRECHARGE_TIMEOUT_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Precharge Sequence Timeout\r\n");
                        LED_set(PRECHARGE_TIMEOUT, ON);
                        vTaskDelay(250);
                        LED_set(PRECHARGE_TIMEOUT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                case FAULT_BIT(CALLBACK_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Contactor Sense Fault\r\n");
                        LED_set(CAR_BPSFAULT, ON);
                        vTaskDelay(250);
                        LED_set(CAR_BPSFAULT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                case FAULT_BIT(MOTOR_SENSE_MISMATCH_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Motor Sense Mismatch\r\n");
                        LED_set(CAR_BPSFAULT, ON);
                        vTaskDelay(250);
                        LED_set(CAR_BPSFAULT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                case FAULT_BIT(PRECHARGE_SENSE_MISMATCH_FAULT):
                    Kill_Precharge_Task();
                    contactor_emergency_open_all();
                    while (1) {
                        printf("Fault: Precharge Sense Mismatch\r\n");
                        LED_set(CAR_BPSFAULT, ON);
                        vTaskDelay(250);
                        LED_set(CAR_BPSFAULT, OFF);
                        vTaskDelay(1000);
                    }
                    break;
                default:
                    break;
            }
        }

        vTaskDelay(1000);
    }
}