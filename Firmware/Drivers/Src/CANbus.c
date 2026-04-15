#include "CANbus.h"
#include "BPSCAN_can_msgs.h"
#include "CarCAN_can_msgs.h"
#include "MotorCAN_can_msgs.h"
#include "UpdateVCUInputsTask.h"
#include "Watchdogs.h"
#include <math.h>
#include <string.h>


#define FDCAN_NVIC_PRIO configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 5

FDCAN_HandleTypeDef *motorfdcan;
FDCAN_HandleTypeDef *carfdcan;

can_status_t MotorCAN_Init(void) {
    motorfdcan = hfdcan1;
    motorfdcan->Instance = FDCAN1;

    motorfdcan->Init.ClockDivider = FDCAN_CLOCK_DIV1;
    motorfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    motorfdcan->Init.Mode = FDCAN_MODE_NORMAL;
    motorfdcan->Init.AutoRetransmission = ENABLE;
    motorfdcan->Init.TransmitPause = DISABLE;
    motorfdcan->Init.ProtocolException = DISABLE;
    motorfdcan->Init.NominalPrescaler = 20;
    motorfdcan->Init.NominalSyncJumpWidth = 1;
    motorfdcan->Init.NominalTimeSeg1 = 13;
    motorfdcan->Init.NominalTimeSeg2 = 2;
    motorfdcan->Init.DataPrescaler = 1;
    motorfdcan->Init.DataSyncJumpWidth = 1;
    motorfdcan->Init.DataTimeSeg1 = 1;
    motorfdcan->Init.DataTimeSeg2 = 1;
    motorfdcan->Init.StdFiltersNbr = 1;
    motorfdcan->Init.ExtFiltersNbr = 0;
    motorfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    // accepts all CAN IDs
    FDCAN_FilterTypeDef sFilterConfig1;
    sFilterConfig1.IdType = FDCAN_STANDARD_ID;
    sFilterConfig1.FilterIndex = 0;
    sFilterConfig1.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig1.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; // directs frames to FIFO0

    if (can_fd_init(motorfdcan, &sFilterConfig1) != CAN_OK) {
        return CAN_ERR;
    }

    if (can_fd_start(motorfdcan) != CAN_OK) {
        return CAN_ERR;
    }

    return CAN_OK;
}

can_status_t MotorCAN_Send(FDCAN_TxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks) {
    return can_fd_send(motorfdcan, header, data, delay_ticks);
}

can_status_t MotorCAN_Recv(uint32_t id, FDCAN_RxHeaderTypeDef *header, uint8_t data[],
                           TickType_t delay_ticks) {
    return can_fd_recv(motorfdcan, id, header, data, delay_ticks);
}

can_status_t MotorCAN_Recv_Velocity(mc_velocitymeasurement_t *out, TickType_t delay) {
    if (out == NULL) return CAN_EMPTY;

    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t motor_vel_rx_data[CAN_DLC_MC_VELOCITYMEASUREMENT] = {0};

    can_status_t result =
        can_fd_recv(motorfdcan, CAN_ID_MC_VELOCITYMEASUREMENT, &header, motor_vel_rx_data, delay);

    if (result == CAN_OK) {
        out->MC_MotorVelocity = *((float *)&motor_vel_rx_data[0]);
        out->MC_VehicleVelocity = *((float *)&motor_vel_rx_data[4]);
        watchdog_received_can_message(WD_IDX_MOCO_VELOCITY);
    }
    return result;
}

