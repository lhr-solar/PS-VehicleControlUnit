#include "stm32xx_hal.h"
#include "pinDefs.h"

// Initialize clock for heartbeat LED port
void Heartbeat_Clock_Init() {
    switch ((uint32_t)HB_LED_PORT) {
        case (uint32_t)GPIOA:
            __HAL_RCC_GPIOA_CLK_ENABLE();
            break;
        case (uint32_t)GPIOB:
            __HAL_RCC_GPIOB_CLK_ENABLE();
            break;
        case (uint32_t)GPIOC:
            __HAL_RCC_GPIOC_CLK_ENABLE();
            break;
    }
}

int main(){
    HAL_Init();

    // Heartbeat LED on VCU is PB14
    GPIO_InitTypeDef led_config = {
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Pin = HB_LED_PIN
    };
    
    Heartbeat_Clock_Init(); // enable clock for HB_LED_PORT
    HAL_GPIO_Init(HB_LED_PORT, &led_config); // initialize HB_LED_PORT with led_config

    while(1){
        HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
        HAL_Delay(500);
    }

    return 0;
}