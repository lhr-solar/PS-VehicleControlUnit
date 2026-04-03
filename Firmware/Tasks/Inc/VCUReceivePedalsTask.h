#pragma once

#include "inits.h"
#include "CarCAN_can_msgs.h"
#include "CANbus.h"
#include "CAN_FD.h"
#include <string.h>

void ReceivePedalsTask_Init(void);

void can_fd_rx_callback_hook(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs, can_rx_payload_t recv_payload);

void Task_ReceivePedals();