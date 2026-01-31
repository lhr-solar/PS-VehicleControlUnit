#include <stdint.h>
#include "pinDefs.h"

#define Overvoltage_Threshold 140000 // mV
#define Ratio_Scale 1000
#define Threshold_1 900
#define Threshold_2 800
#define Gain_Divider_Constant (33000 / (4095 * 4 * (2490 / (2490 + 100000))))
float Precharge_Threshold = Threshold_1;

void Precharge()
{
    /* Close main contactor */

    /* Wait for precharge delay */

    Read_ADC();

    if (Battery_Voltage > Overvoltage_Threshold)
    {
        /* BATTERY ABOUT TO GO BOOM */
    }
        else if (Battery_Voltage < 1000) // A sensible low-voltage threshold
    {
        /* Battery voltage is too low or battery is disconnected, treat as fault */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
    }
    else if (Motor_Voltage / Battery_Voltage < Precharge_Threshold)
    {
        /* Precharge timeout fault */
        // Set GPIO Pin 12 High
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET);
    }
    else if (Motor_Voltage / Battery_Voltage >= Precharge_Threshold)
    {
        Precharge_Threshold = Threshold_2;
        /* Close precharge contactor */
        // Set GPIO Pin 8 High
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
    }
}