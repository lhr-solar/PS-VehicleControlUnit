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
    // printf("Watchdog timeout for CAN message %d\n", fsm_signal_to_can_id[signal]);
    Faults_ThrowFault(FAULT_ID_WATCHDOG_FSM);
}








