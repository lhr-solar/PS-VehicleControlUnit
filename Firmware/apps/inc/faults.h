#ifndef FAULTS_H
#define FAULTS_H

#include <stdbool.h>
#include "DriveMotor.h"
//faults struct, fault IDs, fault handling functions, context info, unconditionally throw fault or wait for function to check
typedef void (*callback_t)(void);

//For the recovery handler, should return true if recovery successful, false otherwise
typedef bool (*recovery_callback_t)(void);

void Faults_ThrowFault(fault_id_t faultBit);
void Faults_ThrowMultipleFaults(fault_id_t faultBits[], int numBits);
void Faults_ThrowFaultsUsingBitfield(EventBits_t bitfield);
void Faults_ClearFault(fault_id_t faultBit);
EventBits_t Faults_GetCurrentFaults(void);
void Task_FaultHandler(void *arg);

#define ALL_FAULT_BITS ((1 << NUM_FAULTS) - 1)

typedef struct faults_t{
    callback_t fault_handler;
    char* fault_context_message;
    bool recoverable;
    recovery_callback_t recover_handler;
    // int context_info;
} fault_t;

//have an event group for all the faults, also define the fault IDs and then wake said event group bit when fault thrown
// Enum for motor fault IDs
typedef enum {
    //these gotta be grouped, check getMotorStatus() for why...
    FAULT_ID_MOTOR_OVERSPEED,          // Motor over speed (15% overshoot above max RPM)
    FAULT_ID_IGBT_DESAT,               // Desaturation Fault (IGBT desaturation, IGBT driver OVLO)
    FAULT_ID_15V_RAIL_UVLO,            // 15V Rail under voltage lock out
    FAULT_ID_CONFIG_READ_ERROR,        // Config read error (some values may be reset to defaults)
    FAULT_ID_WATCHDOG_LAST_RESET,      // Watchdog caused last reset
    FAULT_ID_BAD_HALL_SEQUENCE,        // Bad motor position hall sequence
    FAULT_ID_DC_BUS_OV,                 // DC Bus over voltage
    FAULT_ID_SOFTWARE_OVER_CURRENT,    // Software over current
    FAULT_ID_HARDWARE_OVER_CURRENT,    // Hardware over current
    //ending motor flag group

    FAULT_ID_WATCHDOG_FSM,       
    FAULT_ID_GENERIC_CUZ_IM_LAZY,      


    NUM_FAULTS                         // Always last
} fault_id_t;


#endif /* FAULTS_H */