can_status_t MotorCAN_Recv_Status(mc_status_t *out, TickType_t delay) {
    if (out == NULL) return CAN_EMPTY;

    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t moco_status_rx_data[CAN_DLC_MC_STATUS] = {0};

    can_status_t result =
        can_fd_recv(motorfdcan, CAN_ID_MC_STATUS, &header, moco_status_rx_data, delay);

    if (result == CAN_OK) {
        out->MC_LIMIT_OutputVoltagePWM = (moco_status_rx_data[0] >> 0) & 0x1;
        out->MC_LIMIT_MotorCurrent = (moco_status_rx_data[0] >> 1) & 0x1;
        out->MC_LIMIT_Velocity = (moco_status_rx_data[0] >> 2) & 0x1;
        out->MC_LIMIT_BusCurrent = (moco_status_rx_data[0] >> 3) & 0x1;
        out->MC_LIMIT_BusVoltageUpper = (moco_status_rx_data[0] >> 4) & 0x1;
        out->MC_LIMIT_BusVoltageLower = (moco_status_rx_data[0] >> 5) & 0x1;
        out->MC_LIMIT_MotorTemp = (moco_status_rx_data[0] >> 6) & 0x1;
        out->MC_LIMIT_Reserved = ((uint16_t)(moco_status_rx_data[0] >> 7) & 0x1) |
                                 ((uint16_t)moco_status_rx_data[1] << 1);

        out->MC_FAULT_HardwareOverCurrent = (moco_status_rx_data[2] >> 0) & 0x1;
        out->MC_FAULT_SoftwareOverCurrent = (moco_status_rx_data[2] >> 1) & 0x1;
        out->MC_FAULT_DcBusOverVoltage = (moco_status_rx_data[2] >> 2) & 0x1;
        out->MC_FAULT_BadMotorPositionHallSeq = (moco_status_rx_data[2] >> 3) & 0x1;
        out->MC_FAULT_WatchdogCausedLastReset = (moco_status_rx_data[2] >> 4) & 0x1;
        out->MC_FAULT_ConfigRead = (moco_status_rx_data[2] >> 5) & 0x1;
        out->MC_FAULT_15vRailUnderVoltage = (moco_status_rx_data[2] >> 6) & 0x1;
        out->MC_FAULT_DesaturationFault = (moco_status_rx_data[2] >> 7) & 0x1;
        out->MC_FAULT_MotorOverSpeed = (moco_status_rx_data[3] >> 0) & 0x1;
        out->MC_FAULT_Reserved = (moco_status_rx_data[3] >> 1) & 0x7F;

        out->MC_ActiveMotor =
            (uint16_t)moco_status_rx_data[4] | ((uint16_t)moco_status_rx_data[5] << 8);
        out->MC_TxErrorCount = moco_status_rx_data[6];
        out->MC_RxErrorCount = moco_status_rx_data[7];

        watchdog_received_can_message(WD_IDX_MOCO_STATUS);
    }
    
    return result;
}

can_status_t MotorCAN_Recv_Control_Src(set_motor_cmd_src_t *out, TickType_t delay) {
    if (out == NULL) return CAN_EMPTY;

    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t moco_control_src_rx_data[CAN_DLC_SET_MOTOR_CMD_SRC] = {0};

    can_status_t result =
        can_fd_recv(motorfdcan, CAN_ID_SET_MOTOR_CMD_SRC, &header, moco_control_src_rx_data, delay);

    if (result == CAN_OK) {
        out->Motor_Command_Source = moco_control_src_rx_data[0];
    }
    
    return result;
}


can_status_t MotorCAN_Send_Drive_Cmd(float velocity, float current, TickType_t delay) {
    // printf("Motor CAN send drive cmd: %f vel, %f curr", velocity, current);
    if (!isfinite(velocity) || !isfinite(current)) return CAN_EMPTY;
    if (g_data_read->motor_controls_src.Motor_Command_Source) return CAN_OK;

    FDCAN_TxHeaderTypeDef header = {
        .Identifier = CAN_ID_MC_DRIVECOMMAND,
        .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = FDCAN_DLC_BYTES(CAN_DLC_MC_DRIVECOMMAND),
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0,
    };

    uint8_t moco_drive_tx_data[CAN_DLC_MC_DRIVECOMMAND] = {0};

    memcpy(&moco_drive_tx_data[0], &velocity, sizeof(float));
    memcpy(&moco_drive_tx_data[4], &current, sizeof(float));

    can_status_t result =
        can_fd_send(motorfdcan, &header, moco_drive_tx_data, delay);

    return result; 
}

can_status_t MotorCAN_Send_Power_Cmd(float current, TickType_t delay){
     // printf("Motor CAN send drive cmd: %f vel, %f curr", velocity, current);
    if (!isfinite(current)) return CAN_EMPTY;
    if (g_data_read->motor_controls_src.Motor_Command_Source) return CAN_OK;

    FDCAN_TxHeaderTypeDef header = {
        .Identifier = CAN_ID_MC_POWERCOMMAND,
        .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = FDCAN_DLC_BYTES(CAN_DLC_MC_POWERCOMMAND),
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0,
    };

    uint8_t moco_power_tx_data[CAN_DLC_MC_POWERCOMMAND] = {0};

    //first 4 bytes reserved
    memcpy(&moco_power_tx_data[4], &current, sizeof(float));

    can_status_t result =
        can_fd_send(motorfdcan, &header, moco_power_tx_data, delay);

    return result; 
}


///////// Carcan

