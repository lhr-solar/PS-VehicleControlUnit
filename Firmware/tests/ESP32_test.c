#include "StatusLEDs.h"
#include "pinDefs.h"
#include "ESP32.h"
#include "printf.h"
#include "ADC_Sense.h"
#include <stdlib.h>

uint8_t printf_enabled = 0;

void TxTask(void *argument);
void RxTask(void *argument);

// Static task creation resources
StaticTask_t txTaskBuffer;
StaticTask_t rxTaskBuffer;
StackType_t txTaskStack[configMINIMAL_STACK_SIZE];
StackType_t rxTaskStack[configMINIMAL_STACK_SIZE];

void TxTask(void *argument){

    const TickType_t xDelay = pdMS_TO_TICKS(1000);  // 1 second delay
    // uint8_t testData[] = "Test Message 123\r\n";
    uint8_t msgLen = 0;
    uint8_t newLine[] = "\r\n";
    const uint8_t newLineLen = sizeof(newLine) - 1;

    uint16_t txCount = 0;
    uart_status_t status = UART_ERR;
    // static char buffer[50];  // storage for the message
    // uint8_t testmessage[50];
    ADC_Sense_Result ADC_Result = {0};
    while(1) {
        // Send test message
        uint8_t startMsg1[] = "Message: #";
        msgLen = sizeof(startMsg1) - 1;
        status = ESP32_Send(startMsg1, msgLen, portMAX_DELAY);

        uint8_t startMsg2[6];
        utoa(txCount, (char*) startMsg2, 10);
        msgLen = sizeof(startMsg2) - 1;
        status = ESP32_Send(startMsg2, msgLen, portMAX_DELAY);

        status = ESP32_Send(newLine, newLineLen, portMAX_DELAY);
        if (status == UART_OK) {
            txCount++;
            // Toggle LED to indicate successful transmission
            HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
            // printf("sucesfully transmitted!\n\r");
        }else{
            HAL_GPIO_TogglePin(FAULT_LED_PORT, FAULT_LED_PIN);
        }

        if (Read_ADC(20, &ADC_Result) != ADC_SENSE_OK)
        {
            uint8_t errorData[] = "ADC Error\r\n";
            const uint8_t errorMsgLen = sizeof(errorData) - 1;
            ESP32_Send( errorData, errorMsgLen, portMAX_DELAY);
            Error_Handler();
        }else{
            uint8_t adctestData[] = "ADC read\r\n";
            const uint8_t adcmsgLen = sizeof(adctestData) - 1;
            ESP32_Send( adctestData, adcmsgLen, portMAX_DELAY);
        }

        // adc message construction
        uint8_t adcMsg1[] = "Motor: ";
        msgLen = sizeof(adcMsg1) - 1;
        status = ESP32_Send(adcMsg1, msgLen, portMAX_DELAY);

        uint8_t adcMsg2[6];
        utoa((uint16_t) ADC_Result.Motor_Voltage, (char*) adcMsg2, 10);
        msgLen = sizeof(adcMsg2) - 1;
        status = ESP32_Send(adcMsg2, msgLen, portMAX_DELAY);

        uint8_t adcMsg3[] = " | Battery: ";
        msgLen = sizeof(adcMsg3) - 1;
        status = ESP32_Send(adcMsg3, msgLen, portMAX_DELAY);

        uint8_t adcMsg4[6];
        utoa((uint16_t) ADC_Result.Battery_Voltage, (char*) adcMsg4, 10);
        msgLen = sizeof(adcMsg4) - 1;
        status = ESP32_Send(adcMsg4, msgLen, portMAX_DELAY);

        status = ESP32_Send(newLine, newLineLen, portMAX_DELAY);

        
        vTaskDelay(xDelay);
    }
}

void RxTask(void *argument){

    uint8_t rxBuffer;
    uint32_t rxCount = 0;

    while (1) {
        uart_status_t status = UART_ERR;
        status = uart_recv(hlpuart1, &rxBuffer, 1, portMAX_DELAY);

        if (status == UART_RECV) {
            rxCount++;

            // Print received character
            if(printf_enabled == 1){
                printf("RX[%lu]: %c\n\r", rxCount, rxBuffer);        
            }
        }

    }
}

int main(void) {

    HAL_Init();
    SystemClock_Config();

    GPIO_InitTypeDef led_config = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = HB_LED_PIN
    };
    
    // Heartbeat_Clock_Init(); // enable clock for LED_PORT
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_Init(HB_LED_PORT, &led_config); // initialize with led_config

    ESP32_UART_Init();
    ADC_Sense_Init();

    // Create the tasks statically
    xTaskCreateStatic(TxTask, 
                     "TX",
                     configMINIMAL_STACK_SIZE,
                     NULL,
                     tskIDLE_PRIORITY + 2,
                     txTaskStack,
                     &txTaskBuffer);

    xTaskCreateStatic(RxTask,
                     "RX", 
                     configMINIMAL_STACK_SIZE,
                     NULL,
                     tskIDLE_PRIORITY + 2,
                     rxTaskStack,
                     &rxTaskBuffer);

    // Start the scheduler
    vTaskStartScheduler();

    while (1) {
        // Should never get here
    }
}

