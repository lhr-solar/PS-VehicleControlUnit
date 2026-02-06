#include <stdint.h>
#include "pinDefs.h"
#include "ADC_Sense.h"
#include "Precharge.h"

static int32_t Precharge_Threshold = PRECHARGE_GOOD_THRESHOLD;

typedef enum {
    PRECHARGE_STATE_IDLE = 0,
    PRECHARGE_STATE_RUNNING,
    PRECHARGE_STATE_DONE,
    PRECHARGE_STATE_FAULT,
} Precharge_State;

static Precharge_State State = PRECHARGE_STATE_IDLE;
static TickType_t Start_Tick = 0;
static Precharge_Status Fault_Reason = PRECHARGE_ERR_TIMEOUT;

Precharge_Status PrechargeStart() // Start precharge sequence and return status
{
    // Store system time once sense signal is given
    if (State == PRECHARGE_STATE_IDLE) 
    {
        Start_Tick = xTaskGetTickCount();
        State = PRECHARGE_STATE_RUNNING;

        // TODO: close precharge contactor here (and open main until threshold is reached)
    }

    if (State == PRECHARGE_STATE_DONE) 
    {
        return PRECHARGE_OK;
    }
    if (State == PRECHARGE_STATE_FAULT) 
    {
        // Fault is latched until PrechargeReset() is called.
        return Fault_Reason;
    }

    ADC_Sense_Result ADC_Result = {0};

    uint32_t updated = 0;
    if (Read_ADC(pdMS_TO_TICKS(20), &ADC_Result, &updated) != ADC_SENSE_OK) {
        // TODO: open contactors / safe state

        State = PRECHARGE_STATE_FAULT;
        Fault_Reason = PRECHARGE_ERR_ADC;
        return PRECHARGE_ERR_ADC;
    }

    int32_t Battery_Voltage = ADC_Result.Battery_Voltage;
    int32_t Motor_Voltage = ADC_Result.Motor_Voltage;

    if (Battery_Voltage > OVERVOLTAGE_THRESHOLD_MV)
    {
        /* BATTERY ABOUT TO GO BOOM */
        State = PRECHARGE_STATE_FAULT;
        Fault_Reason = PRECHARGE_ERR_OVERVOLTAGE;
        return PRECHARGE_ERR_OVERVOLTAGE;
    }

    if (Battery_Voltage < UNDERVOLTAGE_THRESHOLD_MV)
    {
        /* Battery voltage is too low or battery is disconnected, treat as fault */
        State = PRECHARGE_STATE_FAULT;
        Fault_Reason = PRECHARGE_ERR_UNDERVOLTAGE;
        return PRECHARGE_ERR_UNDERVOLTAGE;
    }
    
    if ((int64_t)Motor_Voltage * RATIO_SCALE >= (int64_t)Battery_Voltage * Precharge_Threshold) 
    {
        // Transition: once we reach 90%, we can lower threshold (hysteresis) and finish.
        Precharge_Threshold = PRECHARGE_TRANSITION_THRESHOLD;

        // TODO: close precharge contactor

        State = PRECHARGE_STATE_DONE;
        return PRECHARGE_OK;
    }

    const TickType_t Current_Tick = xTaskGetTickCount();
    if ((Current_Tick - Start_Tick) > pdMS_TO_TICKS(PRECHARGE_TIMEOUT_MS)) 
    {
        State = PRECHARGE_STATE_FAULT;
        Fault_Reason = PRECHARGE_ERR_TIMEOUT;
        return PRECHARGE_ERR_TIMEOUT;
    }

    return PRECHARGE_IN_PROGRESS;
}

// TODO: Precharge task during car operation

// TODO: Precharge shutdown sequence