#include "StatusLEDs.h"
#include "pinDefs.h"
#include "UART.h"
#include "stm32xx_hal.h"
#include "UART_Init.h"
#include "ESP32.h"
#include "printf.h"
#include <stdio.h>
#include "ADC_Sense.h"

StaticTask_t txTaskBuffer;
StackType_t txTaskStack[configMINIMAL_STACK_SIZE];

StaticTask_t rxTaskBuffer;
StackType_t rxTaskStack[configMINIMAL_STACK_SIZE];

void TxTask(void *argument){

    uint8_t testData[] = "Test Message 123\r\n";
    const uint8_t msgLen = sizeof(testData) - 1;
    ESP32_Send( testData, msgLen, portMAX_DELAY);
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
        //print ADC voltages
        ADC_Sense_Result ADC_Result = {0};
        if (Read_ADC(1000, &ADC_Result) != ADC_SENSE_OK)
        {
            Error_Handler();
        }
        char buf[34];
        snprintf(buf, 34, "Motor: %ldmV | Battery: %ldmV\r\n", ADC_Result.Motor_Voltage, ADC_Result.Battery_Voltage);
        uint8_t adcData[34];
        for(int i =0;i<sizeof(buf); i++){
            adcData[i]= buf[i];
        }
        const uint8_t adcMsgLen = sizeof(adcData) -1;
        ESP32_Send(adcData, adcMsgLen, portMAX_DELAY);
        printf("Motor: %ldmV | Battery: %ldmV\r\n", ADC_Result.Motor_Voltage, ADC_Result.Battery_Voltage); 
        
    }
}

void RxTask(void *argument){

    uint8_t rxBuffer;
    uint32_t rxCount = 0;

    while(1){
        uart_status_t status = ESP32_Recv(&rxBuffer, 1, portMAX_DELAY);

        if (status == UART_RECV) {
            rxCount++;

            // Mirror received character over USB
            printf("RX[%lu]: %c\n\r", rxCount, rxBuffer); 
            }
    }
}

int main(void) {

    HAL_Init();
    SystemClock_Config();

    ESP32_UART_Init();
    Init_UART_Printf();


    xTaskCreateStatic(TxTask, 
                     "TX",
                     configMINIMAL_STACK_SIZE,
                     NULL,
                     tskIDLE_PRIORITY + 2,
                     txTaskStack,
                     &txTaskBuffer);
    
    xTaskCreateStatic(TxTask, 
                     "RX",
                     configMINIMAL_STACK_SIZE,
                     NULL,
                     tskIDLE_PRIORITY + 2,
                     rxTaskStack,
                     &rxTaskBuffer);

    vTaskStartScheduler();

    // should never reach here
    while (1) {

    }
}