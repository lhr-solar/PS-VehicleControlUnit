#include "FreeRTOS.h"
#include "event_groups.h"
#include "faults.h"

StaticEventGroup_t xFaultEventGroupBuffer;
EventGroupHandle_t xFaultEventGroup;

void Faults_Init(void) {
    xFaultEventGroup = xEventGroupCreateStatic(&xFaultEventGroupBuffer);
    xEventGroupClearBits(xFaultEventGroup, ALL_FAULT_BITS);
}

void Faults_ThrowFault(fault_id_t faultBit) {
    xEventGroupSetBits(xFaultEventGroup, (1 << faultBit));
}

void Faults_ThrowMultipleFaults(fault_id_t faultBits[], int numBits) {
    EventBits_t faultGroup = 0;
    for(int i = 0; i < numBits; i++) {
        faultGroup |= (1 << faultBits[i]);
    }   
    xEventGroupSetBits(xFaultEventGroup, faultGroup);
}

void Faults_ThrowFaultsUsingBitfield(EventBits_t bitfield) {
    xEventGroupSetBits(xFaultEventGroup, bitfield);
}


//I don't know if we ever need this since faults are persistent
void Faults_ClearFault(fault_id_t faultBit) {
    xEventGroupClearBits(xFaultEventGroup, (1 << faultBit));
}

EventBits_t Faults_GetCurrentFaults(void) {
    return xEventGroupGetBits(xFaultEventGroup);
}

//Have a task for faults that blocks indefinitely until woken up by even group (fault is thrown)

void Task_FaultHandler(void *arg) {
    EventBits_t uxBits;
    bool canRecover = true;

    while(true) {   
        uxBits = xEventGroupWaitBits(
                    xFaultEventGroup,
                    ALL_FAULT_BITS, //wait for any fault bit
                    pdFALSE,    // do not clear bits automatically
                    pdFALSE,   // wait for any bit
                    portMAX_DELAY
                 );

        // Handle each fault that is set
        for(fault_id_t i = 0; i < NUM_FAULTS; i++) {
            if(uxBits & (1 << i)) {
                // Call the associated fault handler
                // Assuming we have an array of fault_t structs called faultArray
                canRecover &= faultArray[i].recoverable;
                //Go to disabled for FSM -> print context -> see if i can throw some os error based on bool
                printf("Handling Fault bit %d: %s\n", i, faultArray[i].fault_context_message);
                if(faultArray[i].fault_handler != NULL) {
                    faultArray[i].fault_handler();
                }

                //Store recover handler if exists
                if(faultArray[i].recover_handler != NULL) {
                    recoverHandlers[i] = faultArray[i].recover_handler;
                }
            }
        }

        //disable the FSM for any fault
        disableFSM();

        if(canRecover) { //mainly for SWOC
            // Attempt recovery actions if possible
            printf("All faults are recoverable. Attempting recovery actions.\n");
            bool recoverySuccessful = true;
            for(int i = 0; i < NUM_FAULTS; i++) {
                if(recoverHandlers[i] != NULL) {
                    printf("Attempting recovery for Fault ID %d\n", i);
                    if(recoverHandlers[i]()){ //The recover handler must return a bool indicating whether recovery was successful
                        printf("Recovery successful for Fault ID %d\n", i);
                    }else{
                        printf("Recovery failed for Fault ID %d\n", i);
                        recoverySuccessful = false;
                        break;
                    }
                }
            }

            if(recoverySuccessful){
                printf("All recoveries successful. Clearing faults and resuming operation.\n");
                xEventGroupClearBits(xFaultEventGroup, ALL_FAULT_BITS);
                recoverFSM(); //bringing it back to not ready
            }
        }else{
            // Lock the scheduler or take other necessary actions
            printf("One or more faults are non-recoverable. System requires reset.\n");
            // Implement system reset or halt
            vTaskSuspendAll(); // no new scheduling
            while(1); // halt

        }
            
    }
}