can_status_t CarCAN_Init(void) {
    carfdcan = hfdcan3;
    carfdcan->Instance = FDCAN3;

    carfdcan->Init.ClockDivider = FDCAN_CLOCK_DIV1;
    carfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    carfdcan->Init.Mode = FDCAN_MODE_NORMAL;
    carfdcan->Init.AutoRetransmission = DISABLE;
    carfdcan->Init.TransmitPause = DISABLE;
    carfdcan->Init.ProtocolException = DISABLE;
    carfdcan->Init.NominalPrescaler = 20;
    carfdcan->Init.NominalSyncJumpWidth = 1;
    carfdcan->Init.NominalTimeSeg1 = 13;
    carfdcan->Init.NominalTimeSeg2 = 2;
    carfdcan->Init.DataPrescaler = 1;
    carfdcan->Init.DataSyncJumpWidth = 1;
    carfdcan->Init.DataTimeSeg1 = 1;
    carfdcan->Init.DataTimeSeg2 = 1;
    carfdcan->Init.StdFiltersNbr = 1;
    carfdcan->Init.ExtFiltersNbr = 0;
    carfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    // accepts all CAN IDs from
    // FDCAN1 Filter Config
    FDCAN_FilterTypeDef CarCANFilterConfig;
    CarCANFilterConfig.IdType = FDCAN_STANDARD_ID;
    CarCANFilterConfig.FilterIndex = 0;
    CarCANFilterConfig.FilterType = FDCAN_FILTER_MASK;
    CarCANFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; // directs frames to FIFO0
    CarCANFilterConfig.FilterID1 = 0x000;
    CarCANFilterConfig.FilterID2 = 0x000;

    if (can_fd_init(carfdcan, &CarCANFilterConfig) != CAN_OK) {
        return CAN_ERR;
    }

    if (can_fd_start(carfdcan) != CAN_OK) {
        return CAN_ERR;
    }

    return CAN_OK;
}

can_status_t CarCAN_Send(FDCAN_TxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks) {
    return can_fd_send(carfdcan, header, data, delay_ticks);
}

can_status_t CarCAN_Recv(uint32_t id, FDCAN_RxHeaderTypeDef *header, uint8_t data[],
                         TickType_t delay_ticks) {
    return can_fd_recv(carfdcan, id, header, data, delay_ticks);
}

can_status_t CarCAN_Recv_BPS_Status(bps_status_t *out, TickType_t delay) {
    if (out == NULL) return CAN_EMPTY;

    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t bps_status_rx_data[CAN_DLC_BPS_STATUS] = {0};

    can_status_t result =
        can_fd_recv(carfdcan, CAN_ID_BPS_STATUS, &header, bps_status_rx_data, delay);

    if (result == CAN_OK) {
        out->BPS_Fault = bps_status_rx_data[0];
        out->BPS_Regen_OK = (bps_status_rx_data[1] >> 0) & 1;
        out->BPS_Charge_OK = (bps_status_rx_data[1] >> 1) & 1;
        out->HV_Plus_Contactor_State = (bps_status_rx_data[1] >> 2) & 1;
        out->HV_Minus_Contactor_State = (bps_status_rx_data[1] >> 3) & 1;
        out->Array_Contactor_State = (bps_status_rx_data[1] >> 4) & 1;
        out->Array_Precharge_Contactor_State = (bps_status_rx_data[1] >> 5) & 1;
        out->Main_Battery_Voltage = (uint32_t)(bps_status_rx_data[4] | 
                                    ((uint32_t)bps_status_rx_data[5] << 8) |
                                    ((uint32_t)bps_status_rx_data[6] << 16) |
                                    ((uint32_t)bps_status_rx_data[7] << 24));
        out->Main_Battery_Avg_Temperature = (int16_t)((uint16_t)bps_status_rx_data[2] | ((uint16_t)bps_status_rx_data[3] << 8));

        watchdog_received_can_message(WD_IDX_BPS_STATUS);
    }
    return result;
}

can_status_t CarCAN_Recv_Controls_Status(controls_status_t *out, TickType_t delay) {
    if (out == NULL) return CAN_EMPTY;

    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t controls_status_rx_data[CAN_DLC_CONTROLS_STATUS] = {0};

    can_status_t result =
        can_fd_recv(carfdcan, CAN_ID_CONTROLS_STATUS, &header, controls_status_rx_data, delay);

    if (result == CAN_OK) {
        out->Controls_Leader_Fault = controls_status_rx_data[0];
        // out->Controls_Lighting_Fault = (uint16_t) (controls_status_rx_data[1]);
        watchdog_received_can_message(WD_IDX_CONTROLS_STATUS);
    }

    return result;
}

