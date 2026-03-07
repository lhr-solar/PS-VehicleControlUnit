#include "StatusLEDs.h"
#include "pinDefs.h"
#include "inits.h"
#include "UART.h"
#include "projdefs.h"
#include "stm32xx_hal.h"
#include "printf.h"
#include "UART_Init.h"

StaticTask_t txTaskBuffer;
StackType_t txTaskStack[configMINIMAL_STACK_SIZE];

void TxTask(void *argument){

    while(1){
        printf("Hello World! %s %d %f\n\r", "Test String", 5, 4.4);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {

    HAL_Init();
    SystemClock_Config();

    Init_UART_Printf();

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