#pragma once

#include "inits.h"
#include <stdint.h>

/** @brief LED States */
typedef enum {
    LED_OFF = GPIO_PIN_SET, // Negative logic
    LED_ON = GPIO_PIN_RESET
} LED_state_t;

/** 
 * @brief Logic-to-Hardware mapping for diagnostic LEDs.
 * @note Values correspond to specific shift register positions.
 */
typedef enum {
    PRECHARGE_COMPLETE,      // precharge voltage threshold reached
    PRECHARGE_TIMEOUT,       // precharge voltage did not meet threshold in time
    PRECHARGE_SENSE_TIMEOUT, // precharge contactor sense did not return in time
    MOTOR_SENSE_TIMEOUT,     // motor contactor sense did not return in time
    CAR_DRIVABLE,            // car state is drivable
    CAR_DRIVING,             // car state is driving
    CAR_CRUISE,              // car state is cruise
    CAR_REGEN,               // car state is regen
    CAR_BPSFAULT,            // received BPS fault message
    CAR_HB,                  // car state HB led
    HALL_EFFECT,             // moco hall effect sensor fault
    WATCHDOG,                // moco watchdog fault
    SWOC,                    // moco swoc fault
    MOTOR_FAULT,             // moco generic fault
    HB,                      // hb led
    NUM_LEDS,
} Status_Mapping_t;

/** @brief Sets a specific LED to on (true) or off (false). */
void LED_set(Status_Mapping_t LED, LED_state_t state);

/** @brief Turns off all LEDs. */
void LED_clear(void);

/** @brief Configures GPIO pins for all diagnostic LEDs. */
void LED_init(void);

/** @brief Toggle the LED */
void LED_toggle(Status_Mapping_t LED);
