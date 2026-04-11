#pragma once

#include "stm32xx_hal.h"
#include "CAN_FD.h"

extern FDCAN_HandleTypeDef *hfdcan1;
extern FDCAN_HandleTypeDef *hfdcan3;

extern FDCAN_HandleTypeDef *motorfdcan;
extern FDCAN_HandleTypeDef *carfdcan;

can_status_t CAN_Init(void);

can_status_t Motor_CANBus_Send(FDCAN_TxHeaderTypeDef *header,
                               uint8_t data[],
                               TickType_t delay_ticks);

can_status_t Motor_CANBus_Receive(uint16_t id,
                                  FDCAN_RxHeaderTypeDef *header,
                                  uint8_t data[],
                                  TickType_t delay_ticks);

can_status_t Car_CANBus_Send(FDCAN_TxHeaderTypeDef *header,
                             uint8_t data[],
                             TickType_t delay_ticks);

can_status_t Car_CANBus_Receive(uint16_t id,
                                FDCAN_RxHeaderTypeDef *header,
                                uint8_t data[],
                                TickType_t delay_ticks);

/* HAL MSP hooks implemented in CANbus.c */
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *fdcanHandle);
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef *fdcanHandle);
