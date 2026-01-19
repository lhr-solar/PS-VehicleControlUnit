#include "FreeRTOS.h"
#include "event_groups.h"
#include "tasks.h"


StaticEventGroup_t xWdEventGroupBuffer;
EventGroupHandle_t xWdEventGroup;

void CAN_Watchdog_Init(void) {
    xWdEventGroup = xEventGroupCreateStatic(&xWdEventGroupBuffer);
    // At start of window: assume all messages are missing
    xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS);
}

//timer callback to restart window
StaticTimer_t xWdTimerBuffer;
TimerHandle_t xWdTimer;

static void Wd_WindowCallback(TimerHandle_t xTimer)
{
    xEventGroupSetBits(xWdEventGroup, WD_WINDOW_DONE);
}

//Timer init function
void Watchdog_TimerInit(void)
{
    xWdTimer = xTimerCreateStatic(
        "WDWin",
        pdMS_TO_TICKS(100), //window length
        pdFALSE,        //one-shot that bih
        NULL,       //no timer ID, may want to assign later if needed
        Wd_WindowCallback, //callback function
        &xWdTimerBuffer //static mem
    );

    configASSERT(xWdTimer);
}

//Real Task
void Task_CANWatchdog(void *arg){
    EventBits_t uxBits;

    xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS); //first clear

    // Start the first window
    xTimerStart(xWdTimer, 0);


    while(true){
        //reset bits
        xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS);

        uxBits = xEventGroupWaitBits(
                    xWdEventGroup,
                    WD_WINDOW_DONE,
                    pdTRUE,    // clear the bit automatically
                    pdFALSE,   // wait for any bit (only WD_WINDOW_DONE here)
                    portMAX_DELAY
                 );

        

        if((uxBits & ALL_CAN_MSGS) == ALL_CAN_MSGS){ //might not need to & ALL BITS DONE bc it auto clears
            //We received all the messages
            //xEventGroupClearBits(xWdEventGroup, ALL_CAN_MSGS); //resetting again
            xTimerStart(xWdTimer, 0); //start next window
        }else{
            //one or more messages were missing, wake up a fault task
            
        }
    }

}