can_status_t CarCAN_Recv_LWS(lws_standard_t *out, TickType_t delay) {
    if (out == NULL) return CAN_EMPTY;

    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t steering_angle_rx_data[CAN_DLC_LWS_STANDARD] = {0};

    can_status_t result =
        can_fd_recv(carfdcan, CAN_ID_LWS_STANDARD, &header, steering_angle_rx_data, delay);

    if (result == CAN_OK) {
        // Angle: Byte0 (LSB), Byte1 (MSB)
        out->LWS_Angle = (int16_t)(
            (steering_angle_rx_data[1] << 8) |
             steering_angle_rx_data[0]);

        // Speed: Byte2
        out->LWS_Speed = steering_angle_rx_data[2];

        // Status bits: Byte3
        out->LWS_Fault     = (steering_angle_rx_data[3] >> 0) & 0x01;
        out->LWS_CalibrationStaus = (steering_angle_rx_data[3] >> 1) & 0x01;
        out->LWS_Trimming_Status = (steering_angle_rx_data[3] >> 2) & 0x01;

        watchdog_received_can_message(WD_IDX_STEERING_ANGLE);
    }

    return result;
}

can_status_t CarCAN_Recv_Driver_Input(driver_input_status_t *out, TickType_t delay) {
    if (out == NULL) return CAN_EMPTY;

    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t driver_input_rx_data[CAN_DLC_DRIVER_INPUT_STATUS] = {0};

    can_status_t result =
        can_fd_recv(carfdcan, CAN_ID_DRIVER_INPUT_STATUS, &header, driver_input_rx_data, delay);

    if (result == CAN_OK) {
        out->Ignition_Array   = !!(driver_input_rx_data[0] & (1U << 0));
        out->Ignition_Motor   = !!(driver_input_rx_data[0] & (1U << 1));
        out->Ignition_Off     = !!(driver_input_rx_data[0] & (1U << 2));
        out->Cruise_Enable    = !!(driver_input_rx_data[0] & (1U << 3));
        out->Cruise_Set       = !!(driver_input_rx_data[0] & (1U << 4));
        out->Gear_Forward     = !!(driver_input_rx_data[0] & (1U << 5));
        out->Gear_Neutral     = !!(driver_input_rx_data[0] & (1U << 6));
        out->Gear_Reverse     = !!(driver_input_rx_data[0] & (1U << 7));

        out->Hazard_Pressed      = !!(driver_input_rx_data[1] & (1U << 0));
        out->Horn_Pressed        = !!(driver_input_rx_data[1] & (1U << 1));
        out->Blinker_Left        = !!(driver_input_rx_data[1] & (1U << 2));
        out->Blinker_Right       = !!(driver_input_rx_data[1] & (1U << 3));
        out->PushToTalk_Pressed  = !!(driver_input_rx_data[1] & (1U << 4));
        out->Regen_Activate      = !!(driver_input_rx_data[1] & (1U << 5));
        out->Regen_Enable        = !!(driver_input_rx_data[1] & (1U << 6));

        watchdog_received_can_message(WD_IDX_DRIVER_INPUT);
    }

    return result;
}

can_status_t CarCAN_Recv_Pedals_Position(pedal_status_t *out, TickType_t delay) {
    if (out == NULL) return CAN_EMPTY;

    FDCAN_RxHeaderTypeDef header = {0};
    uint8_t pedals_pos_rx_data[CAN_DLC_PEDAL_STATUS] = {0};

    can_status_t result =
        can_fd_recv(carfdcan, CAN_ID_PEDAL_STATUS, &header, pedals_pos_rx_data, delay);

    if (result == CAN_OK) {
        out->AccelPedal_Main_Pos        = pedals_pos_rx_data[0];
        out->AccelPedal_Redundant_Pos   = pedals_pos_rx_data[1];
        out->BrakePedal_Main_Pos        = pedals_pos_rx_data[2];
        out->BrakePedal_Redundant_Pos   = pedals_pos_rx_data[3];

        out->AccelPedal_Main_Fault        = !!(pedals_pos_rx_data[4] & (1U << 0));
        out->AccelPedal_Redundant_Fault   = !!(pedals_pos_rx_data[4] & (1U << 1));
        out->BrakePedal_Main_Fault        = !!(pedals_pos_rx_data[4] & (1U << 2));
        out->BrakePedal_Redundant_Fault   = !!(pedals_pos_rx_data[4] & (1U << 3));

        watchdog_received_can_message(WD_IDX_ACCEL_BRAKE);
    }

    return result;
}

