#pragma once

#include "MotorCAN_can_msgs.h"
#include "stm32xx_hal.h"
#include <string.h>
#include "CANbus.h"
#include "inits.h"
#include "MotorSafeBits.h"
#include "InitTask.h"
#include "StatusLEDs.h"

// this one is just used for testing. TODO: swap out once controls integration starts
static const EventBits_t motorDrivableBits =    MOTOR_STATUS_BIT(MOTOR_CONTACTOR_ENABLED) 
                                            |  MOTOR_STATUS_BIT(MOTOR_PRECHARGE_CONTACTOR_ENABLED);
// Real motor bits
// static const EventBits_t motorDrivableBits =    MOTOR_STATUS_BIT(MOTOR_CONTACTOR_ENABLED) 
//                                             |  MOTOR_STATUS_BIT(MOTOR_PRECHARGE_CONTACTOR_ENABLED)
//                                             |  MOTOR_STATUS_BIT(PEDALS_READING_ACCELERATOR)
//                                             |  MOTOR_STATUS_BIT(PEDALS_READING_BRAKE)
//                                             |  MOTOR_STATUS_BIT(DASHBOARD_IGNITION_MOTOR)
//                                             |  MOTOR_STATUS_BIT(BPS_SAFE);


void MotorControlTask_Init(void);

void Task_MotorControl();
