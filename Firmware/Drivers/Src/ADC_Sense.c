int Motor_Voltage = 0;
int Battery_Voltage = 0;

#define ADC_Max 4095
#define V_Ref 3300 // mV
#define Gain_Numerator 2
#define Gain_Denominator 5
#define Divider_Numerator 2490
#define Divider_Denominator (2490 + 100000)

void Read_ADC()
{
    uint32_t Motor_ADC = 0;
    uint32_t Battery_ADC = 0;

    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        Motor_ADC = HAL_ADC_GetValue(&hadc1);
    }

    HAL_ADC_Stop(&hadc1);

    HAL_ADC_Start(&hadc2);

    if (HAL_ADC_PollForConversion(&hadc2, 100) == HAL_OK)
    {
        Battery_ADC = HAL_ADC_GetValue(&hadc2);
    }

    HAL_ADC_Stop(&hadc2);

    Motor_Voltage = Motor_ADC * V_Ref * Gain_Denominator / (ADC_Max * Gain_Numerator * Divider_Denominator);
    Battery_Voltage = Battery_ADC * V_Ref * Gain_Denominator / (ADC_Max * Gain_Numerator * Divider_Denominator);
}