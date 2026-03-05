#pragma once

#include "stm32xx_hal.h"
#include "CANbus.h"
#include "CAN_FD.h"
#include "MotorCAN_can_msgs.h"

void Init_ReadMotorCAN();

void Task_ReadMotorCAN();
