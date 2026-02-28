#pragma once

#include <event_groups.h>

/**
 * @brief ADC task initialization function, initializes ADC and printf
 * @param None
 * @retval None
 */
void Init_ReadADCTask();

/**
 * @brief ADC task main execution function, continuously reads ADC values and prints them with a 1 second delay
 * @param None
 * @retval None
 */
void Task_ReadADC();