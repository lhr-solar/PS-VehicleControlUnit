#pragma once

#include "StatusLEDs.h"
#include "InitTask.h"
#include "MotorSafeBits.h"
#include "CANbus.h"
#include "CarCAN_can_msgs.h"
#include <string.h>
#include "PrechargeTask.h"

// enables the fdcan3 recieve hook, calls can_fd_rx_callback_hook everytime a can rx interrupt happens
#define FDCAN3_RECV_HOOK_EN
#define DRIVER_INPUT_QUEUE_SIZE 32
#define BPS_QUEUE_SIZE 32
#define IGNITION_MOTOR_INDEX 1 // index of motor ignition in driver input status message
#define CAN_BLOCKING_TIME_MS 100

extern uint8_t Start_Precharge;
extern uint8_t End_Precharge;

void Task_VCUReceiveCAN();