#pragma once

#include "stm32xx_hal.h"
#include "CAN_FD.h"

can_status_t Motor_CANBus_Init(void);

can_status_t Motor_CANBus_Send(FDCAN_TxHeaderTypeDef* header, uint8_t data[], TickType_t delay_ticks);

can_status_t Motor_CANBus_Recieve(uint16_t id, FDCAN_RxHeaderTypeDef* header, uint8_t data[], TickType_t delay_ticks);

can_status_t Car_CANBus_Send(FDCAN_TxHeaderTypeDef* header, uint8_t data[], TickType_t delay_ticks);

can_status_t Car_CANBus_Recieve(uint16_t id, FDCAN_RxHeaderTypeDef* header, uint8_t data[], TickType_t delay_ticks);