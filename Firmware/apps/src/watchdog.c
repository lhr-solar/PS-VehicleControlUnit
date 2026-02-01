#include "FreeRTOS.h"
#include "event_groups.h"
#include "tasks.h"
#include <DriveMotor.h>
#include "faults.h"
#include <assert.h>
#include "timers.h"   // for FreeRTOS software timers

#define MAX_CAN_WD_TIMERS 20


static StaticTimer_t xCANWdTimerBuffers[MAX_CAN_WD_TIMERS];
static TimerHandle_t xCANWdTimers[MAX_CAN_WD_TIMERS];
static int wd_timer_count = 0;

void generic_can_wd_callback(TimerHandle_t xTimer);

void watchdog_init(void) {
    xWdEventGroup = xEventGroupCreateStatic(&xWdEventGroupBuffer);
    // At start of window: assume all messages are missing
    xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS);
}

void recieved_CAN_message(FSM_Signal_t signal) {
    //Set the bit corresponding to the received message
    xEventGroupSetBits(xWdEventGroup, (1 << signal));
}

void CAN_MSG_Watchdog_Create(const char* timerName,
                            FSM_Signal_t signal,
                            uint32_t timeout_ms)
{
    assert(wd_timer_count < MAX_CAN_WD_TIMERS);

    xCANWdTimers[wd_timer_count] = xTimerCreateStatic(
        timerName,
        pdMS_TO_TICKS(timeout_ms),
        pdTRUE,  // auto-reload
        (void *)(uintptr_t)signal, //using signal as timer ID for later use
        generic_can_wd_callback,   
        &xCANWdTimerBuffers[wd_timer_count]
    );

    assert(xCANWdTimers[wd_timer_count] != NULL);

    wd_timer_count++;
}

void generic_can_wd_callback(TimerHandle_t xTimer){

    FSM_Signal_t signal = (FSM_Signal_t)(uintptr_t)xTimerGetTimerID(xTimer);

    if(xEventGroupGetBits(xWdEventGroup) & (1 << signal)){
        //Message was received in time, clear bit and return
        xEventGroupClearBits(xWdEventGroup, (1 << signal));
        return;
    }

    //Handle the timeout event for the specific CAN message
    //e.g., log error, set fault flag, etc.
    printf("Watchdog timeout for CAN message %d\n", fsm_signal_to_can_id[signal]);
    Faults_ThrowFault(FAULT_ID_WATCHDOG_FSM);
}


//Gotta fade this implementation :(
// StaticEventGroup_t xWdEventGroupBuffer;
// EventGroupHandle_t xWdEventGroup;

// void CAN_Watchdog_Init(void) {
//     xWdEventGroup = xEventGroupCreateStatic(&xWdEventGroupBuffer);
//     // At start of window: assume all messages are missing
//     xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS);
// }

// //timer callback to restart window
// StaticTimer_t xWdTimerBuffer;
// TimerHandle_t xWdTimer;

// static void Wd_WindowCallback(TimerHandle_t xTimer)
// {
//     xEventGroupSetBits(xWdEventGroup, WD_WINDOW_DONE);
// }

// //Timer init function
// void Watchdog_TimerInit(void)
// {
    // xWdTimer = xTimerCreateStatic(
    //     "WDWin",
    //     pdMS_TO_TICKS(100), //window length
    //     pdFALSE,        //one-shot that bih
    //     NULL,       //no timer ID, may want to assign later if needed
    //     Wd_WindowCallback, //callback function
    //     &xWdTimerBuffer //static mem
    // );

//     configASSERT(xWdTimer);
// }

// //Real Task
// void Task_CANWatchdog(void *arg){
//     EventBits_t uxBits;

//     xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS); //first clear

//     // Start the first window
//     xTimerStart(xWdTimer, 0);


//     while(true){
//         //reset bits
//         xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS);

//         uxBits = xEventGroupWaitBits(
//                     xWdEventGroup,
//                     WD_WINDOW_DONE,
//                     pdTRUE,    // clear the bit automatically
//                     pdFALSE,   // wait for any bit (only WD_WINDOW_DONE here)
//                     portMAX_DELAY
//                  );

        

//         if((uxBits & ALL_CAN_MSGS) == ALL_CAN_MSGS){ //might not need to & ALL BITS DONE bc it auto clears
//             //We received all the messages
//             //xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS); //resetting again
//             xTimerStart(xWdTimer, 0); //start next window
//         }else{
//             //one or more messages were missing, wake up a fault task
            
//         }
//     }

// }








