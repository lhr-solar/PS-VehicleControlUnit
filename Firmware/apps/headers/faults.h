#include <stdbool.h>
//faults struct, fault IDs, fault handling functions, context info, unconditionally throw fault or wait for function to check
typedef void (*callback_t)(void);

//For the recovery handler, should return true if recovery successful, false otherwise
typedef bool (*recovery_callback_t)(void);

#define ALL_FAULT_BITS ((1 << NUM_FAULTS) - 1)

typedef struct faults_t{
    callback_t fault_handler;
    char* fault_context_message;
    bool recoverable;
    recovery_callback_t recover_handler;
    // int context_info;
} fault_t;

//have an event group for all the faults, also define the fault IDs and then wake said event group bit when fault thrown
typedef enum {
    FAULT_ID_WATCHDOG_FSM,
    NUM_FAULTS
    //add more motor faults
} fault_id_t;


//have some type of array that throws the associated fault when indexes with enums, assert lengths  

//I don't have to make the enum index explicit but its safer...
static fault_t faultArray[NUM_FAULTS] = {
    [FAULT_ID_WATCHDOG_FSM] = {&handleWatchdogFSMFault, "FSM Watchdog Fault: Missing CAN messages in watchdog window.", false, NULL},
    //add more faults here
};

//making an empty array for storing all the recover handlers dynamically in faults task
static recovery_callback_t recoverHandlers[NUM_FAULTS] = {NULL};


    