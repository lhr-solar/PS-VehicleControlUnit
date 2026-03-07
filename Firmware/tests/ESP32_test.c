#include "StatusLEDs.h"
#include "pinDefs.h"
#include "UART.h"
#include "stm32xx_hal.h"
#include "UART_Init.h"
#include "ESP32.h"
#include "printf.h"

StaticTask_t txTaskBuffer;
StackType_t txTaskStack[configMINIMAL_STACK_SIZE];

StaticTask_t rxTaskBuffer;
StackType_t rxTaskStack[configMINIMAL_STACK_SIZE];

void TxTask(void *argument){

    uint8_t testData[] = "Test Message 123\r\n";
    const uint8_t msgLen = sizeof(testData) - 1;

    while(1){
        ESP32_Send( testData, msgLen, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
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