#include "CANbus.h"
#include "stm32xx_hal.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"
#include "MotorTelemetryTask.h"
#include "MotorControlTask.h"
#include "MotorSafeBits.h"

StaticTask_t wait_buffer;
StackType_t wait_stack[512];

StaticTask_t producer_buffer;
StackType_t producer_stack[512];

void waitTask(void *pvParameters){

    while(1){
        MotorSafeBits_Wait((MOTOR_CONTACTOR_ENABLED), portMAX_DELAY);
        Toggle_LED(HB);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void producerTask(void *pvParameters){


    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
        set_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);

        // vTaskDelay(pdMS_TO_TICKS(1000));
        // set_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);

    }
}


int main(){
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LEDs_init();
    
    Init_UART_Printf();

    MotorSafeBits_Init();


    xTaskCreateStatic(
                waitTask,
                "Task waiting for bits",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                wait_stack,
                &wait_buffer);

    xTaskCreateStatic(
                producerTask,
                "Task writting the bits",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                producer_stack,
                &producer_buffer);

    
    vTaskStartScheduler();

    while(1){

    }
    return 0;
}