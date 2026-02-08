#include "ADC.h"
#include "pinDefs.h"
#include "ADC_Sense.h"

QueueHandle_t Motor_ADC_Queue;
QueueHandle_t Battery_ADC_Queue;

static uint8_t Is_Initialized = 0;
static uint32_t Error_Mask = ADC_SENSE_ERR_NONE;

uint8_t qStorage[ADC_QUEUE_LENGTH * sizeof(uint16_t)];
static StaticQueue_t xStaticQueue;
uint8_t qStorage2[ADC_QUEUE_LENGTH * sizeof(uint16_t)];
static StaticQueue_t xStaticQueue2;

ADC_InitTypeDef adc_init_1 = {0};
ADC_InitTypeDef adc_init_2 = {0};

ADC_Sense_Status ADC_Sense_Init(void) // Initialize ADCs and queues
{   
    Motor_ADC_Queue   = xQueueCreateStatic(ADC_QUEUE_LENGTH, sizeof(uint16_t), qStorage, &xStaticQueue);
    Battery_ADC_Queue = xQueueCreateStatic(ADC_QUEUE_LENGTH, sizeof(uint16_t), qStorage2, &xStaticQueue2);

    Is_Initialized = 0;

    if (Motor_ADC_Queue == NULL || Battery_ADC_Queue == NULL) 
    {
        Error_Mask |= ADC_SENSE_ERR_NOT_INIT;
        return ADC_SENSE_ERR;
    }

    if (adc_init(&adc_init_1, hadc1) != ADC_OK) 
    {
        Error_Mask |= ADC_SENSE_ERR_ADC1_INIT;
        return ADC_SENSE_ERR;
    }

    if (adc_init(&adc_init_2, hadc2) != ADC_OK) 
    {
        Error_Mask |= ADC_SENSE_ERR_ADC2_INIT;
        return ADC_SENSE_ERR;
    }

    Is_Initialized = 1;
    return ADC_SENSE_OK;
}

uint32_t ADC_Sense_GetErrorMask(void)
{
    return Error_Mask;
}

void ADC_Sense_ClearErrors(uint32_t Mask)
{
    Error_Mask &= ~Mask;
}

ADC_Sense_Status Read_ADC(uint32_t Timeout_MS,  ADC_Sense_Result *Result, uint32_t *Updated_Mask) // Read ADC values and calculate voltages
{
    if (!Is_Initialized) 
    {
        if (Updated_Mask) *Updated_Mask = ADC_SENSE_UPD_NONE;
        Error_Mask |= ADC_SENSE_ERR_NOT_INIT;
        return ADC_SENSE_ERR;
    }

    if (Result == NULL) 
    {
        Error_Mask |= ADC_SENSE_ERR_BAD_PARAM;
        return ADC_SENSE_ERR;
    }

    uint16_t Motor_ADC = 0;
    uint16_t Battery_ADC = 0;
    uint32_t Updated = ADC_SENSE_UPD_NONE;
    TickType_t Timeout_Ticks = pdMS_TO_TICKS(Timeout_MS);

    adc_read(MOTOR_ADC, ADC_SAMPLING_TIME, hadc1, Motor_ADC_Queue);
    adc_read(BATTERY_ADC, ADC_SAMPLING_TIME, hadc2, Battery_ADC_Queue);

    if (xQueueReceive(Motor_ADC_Queue, &Motor_ADC, Timeout_Ticks) == pdPASS)
    {
        int64_t Numerator = (int64_t)Motor_ADC * V_Ref * Gain_Denominator * Divider_Denominator; // Convert ADC reading to voltage in mV with scaling factors
        int64_t Denominator = (int64_t)ADC_Max * Gain_Numerator * Divider_Numerator;
        Result->Motor_Voltage = (int32_t)(Numerator / Denominator);
        Updated |= ADC_SENSE_UPD_MOTOR;
    }
    else 
    {
        // Motor ADC stopped
        Error_Mask |= ADC_SENSE_ERR_MOTOR_STALE;
    }

    if (xQueueReceive(Battery_ADC_Queue, &Battery_ADC, Timeout_Ticks) == pdPASS)
    {
        int64_t Numerator = (int64_t)Battery_ADC * V_Ref * Gain_Denominator * Divider_Denominator; // Convert ADC reading to voltage in mV with scaling factors
        int64_t Denominator = (int64_t)ADC_Max * Gain_Numerator * Divider_Numerator;
        Result->Battery_Voltage = (int32_t)(Numerator / Denominator);
        Updated |= ADC_SENSE_UPD_BATTERY;
    }
    else 
    {
        // Battery ADC stopped
        Error_Mask |= ADC_SENSE_ERR_BATT_STALE;
    }

    if (Updated_Mask) 
    {
        *Updated_Mask = Updated;
    }

    if ((Updated & (ADC_SENSE_UPD_MOTOR | ADC_SENSE_UPD_BATTERY)) != (ADC_SENSE_UPD_MOTOR | ADC_SENSE_UPD_BATTERY)) // If either ADC failed to update, return error
    {
        return ADC_SENSE_ERR;
    }

    return ADC_SENSE_OK;
}