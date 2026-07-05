#include "stm32xx_hal.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"
#include "MotorSafeBits.h"

static const EventBits_t motorDrivableBits = MOTOR_STATUS_BIT(MOTOR_CONTACTOR_ENABLED)
                                            | MOTOR_STATUS_BIT(MOTOR_PRECHARGE_CONTACTOR_ENABLED);

StaticTask_t wait_buffer;
StackType_t wait_stack[512];

StaticTask_t producer_buffer;
StackType_t producer_stack[512];

void waitTask(void *pvParameters){

    while(1){
        MotorSafeBits_WaitMask((motorDrivableBits), portMAX_DELAY);
        LED_toggle(CAR_DRIVABLE);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void producerTask(void *pvParameters){


    while(1){
        vTaskDelay(pdMS_TO_TICKS(5000));

        set_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);
        LED_set(HB, LED_ON);

        vTaskDelay(pdMS_TO_TICKS(5000));
        set_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);
        LED_set(CAR_HB, LED_ON);

        vTaskDelay(pdMS_TO_TICKS(10000));
        clear_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);
        LED_set(HB, LED_OFF);
        LED_set(CAR_HB, LED_OFF);
    }
}


int main(){
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LED_init();
    
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