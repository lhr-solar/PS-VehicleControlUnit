#pragma once

#include "CAN_FD.h"
#include "stm32xx_hal.h"
#include "MotorCAN_can_msgs.h"
#include "BPSCAN_can_msgs.h"
#include "CarCAN_can_msgs.h"
#include "SteeringCAN_can_msgs.h"

extern FDCAN_HandleTypeDef *motorfdcan;
extern FDCAN_HandleTypeDef *carfdcan;

can_status_t MotorCAN_Init(void);
can_status_t MotorCAN_Send(FDCAN_TxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks);
can_status_t MotorCAN_Recv(uint32_t id, FDCAN_RxHeaderTypeDef *header, uint8_t data[],
                           TickType_t delay_ticks);

can_status_t MotorCAN_Recv_Status(mc_status_t *out, TickType_t delay);
can_status_t MotorCAN_Recv_Velocity(mc_velocitymeasurement_t *out, TickType_t delay);
can_status_t MotorCAN_Recv_Control_Src(set_motor_cmd_src_t *out, TickType_t delay);

can_status_t MotorCAN_Send_Drive_Cmd(float velocity, float current, TickType_t delay);
can_status_t MotorCAN_Send_Power_Cmd(float current, TickType_t delay);

can_status_t CarCAN_Init(void);
can_status_t CarCAN_Send(FDCAN_TxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks);
can_status_t CarCAN_Recv(uint32_t id, FDCAN_RxHeaderTypeDef *header, uint8_t data[],
                                TickType_t delay_ticks);

can_status_t CarCAN_Recv_BPS_Status(bps_status_t *out, TickType_t delay);
can_status_t CarCAN_Recv_LWS(lws_standard_t *out, TickType_t delay);
can_status_t CarCAN_Recv_Driver_Input(driver_input_status_t *out, TickType_t delay);
can_status_t CarCAN_Recv_Pedals_Position(pedal_status_t *out, TickType_t delay);
can_status_t CarCAN_Recv_Brake_Pressure1(brake_pressure_1_t *out, TickType_t delay);
can_status_t CarCAN_Recv_Brake_Pressure2(brake_pressure_2_t *out, TickType_t delay);
can_status_t CarCAN_Recv_Controls_Status(controls_status_t *out, TickType_t delay);
can_status_t CarCAN_Send_Precharge_Voltages(uint32_t motor_mv, uint32_t battery_mv,
                                            TickType_t delay);
can_status_t CarCAN_Send_Drive_Cmd(float velocity, float current, TickType_t delay);

can_status_t CAN_Send_Drive_Cmd(float velocity, float current, TickType_t delay);


/* HAL MSP hooks implemented in CANbus.c */
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *fdcanHandle);
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef *fdcanHandle);