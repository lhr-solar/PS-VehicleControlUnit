#include "stm32xx_hal.h"
#include "pinDefs.h"
#include "StatusLEDs.h"
#include "User_SDCard.h"

StaticTask_t task_buffer;
StackType_t task_stack[512];

static void task(void *pvParameters){

    if(SDCard_Init() != SD_OK){
        LED_set(MOTOR_FAULT, LED_ON);
    }

    char *msg = "balls\r\n";

    while(1){
        if(SDCard_Write("MOTOR.TXT", msg, pdMS_TO_TICKS(portMAX_DELAY)) != SD_OK){
            LED_set(CAR_BPSFAULT, LED_ON);
        }
        Toggle_LED(HB);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(){
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LEDs_init();

    xTaskCreateStatic(
                task,
                "task",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                task_stack,
                &task_buffer);



    vTaskStartScheduler();

    while(1){

    }
}