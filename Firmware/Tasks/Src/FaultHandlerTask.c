#include "FaultHandlerTask.h"

void Init_FaultHandlerTask() {
    if (faultBits_init() != 1) 
    {
        // Fault bit initialization failed
        while(1);
    }

    Init_UART_Printf();
}

void Task_FaultHandler() {
    Init_FaultHandlerTask();

    while(1) {
        switch (faultBit_wait(NUM_FAULTS, portMAX_DELAY)) // Wait for any fault bit to be set
        {
            case MOTOR_GREATER_THAN_BATTERY_FAULT:
                printf("Fault: Motor Voltage Greater Than Battery Voltage\r\n");
                break;
            case BATTERY_OVERVOLTAGE_FAULT:
                printf("Fault: Battery Voltage Greater Than Threshold\r\n");
                break;
            case BATTERY_UNDERVOLTAGE_FAULT:
                printf("Fault: Battery Voltage Less Than Threshold\r\n");
                break;
            case MOTOR_SENSE_TIMEOUT_FAULT:
                printf("Fault: Motor Voltage Sense Timeout\r\n");
                break;
            case PRECHARGE_SENSE_TIMEOUT_FAULT:
                printf("Fault: Precharge Voltage Sense Timeout\r\n");
                break;
            case PRECHARGE_TIMEOUT_FAULT:
                printf("Fault: Precharge Sequence Timeout\r\n");
                break;
            default:
                break;
        }

        vTaskDelay(1000);
    }
}