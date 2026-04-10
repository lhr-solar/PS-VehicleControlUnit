/**
 * VCU status test. 
 * 
 * Only enables vcu status task. This basically tests if the msg works (not 
 * really its interaction with other stuff).
 * 
 * 
 */

#include "VCUStatusTask.h"
#include "inits.h"
#include "InitTask.h"
#include "FSM.h"
// #include ""

#define PRINTF_DEBUG

void can_error_handler() {
    while (1) {
        HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
        HAL_Delay(500);
    }
}

int main() {
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LED_init();

    if (CarCAN_Init() != CAN_OK) {
        can_error_handler();
    }

    Init_UART_Printf();

    faults_init();
    contactor_init();
    FSM_TaskInit();
    fsm_init();

    xTaskCreateStatic(
        Task_BroadcastVCUStatus, 
        "VCU status tx", 
        VCU_STATUS_TASK_STACK_SIZE, 
        NULL, 
        VCU_STATUS_THREAD_PRIO,
        VCUStatus_Task_Stack, 
        &VCUStatus_Task_Buffer
    );

    vTaskStartScheduler();

    while (1) {
    }
}
