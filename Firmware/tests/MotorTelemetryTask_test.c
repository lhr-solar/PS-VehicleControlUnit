#include "CANbus.h"
#include "stm32xx_hal.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"
#include "MotorTelemetryTask.h"

#define PRINTF_DEBUG

StaticTask_t task_buffer;
StackType_t task_stack[512];

void can_error_handler(){
    
    while(1){
        LED_set(MOTOR_FAULT, GPIO_PIN_SET);
    }
}


int main(){
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LEDs_init();


    if(Motor_CANBus_Init() != CAN_OK){
        can_error_handler();
    }

    MotorTelemetryTask_Init();

    Init_UART_Printf();

    xTaskCreateStatic(
                Task_MotorTelemetry,
                "Motor Telemetry Task",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                task_stack,
                &task_buffer);

    
    vTaskStartScheduler();

    while(1){

    }
}
