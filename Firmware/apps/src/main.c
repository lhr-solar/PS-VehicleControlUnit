#include "stm32xx_hal.h"
#include "tasks.h"
#include "SendTritium.h"


void Task_CANWatchdog(void *arg);
void Task_FaultHandler(void *arg);
void Task_BroadcastMotorStatus(void *p_arg);

//attach this to fsm... and figure out all the defines/sizes
//Make the main FSM task, make watch dogs, then have a suspended task actiavted via event for faults, and fix all the OS errors
int main() {
    HAL_Init();
    SystemClock_Config();

    initStatusEventGroup();
    
    xTaskCreateStatic(
        Task_SendTritium, /* The function that implements the task. */
        "Init Task", /* Text name for the task. */
        configMINIMAL_STACK_SIZE, /* The size (in words) of the stack that should be created for the task. */
        (void*)NULL, /* Paramter passed into the task. */
        tskIDLE_PRIORITY + 2, /* Task Priority. */
        Task_FSM_Stack_Array, /* Stack array. */
        &Task_FSM_Buffer  /* Buffer for static allocation. */
   );

   xTaskCreateStatic(
        Task_UpdateControlStatus, /* The function that implements the task. */
        "Update Control Status Task", /* Text name for the task. */
        configMINIMAL_STACK_SIZE, /* The size (in words) of the stack that should be created for the task. */
        (void*)NULL, /* Paramter passed into the task. */
        tskIDLE_PRIORITY + 3, /* Task Priority. */
        Task_UpdateControlStatus_Stack_Array, /* Stack array. */
        &Task_UpdateControlStatus_Buffer  /* Buffer for static allocation. */
   );

   xTaskCreateStatic(
        Task_BroadcastMotorStatus, /* The function that implements the task. */
        "Broadcast Motor Status Task", /* Text name for the task. */
        configMINIMAL_STACK_SIZE, /* The size (in words) of the stack that should be created for the task. */
        (void*)NULL, /* Paramter passed into the task. */
        tskIDLE_PRIORITY + 1, /* Task Priority. */
        Task_BroadcastMotorStatus_Stack_Array, /* Stack array. */
        &Task_BroadcastMotorStatus_Buffer  /* Buffer for static allocation. */
   );

   //Make watchdogs here...

    // xTaskCreateStatic(
    //     Task_CANWatchdog,              
    //     "CAN Watchdog",
    //     configMINIMAL_STACK_SIZE,
    //     (void*)NULL,
    //     tskIDLE_PRIORITY + 3,      //Higher than FSM  
    //     Task_Watchdog_Stack_Array,
    //     &Task_Watchdog_Buffer
    // );

    watchdog_init();

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