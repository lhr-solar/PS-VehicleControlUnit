#include "StatusLEDs.h"    
#include <stdint.h>
#include "pinDefs.h"

static uint16_t LEDbitmap;

static const GpioPin_t DebugLEDs[num_LEDs]= {{PRECHARGE_PRE_COMPLETE_PORT, PRECHARGE_PRE_COMPLETE_PIN}, 
                                {PRECHARGE_PRECHARGE_TO_PORT, PRECHARGE_PRECHARGE_TO_PIN},
                                {PRECHARGE_PRE_SENSE_TO_PORT, PRECHARGE_PRE_SENSE_TO_PIN},
                                {MOTOR_CONTACTOR_M_SENSE_TO_PORT, MOTOR_CONTACTOR_M_SENSE_TO_PIN},
                                {CAR_STATE_DRIVABLE_PORT, CAR_STATE_DRIVABLE_PIN},
                                {CAR_STATE_DRIVING_PORT, CAR_STATE_DRIVING_PIN},
                                {CAR_STATE_CRUISE_PORT, CAR_STATE_CRUISE_PIN},
                                {CAR_STATE_REGEN_PORT, CAR_STATE_REGEN_PIN},
                                {CAR_STATE_BPSFAULT_PORT, CAR_STATE_BPSFAULT_PIN},
                                {CAR_STATE_HB_PORT, CAR_STATE_HB_PIN},
                                {MOTOR_FAULT_HALL_PORT, MOTOR_FAULT_HALL_PIN},
                                {MOTOR_FAULT_DAWG_PORT, MOTOR_FAULT_DAWG_PIN},
                                {MOTOR_FAULT_SWOC_PORT, MOTOR_FAULT_SWOC_PIN},
                                {MOTOR_FAULT_FAULT_PORT, MOTOR_FAULT_FAULT_PIN}
};

void Toggle_LED(Fault_Mapping_t LED, LED_state_t state) {
    HAL_GPIO_WritePin(DebugLEDs[LED].port, DebugLEDs[LED].pin, state);
}

void LED_set(Fault_Mapping_t LED, LED_state_t state) {

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