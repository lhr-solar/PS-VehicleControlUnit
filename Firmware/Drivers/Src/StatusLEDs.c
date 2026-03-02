// NOTE: LEDs are negative logic, however ON and OFF are defined such that they reflect the actual state of the LED

#include "StatusLEDs.h"    
#include <stdint.h>
#include "pinDefs.h"

static uint16_t LEDbitmap;

static const GpioPin_t DebugLEDs[num_LEDs]= {{PRECHARGE_COMPLETE_LED_PORT, PRECHARGE_COMPLETE_LED_PIN}, 
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
                                {FAULT_LED_PORT, FAULT_LED_PIN}
};

void Toggle_LED(Status_Mapping_t LED, LED_state_t state) {
    HAL_GPIO_WritePin(DebugLEDs[LED].port, DebugLEDs[LED].pin, state);
}

void LED_set(Status_Mapping_t LED, LED_state_t state) {

    // make sure LED is in range
    if (LED<0 || LED>=num_LEDs) {
        Error_Handler();
    }

    // clears specified bit
    LEDbitmap &= ~(1<<LED);
    // sets bit if state = 1, otherwise stays cleared if state = 0
    LEDbitmap |= (1<<LED);
    update_status();
}   

void LEDs_clear() {
    LEDbitmap = 0;
    update_status();
}

void update_status(){
    for(int i =0; i< num_LEDs; i++){
        HAL_GPIO_WritePin(DebugLEDs[i].port, DebugLEDs[i].pin, LEDbitmap&(1<<i));
    }
}

void LEDs_init() {

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    for (size_t i = 0; i < num_LEDs; ++i) {

        GPIO_InitTypeDef led_config = {
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Pin = DebugLEDs[i].pin
        };

        HAL_GPIO_Init(DebugLEDs[i].port, &led_config);
        HAL_GPIO_WritePin(DebugLEDs[i].port, DebugLEDs[i].pin, GPIO_PIN_SET);
    }
}