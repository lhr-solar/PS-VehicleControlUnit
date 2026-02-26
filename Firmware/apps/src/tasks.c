#include "tasks.h"

void Task_Init(void *arg){
    
    xTaskCreateStatic(
        Task_SendMotor, /* The function that implements the task. */
        "Send Motor Task", /* Text name for the task. */
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

}