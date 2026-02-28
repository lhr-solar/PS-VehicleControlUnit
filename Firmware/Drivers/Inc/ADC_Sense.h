#pragma once

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "stm32xx_hal.h"

// ADC scaling constants
#define ADC_Max 4095
#define V_Ref 3300  // mV

// Isolated amp gain: 2/5 = 0.4
#define Gain_Numerator   2
#define Gain_Denominator 5

// Divider ratio: Rbottom / (Rbottom + Rtop)
#define Divider_Numerator   2490
#define Divider_Denominator (2490 + 100000)

// ADC Channel 11 (Motor Voltage) and 12 (Battery Voltage) are GPIOB 12 and 2
#define ADC1_CHANNEL ADC_CHANNEL_11
#define ADC2_CHANNEL ADC_CHANNEL_12
#define ADC_QUEUE_LENGTH 4
#define ADC_QUEUE_ITEM_SIZE sizeof(uint16_t)
#define ADC_SAMPLING_TIME ADC_SAMPLETIME_2CYCLES_5

/**
 * @brief ADC voltage measurement results
 *
 * Contains scaled motor and battery voltages in millivolts.
 */
typedef struct {
    uint32_t Motor_Voltage;
    uint32_t Battery_Voltage;
} ADC_Sense_Result;

/**
 * @brief ADC sense function return status
 */
typedef enum {
    ADC_SENSE_OK,
    ADC_QUEUE_ERR,              // Queue creation failed
    ADC_1_INIT_ERR,             // ADC1 initialization failed
    ADC_2_INIT_ERR,             // ADC2 initialization failed
    ADC_1_READ_ERR,             // ADC1 read failed
    ADC_2_READ_ERR,             // ADC2 read failed
    ADC_SENSE_INIT_ERR,         // Initialization not called or failed
    READ_ADC_BAD_PARAM_ERR,     // Bad result parameter
    MOTOR_QUEUE_RECEIVE_ERR,    // ADC values not received from motor ADC queue
    BATTERY_QUEUE_RECEIVE_ERR   // ADC values not received from battery ADC queue
} ADC_Sense_Status;

/**
 * @brief Initialize ADC1 struct and calls the wrapper adc_init function to initialize ADC1 peripheral
 * @param None
 * @retval None
 */
ADC_Sense_Status ADC_1_Init(void);

/**
 * @brief Initialize ADC2 struct and calls the wrapper adc_init function to initialize ADC2 peripheral
 * @param None
 * @retval None
 */
ADC_Sense_Status ADC_2_Init(void);

/**
 * @brief   Initialize ADC peripherals and internal queues
 *
 * Creates ADC queues and initializes both ADC instances.
 *
 * @return  ADC_SENSE_OK if successful, ADC_SENSE_ERR otherwise
 */
ADC_Sense_Status ADC_Sense_Init(void);

/**
 * @brief   Read ADC values and compute scaled voltages
 *
 * Triggers ADC conversions, waits for samples, and converts raw ADC
 * counts into millivolt values using fixed-point math.
 *
 * @param   Timeout_MS  Maximum time to wait for ADC samples in milliseconds
 * @param   Result      Pointer to result structure for voltages
 * @return  ADC_SENSE_OK if both ADC channels updated successfully,
 *          ADC_SENSE_ERR otherwise
 */
ADC_Sense_Status Read_ADC(uint32_t Timeout_MS,  ADC_Sense_Result *Result);

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC1_Init(void);

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC2_Init(void);