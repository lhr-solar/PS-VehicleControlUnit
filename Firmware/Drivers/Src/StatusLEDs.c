// NOTE: LEDs are negative logic, however ON and OFF are defined such that they reflect the actual
// state of the LED

#include "StatusLEDs.h"

static const GpioPin_t DebugLEDs[NUM_LEDS] = {
    {PRECHARGE_COMPLETE_LED_PORT, PRECHARGE_COMPLETE_LED_PIN},
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
    {HB_LED_PORT, HB_LED_PIN}
};

void LED_toggle(Status_Mapping_t LED) {
    if (LED < 0 || LED >= NUM_LEDS) {
        return;
    }
    HAL_GPIO_TogglePin(DebugLEDs[LED].port, DebugLEDs[LED].pin);
}

void LED_set(Status_Mapping_t LED, LED_state_t state) {
    if (LED < 0 || LED >= NUM_LEDS) {
        return;
    }
    if (LED == HB) {
        // the heartbeat LED is the only positive logic LED
        HAL_GPIO_WritePin(DebugLEDs[LED].port, DebugLEDs[LED].pin, !state);
    } else {
        HAL_GPIO_WritePin(DebugLEDs[LED].port, DebugLEDs[LED].pin, state);
    }
}

void LED_clear() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        LED_set(i, LED_OFF);
    }
}

void LED_init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    for (uint8_t i = 0; i < NUM_LEDS; ++i) {
        GPIO_InitTypeDef led_config = {
            .Mode = GPIO_MODE_OUTPUT_PP, 
            .Pull = GPIO_NOPULL, 
            .Pin = DebugLEDs[i].pin
        };

        HAL_GPIO_Init(DebugLEDs[i].port, &led_config);
        LED_set(i, LED_OFF);
    }
}