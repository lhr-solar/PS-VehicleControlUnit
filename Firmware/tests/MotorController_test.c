#include "CANbus.h"
#include "stm32xx_hal.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"
#include "MotorTelemetryTask.h"
#include "MotorControlTask.h"

#define PRINTF_DEBUG


void can_error_handler(){
    
    while(1){
        HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
        HAL_Delay(500);
    }
}


int main(){
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LEDs_init();


    if(CAN_Init() != CAN_OK){
        can_error_handler();
    }

    MotorTelemetryTask_Init();
    MotorControlTask_Init();

    MotorSafeBits_Init();


    Init_UART_Printf();

    xTaskCreateStatic(
                Task_MotorTelemetry,
                "Motor Telemetry Task",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                Motor_Telemetry_Task_Stack,
                &Motor_Telemetry_Task_Buffer);

    xTaskCreateStatic(
                Task_MotorControl,
                "Motor Control Task",
                512,
                NULL,
                tskIDLE_PRIORITY + 4,
                Motor_Control_Task_Stack,
                &Motor_Control_Task_Buffer);

    
    vTaskStartScheduler();

    while(1){

    }
}