can_status_t CarCAN_Send_Precharge_Voltages(uint32_t motor_mv, uint32_t battery_mv, 
    TickType_t delay) {

    printf("Car CAN send prech voltages: %lu motor_mv,  %lu batt_mv\n\r", motor_mv, battery_mv);

    FDCAN_TxHeaderTypeDef header = {
        .Identifier = CAN_ID_VCU_PRECHARGE_VOLTAGES,
        .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = FDCAN_DLC_BYTES(CAN_DLC_VCU_PRECHARGE_VOLTAGES),
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0,
    };

    uint8_t vcu_prech_tx_data[CAN_DLC_VCU_PRECHARGE_VOLTAGES] = {0};

    memcpy(&vcu_prech_tx_data[0], &motor_mv, 3);
    memcpy(&vcu_prech_tx_data[3], &battery_mv, 3);

    can_status_t result =
        can_fd_send(carfdcan, &header, vcu_prech_tx_data, delay);

    return result; 
}

//////// HAL bs

static uint32_t HAL_RCC_FDCAN_CLK_ENABLED = 0;

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *fdcanHandle) {

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (fdcanHandle->Instance == FDCAN1) {
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
        PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        /* FDCAN1 clock enable */
        HAL_RCC_FDCAN_CLK_ENABLED++;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 1) {
            __HAL_RCC_FDCAN_CLK_ENABLE();
        }

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**FDCAN1 GPIO Configuration
        PA11     ------> FDCAN1_RX
        PA12     ------> FDCAN1_TX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* FDCAN1 interrupt Init */
        HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, FDCAN_NVIC_PRIO, 0);
        HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
        HAL_NVIC_SetPriority(FDCAN1_IT1_IRQn, FDCAN_NVIC_PRIO, 0);
        HAL_NVIC_EnableIRQ(FDCAN1_IT1_IRQn);
    } else if (fdcanHandle->Instance == FDCAN3) {
        /** Initializes the peripherals clocks
         */
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
        PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        /* FDCAN3 clock enable */
        HAL_RCC_FDCAN_CLK_ENABLED++;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 1) {
            __HAL_RCC_FDCAN_CLK_ENABLE();
        }

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**FDCAN3 GPIO Configuration
        PA8     ------> FDCAN3_RX
        PA15     ------> FDCAN3_TX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF11_FDCAN3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* FDCAN3 interrupt Init */
        HAL_NVIC_SetPriority(FDCAN3_IT0_IRQn, FDCAN_NVIC_PRIO, 0);
        HAL_NVIC_EnableIRQ(FDCAN3_IT0_IRQn);
        HAL_NVIC_SetPriority(FDCAN3_IT1_IRQn, FDCAN_NVIC_PRIO, 0);
        HAL_NVIC_EnableIRQ(FDCAN3_IT1_IRQn);
    }
}

void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef *fdcanHandle) {
    if (fdcanHandle->Instance == FDCAN1) {
        /* Peripheral clock disable */
        HAL_RCC_FDCAN_CLK_ENABLED--;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 0) {
            __HAL_RCC_FDCAN_CLK_DISABLE();
        }

        /**FDCAN1 GPIO Configuration
        PA11     ------> FDCAN1_RX
        PA12     ------> FDCAN1_TX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);

        /* FDCAN1 interrupt Deinit */
        HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
        HAL_NVIC_DisableIRQ(FDCAN1_IT1_IRQn);
    } else if (fdcanHandle->Instance == FDCAN3) {
        /* Peripheral clock disable */
        HAL_RCC_FDCAN_CLK_ENABLED--;
        if (HAL_RCC_FDCAN_CLK_ENABLED == 0) {
            __HAL_RCC_FDCAN_CLK_DISABLE();
        }

        /**FDCAN3 GPIO Configuration
        PA8     ------> FDCAN3_RX
        PA15     ------> FDCAN3_TX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8 | GPIO_PIN_15);

        /* FDCAN3 interrupt Deinit */
        HAL_NVIC_DisableIRQ(FDCAN3_IT0_IRQn);
        HAL_NVIC_DisableIRQ(FDCAN3_IT1_IRQn);
    }

    /**FDCAN3 GPIO Configuration
    PA8     ------> FDCAN3_RX
    PA15     ------> FDCAN3_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8 | GPIO_PIN_15);

    /* FDCAN3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(FDCAN3_IT0_IRQn);
    HAL_NVIC_DisableIRQ(FDCAN3_IT1_IRQn);
}