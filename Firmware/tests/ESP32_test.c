#include "StatusLEDs.h"
#include "pinDefs.h"
#include "ESP32.h"
#include "printf.h"
#include <stdio.h>
#include "ADC_Sense.h"

// StaticTask_t txTaskBuffer;
// StackType_t txTaskStack[configMINIMAL_STACK_SIZE];

// StaticTask_t rxTaskBuffer;
// StackType_t rxTaskStack[configMINIMAL_STACK_SIZE];

// uint8_t messageNum = 0;

// void TxTask(void *argument){

//     uint8_t testData[] = "\r\nTest Message 123\r\n";
//     const uint8_t msgLen = sizeof(testData) - 1;
//     uart_status_t lpuartStatus = UART_ERR;

//     while(1){
//     lpuartStatus = ESP32_Send(testData, msgLen, portMAX_DELAY);
//     if(lpuartStatus != UART_ERR){
//         Toggle_LED(HB, ON);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         Toggle_LED(HB, OFF);
//     }else{
//         Toggle_LED(MOTOR_FAULT, ON);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         Toggle_LED(MOTOR_FAULT, OFF);
//     }
//     // printf("%s", testData);
//     vTaskDelay(pdMS_TO_TICKS(1000));
//         // vTaskDelay(pdMS_TO_TICKS(500));
//         // messageNum++;
//         // static char buffer[50];  // storage for the message
//         // snprintf(buffer, sizeof(buffer), "Message: #%d\r\n", messageNum);
//         // uint8_t testmessage[50];
//         // for(int i =0;i<sizeof(buffer); i++){
//         //     testmessage[i] = buffer[i];
//         // }
//         // ESP32_Send(testmessage, sizeof(testmessage)-1, portMAX_DELAY);
//         // //print ADC voltages

//         // vTaskDelay(pdMS_TO_TICKS(500)); 
//         // ADC_Sense_Result ADC_Result = {0};

//         // if (Read_ADC(20, &ADC_Result) != ADC_SENSE_OK)
//         // {
//         //     uint8_t errorData[] = "ADC Error\r\n";
//         //     const uint8_t errorMsgLen = sizeof(errorData) - 1;
//         //     ESP32_Send( errorData, errorMsgLen, portMAX_DELAY);
//         //     Error_Handler();
//         // }else{
//         //     uint8_t adctestData[] = "ADC read\r\n";
//         //     const uint8_t adcmsgLen = sizeof(adctestData) - 1;
//         //     ESP32_Send( adctestData, adcmsgLen, portMAX_DELAY);
//         // }
//         // char buf[34];
//         // snprintf(buf, 34, "Motor: %ldmV | Battery: %ldmV\r\n", ADC_Result.Motor_Voltage, ADC_Result.Battery_Voltage);
//         // uint8_t adcData[34];
//         // for(int i =0;i<sizeof(buf); i++){
//         //     adcData[i]= buf[i];
//         // }
//         // static char buffer2[50];  // storage for the message
//         // snprintf(buffer, sizeof(buffer2), "Motor: mV | Battery: mV\r\n");
//         // uint8_t adcData[50];
//         // for(int i =0;i<sizeof(buffer2); i++){
//         //     adcData[i] = buffer2[i];
//         // }
//         // const uint8_t adcMsgLen = sizeof(testData) - 1;
//         // ESP32_Send(adcData, adcMsgLen, portMAX_DELAY);
//         // // printf("Motor: %ldmV | Battery: %ldmV\r\n", ADC_Result.Motor_Voltage, ADC_Result.Battery_Voltage); 
        
//         // //dummy send
//         // uint8_t dummy[] = "\r\n";
//         // ESP32_Send(dummy, 2, portMAX_DELAY);
//     }
// }

// void RxTask(void *argument){

//     uint8_t rxBuffer;
//     uint32_t rxCount = 0;

//     while(1){
//         uart_status_t status = ESP32_Recv(&rxBuffer, 1, portMAX_DELAY);

//         if (status == UART_RECV) {
//             rxCount++;

//             // Mirror received character over USB
//             printf("RX[%lu]: %c\n\r", rxCount, rxBuffer); 
//             }
//     }
// }

// int main(void) {

//     HAL_Init();
//     // MX_GPIO_Init();
//     SystemClock_Config();

    // ESP32_UART_Init();
    // Init_UART_Printf();
    // ADC_Sense_Init();

//     xTaskCreateStatic(TxTask, 
//                      "TX",
//                      configMINIMAL_STACK_SIZE,
//                      NULL,
//                      tskIDLE_PRIORITY + 2,
//                      txTaskStack,
//                      &txTaskBuffer);
    
//     xTaskCreateStatic(TxTask, 
//                      "RX",
//                      configMINIMAL_STACK_SIZE,
//                      NULL,
//                      tskIDLE_PRIORITY + 2,
//                      rxTaskStack,
//                      &rxTaskBuffer);

//     vTaskStartScheduler();

//     // should never reach here
//     while (1) {
//         LED_set(HB, ON);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//         LED_set(HB, OFF);
//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

/* LPUART test

- Setups up LPUART loopback mode (the TX line is connected to the RX line)
- Send messages and verify correctness
*/


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
    // const uint8_t msgLen = sizeof(testData) - 1;
    uint32_t txCount = 0;
    uart_status_t status = UART_ERR;
    
    while(1) {
        // Send test message
        // status = uart_send(hlpuart1, testData, msgLen, portMAX_DELAY);
        static char buffer[50];  // storage for the message
        snprintf(buffer, sizeof(buffer), "Message: #%ld\r\n", txCount);
        uint8_t testmessage[50];
        for(int i =0;i<sizeof(buffer); i++){
            testmessage[i] = buffer[i];
        }
        status = ESP32_Send(testmessage, sizeof(testmessage)-1, portMAX_DELAY);
        if (status == UART_SENT) {
            txCount++;
            // Toggle LED to indicate successful transmission
            HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
            // printf("sucesfully transmitted!\n\r");
        }else{
            HAL_GPIO_TogglePin(FAULT_LED_PORT, FAULT_LED_PIN);
        }
        ADC_Sense_Result ADC_Result = {0};

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
        char buf[50];
        snprintf(buf, sizeof(buf), "Motor: %ldmV | Battery: %ldmV\r\n", ADC_Result.Motor_Voltage, ADC_Result.Battery_Voltage);
        uint8_t adcData[50];    
        for(int i =0;i<sizeof(buf); i++){
            adcData[i]= buf[i];
        }
        const uint8_t adcMsgLen = sizeof(adcData) - 1;
        ESP32_Send(adcData, adcMsgLen, portMAX_DELAY);
        
        static char buffer[50];  // storage for the message
        snprintf(buffer, sizeof(buffer), "Message: #%ld\r\n", txCount);
        uint8_t testmessage[50];
        for(int i =0;i<sizeof(buffer); i++){
            testmessage[i] = buffer[i];
        }
        status = ESP32_Send(testmessage, sizeof(testmessage)-1, portMAX_DELAY);

        // uint8_t adcMsg1[] = "Motor: ";
        // const uint8_t adcMsg1Len = sizeof(adcMsg1) - 1;
        // ESP32_Send(adcMsg1, adcMsg1Len, portMAX_DELAY);
        // uint8_t adcMsg1[];
        // const uint8_t adcMsg1Len = sizeof(testData) - 1;
        // ESP32_Send(adcMsg1, adcMsg1Len, portMAX_DELAY);

        
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
    HAL_GPIO_Init(HB_LED_PORT, &led_config); // initialize GPIOA with led_config

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

