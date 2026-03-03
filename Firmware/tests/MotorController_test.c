#include "CANbus.h"
#include "stm32xx_hal.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"
#include "MotorTelemetryTask.h"
#include "MotorControlTask.h"

#define PRINTF_DEBUG

StaticTask_t motorTelemetry_buffer;
StackType_t motorTelemetry_stack[512];

StaticTask_t motorControllerTask_buffer;
StackType_t motorControllerTask_stack[512];

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


    if(Motor_CANBus_Init() != CAN_OK){
        can_error_handler();
    }

    MotorTelemetryTask_Init();
    MotorControlTask_Init();

    Init_UART_Printf();

    xTaskCreateStatic(
                Task_MotorTelemetry,
                "Motor Telemetry Task",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                motorTelemetry_stack,
                &motorTelemetry_buffer);

    xTaskCreateStatic(
                Task_MotorControl,
                "Motor Control Task",
                512,
                NULL,
                tskIDLE_PRIORITY + 4,
                motorControllerTask_stack,
                &motorControllerTask_buffer);

    
    vTaskStartScheduler();

    while(1){

    }
}
