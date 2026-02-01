#include "ADC.h"
#include "pinDefs.h"
#include "ADC_Sense.h"

int Motor_Voltage = 0;
int Battery_Voltage = 0;

static QueueHandle_t Motor_ADC_Queue;
static QueueHandle_t Battery_ADC_Queue;

void ADC_Sense_Init() // Initialize ADCs and queues
{   
    Motor_ADC_Queue   = xQueueCreate(16, sizeof(int));
    Battery_ADC_Queue = xQueueCreate(16, sizeof(int));

    configASSERT(Motor_ADC_Queue);
    configASSERT(Battery_ADC_Queue);

    // init is your ADC_InitTypeDef (from elsewhere)
    configASSERT(adc_init(&init, hadc1) == ADC_OK);
    configASSERT(adc_init(&init, hadc2) == ADC_OK);
}

void Read_ADC() // Read ADC values and calculate voltages
{
    int Motor_ADC = 0;
    int Battery_ADC = 0;

    adc_read(ADC1, 20000, hadc1, &Motor_ADC_Queue);
    adc_read(ADC2, 20000, hadc2, &Battery_ADC_Queue);

    if (xQueueReceive(Motor_ADC_Queue, &Motor_ADC, 0) == pdPASS)
    {
        Motor_Voltage = Motor_ADC * V_Ref * Gain_Denominator / (ADC_Max * Gain_Numerator * Divider_Denominator);
    }

    if (xQueueReceive(Battery_ADC_Queue, &Battery_ADC, 0) == pdPASS)
    {
        Battery_Voltage = Battery_ADC * V_Ref * Gain_Denominator / (ADC_Max * Gain_Numerator * Divider_Denominator);
    }
}