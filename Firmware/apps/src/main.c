/*================== INIT TASK ================*/
StaticTask_t Task_Init_Buffer;
StackType_t Task_Init_Stack_Array[configMINIMAL_STACK_SIZE];

//attach this to fsm... and figure out all the defines/sizes
//Make the main FSM task, make watch dogs, then have a suspended task actiavted via event for faults, and fix all the OS errors
int main() {
    HAL_Init();
    SystemClock_Config();

    initStatusEventGroup();
    
    xTaskCreateStatic(
        Task_Init,              
        "Init Task",
        configMINIMAL_STACK_SIZE,
        (void*)NULL,
        tskIDLE_PRIORITY + 3,      //Higher than FSM  
        Task_Init_Stack_Array,
        &Task_Init_Buffer
    );


    while(1){
        // Scheduler should've started by now
        // Code should never enter this point
    }
    
    return 0;
}