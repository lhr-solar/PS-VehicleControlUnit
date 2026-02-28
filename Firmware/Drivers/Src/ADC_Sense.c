#include "ADC.h"
#include "pinDefs.h"
#include "ADC_Sense.h"
#include "stm32xx_hal.h"
#include "inits.h"
#include "UART.h"
#include "printf.h"
#include "FaultBits.h"

static uint8_t Is_Initialized = 0;

ADC_InitTypeDef adc_init_1 = {0};
ADC_InitTypeDef adc_init_2 = {0};

QueueHandle_t Motor_ADC_Queue;
QueueHandle_t Battery_ADC_Queue;

static StaticQueue_t xStaticQueue1;
static StaticQueue_t xStaticQueue2;
uint8_t qStorage1[ADC_QUEUE_LENGTH * ADC_QUEUE_ITEM_SIZE];
uint8_t qStorage2[ADC_QUEUE_LENGTH * ADC_QUEUE_ITEM_SIZE];

static ADC_ChannelConfTypeDef sConfig1 = {
    .Channel = ADC1_CHANNEL,
    .Rank = ADC_REGULAR_RANK_1,
    .SamplingTime = ADC_SAMPLING_TIME,
    .SingleDiff = ADC_SINGLE_ENDED,
    .OffsetNumber = ADC_OFFSET_NONE,
    .Offset = 0
};

static ADC_ChannelConfTypeDef sConfig2 = {
    .Channel = ADC2_CHANNEL,
    .Rank = ADC_REGULAR_RANK_1,
    .SamplingTime = ADC_SAMPLING_TIME,
    .SingleDiff = ADC_SINGLE_ENDED,
    .OffsetNumber = ADC_OFFSET_NONE,
    .Offset = 0 
};

ADC_Sense_Status ADC_1_Init()
{
    adc_init_1.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    adc_init_1.Resolution = ADC_RESOLUTION_12B;
    adc_init_1.DataAlign = ADC_DATAALIGN_RIGHT;
    adc_init_1.ScanConvMode = ADC_SCAN_DISABLE;
    adc_init_1.EOCSelection = ADC_EOC_SINGLE_CONV;
    adc_init_1.LowPowerAutoWait = DISABLE;
    adc_init_1.ContinuousConvMode = DISABLE;
    adc_init_1.NbrOfConversion = 1;
    adc_init_1.DiscontinuousConvMode = DISABLE;
    adc_init_1.ExternalTrigConv = ADC_SOFTWARE_START;
    adc_init_1.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc_init_1.DMAContinuousRequests = DISABLE;
    adc_init_1.Overrun = ADC_OVR_DATA_PRESERVED;
    adc_init_1.OversamplingMode = DISABLE;

    if (adc_init(&adc_init_1, hadc1) != ADC_OK)
    {
        // ADC1 initialization failed
        set_faultBit(ADC_1_INIT_ERR);
        return ADC_1_INIT_ERR;
    }

      ADC_MultiModeTypeDef multimode = {0};
      multimode.Mode = ADC_MODE_INDEPENDENT;
      if (HAL_ADCEx_MultiModeConfigChannel(hadc1, &multimode) != HAL_OK)
      {
          Error_Handler();
      }

      if (HAL_ADC_ConfigChannel(hadc1, &sConfig1) != HAL_OK)
      {
          Error_Handler();
      }

      return ADC_SENSE_OK;
}

ADC_Sense_Status ADC_2_Init()
{
    adc_init_2.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    adc_init_2.Resolution = ADC_RESOLUTION_12B;
    adc_init_2.DataAlign = ADC_DATAALIGN_RIGHT;
    adc_init_2.ScanConvMode = ADC_SCAN_DISABLE;
    adc_init_2.EOCSelection = ADC_EOC_SINGLE_CONV;
    adc_init_2.LowPowerAutoWait = DISABLE;
    adc_init_2.ContinuousConvMode = DISABLE;
    adc_init_2.NbrOfConversion = 1;
    adc_init_2.DiscontinuousConvMode = DISABLE;
    adc_init_2.ExternalTrigConv = ADC_SOFTWARE_START;
    adc_init_2.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc_init_2.DMAContinuousRequests = DISABLE;
    adc_init_2.Overrun = ADC_OVR_DATA_PRESERVED;
    adc_init_2.OversamplingMode = DISABLE;

    if (adc_init(&adc_init_2, hadc2) != ADC_OK)
    {
        // ADC2 initialization failed
        set_faultBit(ADC_2_INIT_ERR);
        return ADC_2_INIT_ERR;
    }

    if (HAL_ADC_ConfigChannel(hadc2, &sConfig2) != HAL_OK)
    {
        Error_Handler();
    }

    return ADC_SENSE_OK;
}

