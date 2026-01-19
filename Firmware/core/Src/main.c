#include "FreeRTOS.h"
#include "stm32xx_hal.h"

int main(){
    HAL_Init();
    SystemClock_Config();

    while(1){

    }
    return 0;
}