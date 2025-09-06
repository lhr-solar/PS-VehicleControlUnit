#include "FreeRTOS.h" /* Must come first. */
#include "stm32xx_hal.h"
#include "tasks.h"

StackType_t Task_Init_Stack_Array[ TASK_INIT_STACK_SIZE ];
StaticTask_t Task_Init_Buffer;

int main(void) {

    HAL_MspInit();
    SystemClock_Config();

    xTaskCreateStatic(
                    Task_Init,
                    "Init Task",
                    configMINIMAL_STACK_SIZE,
                    (void*)NULL,
                    TASK_INIT_PRIO,
                    Task_Init_Stack_Array,
                    &Task_Init_Buffer
   );

    // Start the scheduler
    vTaskStartScheduler();

    while(1){

    }
    
    return 0;
}
