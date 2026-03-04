// NOTE: LEDs are negative logic, however ON and OFF are defined such that they reflect the actual state of the LED

#include "StatusLEDs.h"

static uint16_t LEDbitmap;

static const GpioPin_t DebugLEDs[num_LEDs] = {{PRECHARGE_COMPLETE_LED_PORT, PRECHARGE_COMPLETE_LED_PIN},
                                              {PRECHARGE_TO_LED_PORT, PRECHARGE_TO_LED_PIN},
                                              {PRECHARGE_SENSE_TO_LED_PORT, PRECHARGE_SENSE_TO_LED_PIN},
                                              {MOTOR_SENSE_TO_LED_PORT, MOTOR_SENSE_TO_LED_PIN},
                                              {DRIVABLE_LED_PORT, DRIVABLE_LED_PIN},
                                              {DRIVING_LED_PORT, DRIVING_LED_PIN},
                                              {CRUISE_LED_PORT, CRUISE_LED_PIN},
                                              {REGEN_LED_PORT, REGEN_LED_PIN},
                                              {BPS_FAULT_LED_PORT, BPS_FAULT_LED_PIN},
                                              {CAR_HB_LED_PORT, CAR_HB_LED_PIN},
                                              {HALL_EFFECT_LED_PORT, HALL_EFFECT_LED_PIN},
                                              {DAWG_LED_PORT, DAWG_LED_PIN},
                                              {SWOC_LED_PORT, SWOC_LED_PIN},
                                              {FAULT_LED_PORT, FAULT_LED_PIN},
                                              {HB_LED_PORT, HB_LED_PIN}};

void Toggle_LED(Status_Mapping_t LED, LED_state_t state)
{
    HAL_GPIO_WritePin(DebugLEDs[LED].port, DebugLEDs[LED].pin, state);
}

int Get_LED_Bitmap()
{
    return LEDbitmap;
}

void LED_set(Status_Mapping_t LED, LED_state_t state)
{

    // make sure LED is in range
    if (LED < 0 || LED >= num_LEDs)
    {
        Error_Handler();
    }

    uint16_t mask = (1u << (uint16_t)LED);

    // clears specified bit
    LEDbitmap &= ~mask;
    // sets bit if state = 1, otherwise stays cleared if state = 0
    if (state)
    {
        LEDbitmap |= mask;
    }

    update_status();
}

void LEDs_clear()
{
    LEDbitmap = 0;
    update_status();
}

void update_status()
{
    for (int i = 0; i < num_LEDs; i++)
    {
        uint16_t mask = (1u << i);
        uint8_t led_on = (LEDbitmap & mask) ? 1u : 0u;
        HAL_GPIO_WritePin(DebugLEDs[i].port, DebugLEDs[i].pin, led_on ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}

void LEDs_init()
{

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    for (size_t i = 0; i < num_LEDs; ++i)
    {

        GPIO_InitTypeDef led_config = {
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Pin = DebugLEDs[i].pin};

        HAL_GPIO_WritePin(DebugLEDs[i].port, DebugLEDs[i].pin, OFF);
        HAL_GPIO_Init(DebugLEDs[i].port, &led_config);
        // HAL_GPIO_WritePin(DebugLEDs[i].port, DebugLEDs[i].pin, OFF);
    }
}