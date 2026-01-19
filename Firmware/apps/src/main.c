#include "stm32xx_hal.h"
#include "tasks.h"


void Task_CANWatchdog(void *arg);
void Task_FaultHandler(void *arg);

//attach this to fsm... and figure out all the defines/sizes
//Make the main FSM task, make watch dogs, then have a suspended task actiavted via event for faults, and fix all the OS errors
int main() {
    HAL_Init();
    SystemClock_Config();
    
    xTaskCreateStatic(
        Task_SendTritium, /* The function that implements the task. */
        "Init Task", /* Text name for the task. */
        configMINIMAL_STACK_SIZE, /* The size (in words) of the stack that should be created for the task. */
        (void*)NULL, /* Paramter passed into the task. */
        tskIDLE_PRIORITY + 2, /* Task Priority. */
        Task_FSM_Stack_Array, /* Stack array. */
        &Task_FSM_Buffer  /* Buffer for static allocation. */
   );

   //Make watchdogs here...

    xTaskCreateStatic(
        Task_CANWatchdog,              
        "CAN Watchdog",
        configMINIMAL_STACK_SIZE,
        (void*)NULL,
        tskIDLE_PRIORITY + 3,      //Higher than FSM  
        Task_Watchdog_Stack_Array,
        &Task_Watchdog_Buffer
    );

    //Fault task
    xTaskCreateStatic(
        Task_FaultHandler,          
        "Fault",
        configMINIMAL_STACK_SIZE,
        (void*)NULL,
        configMAX_PRIORITIES - 1,    /* highest priority */
        Task_Fault_Stack_Array,
        &Task_Fault_Buffer
    );



    // Start the scheduler
    vTaskStartScheduler();

    while(1){
        // Scheduler should've started by now
        // Code should never enter this point
    }
    
    return 0;
}