ADC_Sense_Status ADC_Sense_Init(void) // Initialize ADCs and queues
{
    Motor_ADC_Queue = xQueueCreateStatic(ADC_QUEUE_LENGTH, ADC_QUEUE_ITEM_SIZE, qStorage1, &xStaticQueue1);
    Battery_ADC_Queue = xQueueCreateStatic(ADC_QUEUE_LENGTH, ADC_QUEUE_ITEM_SIZE, qStorage2, &xStaticQueue2);

    Is_Initialized = 0;

    if (Motor_ADC_Queue == NULL || Battery_ADC_Queue == NULL)
    {
        // Queue creation failed
        set_faultBit(ADC_QUEUE_ERR);
        return ADC_QUEUE_ERR;
    }
    
    if (ADC_1_Init() != ADC_SENSE_OK || ADC_2_Init() != ADC_SENSE_OK)
    {
        // One or both ADC initializations failed
        set_faultBit(ADC_SENSE_INIT_ERR);
        return ADC_SENSE_INIT_ERR;
    }

    Is_Initialized = 1;
    return ADC_SENSE_OK;
}

ADC_Sense_Status Read_ADC(uint32_t Timeout_MS, ADC_Sense_Result *Result) // Read ADC values and calculate voltages
{
    if (!Is_Initialized)
    {
        // ADC_Sense_Init has not been called or failed
        set_faultBit(ADC_SENSE_INIT_ERR);
        return ADC_SENSE_INIT_ERR;
    }

    if (Result == NULL)
    {
        // Invalid result pointer
        set_faultBit(READ_ADC_BAD_PARAM_ERR);
        return READ_ADC_BAD_PARAM_ERR;
    }

    uint16_t Motor_ADC = 0;
    uint16_t Battery_ADC = 0;
    TickType_t Timeout_Ticks = pdMS_TO_TICKS(Timeout_MS);

    HAL_ADCEx_Calibration_Start(hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(hadc2, ADC_SINGLE_ENDED);

    if (adc_read(hadc1, &sConfig1, Motor_ADC_Queue) != ADC_OK)
    {
        // Motor ADC read failed
        set_faultBit(ADC_1_READ_ERR);
        return ADC_1_READ_ERR;
    }
    if (adc_read(hadc2, &sConfig2, Battery_ADC_Queue) != ADC_OK)
    {
        // Battery ADC read failed
        set_faultBit(ADC_2_READ_ERR);
        return ADC_2_READ_ERR;
    }

    if (xQueueReceive(Motor_ADC_Queue, &Motor_ADC, Timeout_Ticks) == pdPASS)
    {
        uint64_t Numerator = (uint64_t)Motor_ADC * V_Ref * Gain_Denominator * Divider_Denominator; // Convert ADC reading to voltage in mV with scaling factors
        uint64_t Denominator = (uint64_t)ADC_Max * Gain_Numerator * Divider_Numerator;
        Result->Motor_Voltage = (uint32_t)(Numerator / Denominator);
    }
    else
    {
        // Queue receive failed for motor ADC
        set_faultBit(MOTOR_QUEUE_RECEIVE_ERR);
        return MOTOR_QUEUE_RECEIVE_ERR;
    }

    if (xQueueReceive(Battery_ADC_Queue, &Battery_ADC, Timeout_Ticks) == pdPASS)
    {
        uint64_t Numerator = (uint64_t)Battery_ADC * V_Ref * Gain_Denominator * Divider_Denominator; // Convert ADC reading to voltage in mV with scaling factors
        uint64_t Denominator = (uint64_t)ADC_Max * Gain_Numerator * Divider_Numerator;
        Result->Battery_Voltage = (uint32_t)(Numerator / Denominator);
    }
    else
    {
        // Queue receive failed for battery ADC
        set_faultBit(BATTERY_QUEUE_RECEIVE_ERR);
        return BATTERY_QUEUE_RECEIVE_ERR;
    }

    return ADC_SENSE_OK;
}

