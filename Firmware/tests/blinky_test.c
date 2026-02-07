#include "stm32xx_hal.h"
#include "pinDefs.h"

typedef struct {
    GPIO_TypeDef *Port;
    uint16_t Pin;
} Led_t;

// LED list (uses the LED-related defines from pinDefs.h)
static const Led_t leds[] = {
    { PRECHARGE_STATUS_LED_PORT, PRECHARGE_TIMEOUT_LED },
    { PRECHARGE_STATUS_LED_PORT, PRECHARGE_COMPLETELED },
    { CAR_STATE_DRIVABLE_PORT,   CAR_STATE_DRIVABLE_PIN },
    { CAR_STATE_DRIVING_PORT,    CAR_STATE_DRIVING_PIN },
    { CAR_STATE_CRUISE_PORT,     CAR_STATE_CRUISE_PIN },
    { CAR_STATE_REGEN_PORT,      CAR_STATE_REGEN_PIN },
    { CAR_STATE_BPSFAULT_PORT,   CAR_STATE_BPSFAULT_PIN },
    { CAR_STATE_HB_PORT,         CAR_STATE_HB_PIN }
};

static const size_t led_count = sizeof(leds) / sizeof(leds[0]);

// Enable GPIO clock for a given port
static void Enable_Port_Clock(GPIO_TypeDef *port)
{
    switch ((uint32_t)port) {
        case (uint32_t)GPIOA: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case (uint32_t)GPIOB: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case (uint32_t)GPIOC: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case (uint32_t)GPIOD: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        default: break;
    }
}

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

    /* 
    Heartbeat LED on VCU is PB14
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
    */

    // Initialize all LED GPIOs
    for (size_t i = 0; i < led_count; ++i) {
        Enable_Port_Clock(leds[i].Port);

        GPIO_InitTypeDef led_config = {
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Pin = leds[i].Pin
        };

        HAL_GPIO_Init(leds[i].Port, &led_config);
        HAL_GPIO_WritePin(leds[i].Port, leds[i].Pin, GPIO_PIN_RESET);
    }

    while (1) {
        // Turn LEDs on one-by-one
        for (size_t i = 0; i < led_count; ++i) {
            HAL_GPIO_WritePin(leds[i].Port, leds[i].Pin, GPIO_PIN_SET);
            HAL_Delay(200);
        }

        // Short pause with all LEDs on
        HAL_Delay(500);

        // Turn LEDs off one-by-one
        for (size_t i = 0; i < led_count; ++i) {
            HAL_GPIO_WritePin(leds[i].Port, leds[i].Pin, GPIO_PIN_RESET);
            HAL_Delay(100);
        }

        HAL_Delay(500);
    }

    return 0;
}