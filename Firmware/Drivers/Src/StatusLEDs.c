#include "StatusLEDs.h"    

static uint16_t LEDbitmap;
static uint8_t modFaultBitmap;

static GpioPin_t DebugLEDs[] = {{PRECHARGE_PRE_COMPLETE_PORT, PRECHARGE_PRE_COMPLETE_PIN}, 
                                {PRECHARGE_PRECHARGE_TO_PORT, PRECHARGE_PRECHARGE_TO_PIN},
                                {PRECHARGE_PRE_SENSE_TO_PORT, PRECHARGE_PRE_SENSE_TO_PIN},
                                {PRECHARGE_PRE_ENABLE_PORT, PRECHARGE_PRE_ENABLE_PIN},
                                {MOTOR_CONTACTOR_M_SENSE_TO_PORT, MOTOR_CONTACTOR_M_SENSE_TO_PIN},
                                {MOTOR_CONTACTOR_ENABLE_PORT, MOTOR_CONTACTOR_ENABLE_PIN},
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

static uint16_t num_LEDs = 16;

static GpioPin_t Motor_Contactor_Enable = {MOTOR_CONTACTOR_ENABLE_PORT, MOTOR_CONTACTOR_ENABLE_PIN};
static GpioPin_t Motor_Contactor_Sense = {MOTOR_CONTACTOR_SENSE_PORT, MOTOR_CONTACTOR_SENSE_PIN};
static GpioPin_t Precharge_Contactor_Enable = {PRECHARGE_PRE_ENABLE_PORT, PRECHARGE_PRE_ENABLE_PIN};
static GpioPin_t Precharge_Contactor_Sense = {PRECHARGE_PRE_SENSE_PORT, PRECHARGE_PRE_SENSE_PIN};

void Toggle_LED(GpioPin_t LED, LED_state_t state) {
    HAL_GPIO_WritePin(LED.port, LED.pin, state);
}

// sets input to specified bit, then pulses the clock
static void loadBit(bool bit) {
    // HAL_GPIO_WritePin(LED_SER_PORT, LED_SER_PIN, bit);
    // HAL_GPIO_WritePin(LED_SRCLK_PORT, LED_SRCLK_PIN, ON);
    // HAL_GPIO_WritePin(LED_SRCLK_PORT, LED_SRCLK_PIN, OFF);
    // HAL_GPIO_WritePin(LED_SER_PORT, LED_SER_PIN, ON);
    // HAL_GPIO_WritePin(LEDports[0], LEDpins[0]);
    HAL_GPIO_WritePin(DebugLEDs[0].port, DebugLEDs[0].pin, 1);
}

// pushes loaded values to output
static void pushLEDS() {
    // HAL_GPIO_WritePin(LED_RCLK_PORT, LED_RCLK_PIN, ON);
    // HAL_GPIO_WritePin(LED_RCLK_PORT, LED_RCLK_PIN, OFF);
}

// Shift-Reg values loaded back to front (bitmap first bit is heartbeat, last is AmpIn)
static void updateStatusLEDs() {

    // load bits non-modfault LEDS into shift regs (heartbeat is first in, WatchdogErr in last)
    for (uint8_t bit_num = 0; bit_num < FAULT_LED_NUM; bit_num++) {
        loadBit((bool)(LEDbitmap & (1 << bit_num))); 
    }

    // loads mod fault into shift regs, (MSB in first, LSB in last)
    for (int8_t modFault = 4; modFault >= 0; modFault++) {
        loadBit((bool)(modFaultBitmap & (1 << modFault)));
    }

    (LEDbitmap & (1 << DEBUG_LED)) ? loadBit(ON) : loadBit(OFF);

    pushLEDS();
}  

void LEDsModFaultBitmap_set(uint8_t bitmap) {

    // make sure bitmap is in range
    if (bitmap >= (1 << MOD_FAULT_BITS)) {
        Error_Handler();
    }
 
    modFaultBitmap = bitmap;

    updateStatusLEDs();
}

void LED_set(Fault_Mapping_t LED, bool state) {

    // make sure LED is in range
    if ((LED < 0) || ((LED > 9) && (LED != 15))) {
        Error_Handler();
    }

    LED = 1 << LED;

    // clears specified bit
    LEDbitmap &= ~LED;
    // sets bit if state = 1, otherwise stays cleared if state = 0
    LEDbitmap |= (state ? LED : 0); 

    updateStatusLEDs();
}   

void LEDs_clear() {
    LEDbitmap = 0;
    modFaultBitmap = 0;
    updateStatusLEDs();
}

void LEDs_init() {

    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    // HAL_GPIO_WritePin(LED_RCLK_PORT, LED_SRCLK_PIN|LED_RCLK_PIN|LED_SER_PIN, GPIO_PIN_RESET);

    // GPIO_InitTypeDef GPIO_InitStruct = {0};

    // GPIO_InitStruct.Pin = LED_SRCLK_PIN;
    // GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // GPIO_InitStruct.Pull = GPIO_NOPULL;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // HAL_GPIO_Init(LED_SRCLK_PORT, &GPIO_InitStruct);

    // GPIO_InitStruct.Pin = LED_RCLK_PIN;
    // GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // GPIO_InitStruct.Pull = GPIO_NOPULL;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // HAL_GPIO_Init(LED_RCLK_PORT, &GPIO_InitStruct);

    // GPIO_InitStruct.Pin = LED_SER_PIN;
    // GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    // GPIO_InitStruct.Pull = GPIO_NOPULL;
    // GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    // HAL_GPIO_Init(LED_SER_PORT, &GPIO_InitStruct);

    LEDs_clear(); 
}