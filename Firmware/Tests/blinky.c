#include "leds.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();
    leds_init();
    volttemp_led_on();


    while (1) {
        set_heartbeat_led(ON);
        HAL_Delay(500);
        set_heartbeat_led(OFF);
        HAL_Delay(500);
    }
}
