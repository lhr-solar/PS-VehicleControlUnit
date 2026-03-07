#pragma once

#include <stdint.h>
#include "inits.h"

#define num_LEDs 15

/** * @brief LED States  */
typedef enum
{
    OFF = 0, // Negative logic
    ON = 1
} LED_state_t;

/** * @brief Logic-to-Hardware mapping for diagnostic LEDs.
 * @note Values correspond to specific shift register positions.
 */
typedef enum
{
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
    HB                       // hb led
} Status_Mapping_t;

/** @brief Sets a specific LED to on (true) or off (false). */
void LED_set(Status_Mapping_t LED, LED_state_t state);

/** @brief Turns off all LEDs. */
void LEDs_clear(void);

/** @brief Configures GPIO pins for all diagnostic LEDs. */
void LEDs_init(void);

/** @brief Toggle the LED */
void Toggle_LED(Status_Mapping_t LED);

/** @brief sets all LEDs based on LEDbitmap */
void update_status(void);