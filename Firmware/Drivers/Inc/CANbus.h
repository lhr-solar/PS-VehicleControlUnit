#pragma once

#include "CAN_FD.h"
#include "stm32xx_hal.h"

extern FDCAN_HandleTypeDef *motorfdcan;
extern FDCAN_HandleTypeDef *carfdcan;

can_status_t MotorCAN_Init(void);

can_status_t MotorCAN_Send(FDCAN_TxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks);

can_status_t MotorCAN_Recv(uint16_t id, FDCAN_RxHeaderTypeDef *header, uint8_t data[],
                           TickType_t delay_ticks);

can_status_t CarCAN_Init(void);

can_status_t CarCAN_Send(FDCAN_TxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks);

can_status_t CarCAN_Recv(uint16_t id, FDCAN_RxHeaderTypeDef *header, uint8_t data[],
                                TickType_t delay_ticks);