#include "leds.h"

static void VCU_leds_init(void);
static void heartbeat_init(void);

void leds_init(void){
    VCU_leds_init();
    heartbeat_init();
}

static void VCU_leds_init(void){
    GPIO_InitTypeDef led_config = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = (LSOM_LED1_PIN)
    };
    
    led_config.Pin = LSOM_LED1_PIN;
    HAL_GPIO_Init(LSOM_LED1_PORT, &led_config);
}

static void heartbeat_init(void){
    GPIO_InitTypeDef led_config = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = (HEARTBEAT_PIN)
    };
    gpio_clock_enable((uint32_t)HEARTBEAT_PORT);
    HAL_GPIO_Init(HEARTBEAT_PORT, &led_config);
}

void set_heartbeat_led(State pin_state){
    HAL_GPIO_WritePin(HEARTBEAT_PORT, HEARTBEAT_PIN, (pin_state == ON) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


void VCU_led_on(void){
    // Turn off all LEDs first
    HAL_GPIO_WritePin(PSOM_LED1_PORT, PSOM_LED1_PIN, GPIO_PIN_RESET);

    #ifdef VCU_0
    //turn on onboard led
    #endif
}

void VCU_Seven_Seg(int number){
    //make sure its 1-2 digits
    //write driver for shift register to 7-seg
    //multplexing for two digits
    //since the shiftregister+multiplexing is time dependent, potentially move it to a task
    
}
