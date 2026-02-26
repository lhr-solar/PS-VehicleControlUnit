#include "stm32xx_hal.h"
#include "Inc/tasks.h"
#include "Inc/watchdogs.h"

void Init_Task() {
     xTaskCreateStatic(
        Task_SendMotorController, /* The function that implements the task. */
        "Init Task", /* Text name for the task. */
        TASK_FSM_STACK_SIZE, /* The size (in words) of the stack that should be created for the task. */
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


    watchdog_start_all();
}

int main() {
    HAL_Init();
    SystemClock_Config();

    watchdog_init();
    WATCHDOG_INIT_ALL_FSM_SIGNALS();

    initStatusEventGroup();
    
    xTaskCreateStatic(
        Init_Task,
        "init task",
        TASK_FSM_STACK_SIZE,
        
    )

    // Start the scheduler
    vTaskStartScheduler();

    while(1) {
        
    }
    
    return 0;
}