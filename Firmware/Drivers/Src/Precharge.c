#include <stdint.h>
#include "pinDefs.h"
#include "ADC_Sense.h"
#include "Precharge.h"

int Precharge_Threshold = THRESHOLD_1;

void PrechargeStart() // 
{
    /* Close main contactor an start contactor timeout time*/
    // Store system time once sense signal is given

    Read_ADC();

    if (Battery_Voltage > OVERVOLTAGE_THRESHOLD_MV)
    {
        /* BATTERY ABOUT TO GO BOOM */
    }
    else if (Battery_Voltage < UNDERVOLTAGE_THRESHOLD_MV)
    {
        /* Battery voltage is too low or battery is disconnected, treat as fault */
    }
    else if (Motor_Voltage * RATIO_SCALE < Battery_Voltage * Precharge_Threshold) // 
    {
        /* Precharge timeout fault */
        // Set GPIO Pin 12 High   
    }
    else if (Motor_Voltage * RATIO_SCALE >= Battery_Voltage* Precharge_Threshold)
    {
        Precharge_Threshold = THRESHOLD_2;
        /* Close precharge contactor */
        // Set GPIO Pin 8 High
        // Check time differece from precharge start to now
    }
}