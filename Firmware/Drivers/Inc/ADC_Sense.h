#ifndef ADC_SENSE_H
#define ADC_SENSE_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

// ADC scaling constants
#define ADC_Max 4095
#define V_Ref 3300  // mV

// Isolated amp gain: 2/5 = 0.4
#define Gain_Numerator   2
#define Gain_Denominator 5

// Divider ratio: Rbottom / (Rbottom + Rtop)
#define Divider_Numerator   2490
#define Divider_Denominator (2490 + 100000)

// Public globals
extern int Motor_Voltage;
extern int Battery_Voltage;

void ADC_Sense_Init(void);
void Read_ADC(void);

#endif