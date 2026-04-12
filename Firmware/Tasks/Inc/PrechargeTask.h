#pragma once

#include "ADC_Sense.h"
#include "CANbus.h"
#include "Contactors.h"
#include "FaultBits.h"
#include "InitTask.h"
#include "MotorSafeBits.h"
#include "StatusLEDs.h"
#include <string.h>

// Precharge thresholds
#define OVERVOLTAGE_THRESHOLD_MV      140000 // 140 V
#define UNDERVOLTAGE_THRESHOLD_MV     80000  // 80.0 V

// Fixed-point scaling for ratio comparisons
#define RATIO_SCALE                   1000

// 900/1000 = 0.900, 800/1000 = 0.800
// TODO: Test and increase hysterisis threshold closer to 90%
#define PRECHARGE_THRESHOLD_90        900
#define PRECHARGE_THRESHOLD_80        800
// NOTE: The second threshold exists to account for hysteresis, so that we don't drop out of
// the run state after successfully precharging just because of a small ADC reading change

// Allowed difference between motor and battery voltage
#define VOLTAGE_TOLERANCE_NUMERATOR   22
// Motor voltage can at most 10% higher than battery voltage
#define VOLTAGE_TOLERANCE_DENOMINATOR 20

// Precharge time to 90% -> 0.9 = 1 - e^(-t/RC), so t = -RC * ln(1-0.9) = 2.3*RC.
// For our case, R = 110 Ohms and C = 1 mF -> t = 253 ms.
#define PRECHARGE_TIMEOUT_MS          400
#define ADC_TIMEOUT_MS                20

#define PRECHARGE_STATE_LIST(X)                                                                    \
    X(WAITING)     /* Waiting for ignition state */                                                \
    X(INITIAL)     /* Precharge  hasn't started; closing main contactor and starting timer */      \
    X(PRECHARGING) /* Precharge started successfully, close contactor and check hysterisis */      \
    X(COMPLETE)    /* Precharge got through hysterisis, now continuously polling ADC */

typedef enum {
#define X(name) PRECHARGE_STATE_##name,
    PRECHARGE_STATE_LIST(X)
#undef X
    PRECHARGE_STATE_COUNT
} PrechargeState_e;

// Names for faults for printing/debugging purposes. Indexed by FaultID_e values
extern const char *precharge_state_names[];

#define PRECHARGE_STATE_BIT(state) (1UL << (state))
#define PRECHARGE_STATE_MASK    ((EventBits_t)((1UL << NUM_PRECHARGE_STATES) - 1UL))


/**
 * @brief Checks the repeated fault conditions for precharge sequence, if any fault condition is
 * met, will call fault handler and not return
 * @param Motor_Voltage most recent motor voltage reading in mV
 * @param Battery_Voltage most recent battery voltage reading in mV
 * @retval None
 */
void Fault_Checker(uint32_t Motor_Voltage, uint32_t Battery_Voltage);

/**
 * @brief Precharge task main execution function, implements precharge pseudo state machine and
 * fault handling
 * @param None
 * @retval None
 */
void Task_Precharge();

extern uint32_t battery_voltage;
extern uint32_t motor_voltage;