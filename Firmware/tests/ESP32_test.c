#include "StatusLEDs.h"
#include "pinDefs.h"
#include "UART.h"
#include "stm32xx_hal.h"
#include "UART_Init.h"
#include "ESP32.h"

StaticTask_t txTaskBuffer;
StackType_t txTaskStack[configMINIMAL_STACK_SIZE];

void TxTask(void *argument){

    uint8_t testData[] = "Test Message 123\r\n";
    const uint8_t msgLen = sizeof(testData) - 1;

    while(1){
        uart_send(hlpuart1, testData, msgLen, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {

    HAL_Init();
    SystemClock_Config();

    ESP32_UART_Init();

    xTaskCreateStatic(TxTask, 
                     "TX",
                     configMINIMAL_STACK_SIZE,
                     NULL,
                     tskIDLE_PRIORITY + 2,
                     txTaskStack,
                     &txTaskBuffer);

    vTaskStartScheduler();

    while (1) {
    }
}