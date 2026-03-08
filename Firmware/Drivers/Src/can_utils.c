/**
 * @file can_utils.c
 * @brief CAN utility functions for VCU firmware
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 * 
 * This file contains utility functions for handling CAN messages, including
 * packing and unpacking messages, as well as any common CAN-related operations.
 */

#include "can_utils.h"
#include "CAN_FD.h"
#include "faults.h"
#include "watchdogs.h"
#include <string.h>

#define FDCAN_DLC_BYTES(len) \
((len) == 0 ? FDCAN_DLC_BYTES_0 : \
 (len) == 1 ? FDCAN_DLC_BYTES_1 : \
 (len) == 2 ? FDCAN_DLC_BYTES_2 : \
 (len) == 3 ? FDCAN_DLC_BYTES_3 : \
 (len) == 4 ? FDCAN_DLC_BYTES_4 : \
 (len) == 5 ? FDCAN_DLC_BYTES_5 : \
 (len) == 6 ? FDCAN_DLC_BYTES_6 : \
 (len) == 7 ? FDCAN_DLC_BYTES_7 : \
              FDCAN_DLC_BYTES_8)

void can_init_all(void) {
    // vcucan
    hfdcan1->Instance = FDCAN1;
    hfdcan1->Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan1->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan1->Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
    hfdcan1->Init.AutoRetransmission = DISABLE;
    hfdcan1->Init.TransmitPause = DISABLE;
    hfdcan1->Init.ProtocolException = DISABLE;
    hfdcan1->Init.NominalPrescaler = 20;
    hfdcan1->Init.NominalSyncJumpWidth = 1;
    hfdcan1->Init.NominalTimeSeg1 = 13;
    hfdcan1->Init.NominalTimeSeg2 = 2;
    hfdcan1->Init.DataPrescaler = 1;
    hfdcan1->Init.DataSyncJumpWidth = 1;
    hfdcan1->Init.DataTimeSeg1 = 1;
    hfdcan1->Init.DataTimeSeg2 = 1;
    hfdcan1->Init.StdFiltersNbr = 1;
    hfdcan1->Init.ExtFiltersNbr = 0;
    hfdcan1->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    FDCAN_FilterTypeDef vcucan_filter_config = {0};
    vcucan_filter_config.IdType = FDCAN_STANDARD_ID;
    vcucan_filter_config.FilterIndex = 0;
    vcucan_filter_config.FilterType = FDCAN_FILTER_MASK;
    vcucan_filter_config.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    vcucan_filter_config.FilterID1 = 0x000;
    vcucan_filter_config.FilterID2 = 0x000;

    can_fd_init(hfdcan1, &vcucan_filter_config);


    // carcan
    hfdcan3->Instance = FDCAN3;
    hfdcan3->Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan3->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan3->Init.Mode = FDCAN_MODE_INTERNAL_LOOPBACK;
    hfdcan3->Init.AutoRetransmission = DISABLE;
    hfdcan3->Init.TransmitPause = DISABLE;
    hfdcan3->Init.ProtocolException = DISABLE;
    hfdcan3->Init.NominalPrescaler = 20;
    hfdcan3->Init.NominalSyncJumpWidth = 1;
    hfdcan3->Init.NominalTimeSeg1 = 13;
    hfdcan3->Init.NominalTimeSeg2 = 2;
    hfdcan3->Init.DataPrescaler = 1;
    hfdcan3->Init.DataSyncJumpWidth = 1;
    hfdcan3->Init.DataTimeSeg1 = 1;
    hfdcan3->Init.DataTimeSeg2 = 1;
    hfdcan3->Init.StdFiltersNbr = 1;
    hfdcan3->Init.ExtFiltersNbr = 0;
    hfdcan3->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    FDCAN_FilterTypeDef carcan_filter_config = {0};
    carcan_filter_config.IdType = FDCAN_STANDARD_ID;
    carcan_filter_config.FilterIndex = 0;
    carcan_filter_config.FilterType = FDCAN_FILTER_MASK;
    carcan_filter_config.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    carcan_filter_config.FilterID1 = 0x000;
    carcan_filter_config.FilterID2 = 0x000;

    can_fd_init(hfdcan3, &carcan_filter_config);
}

void can_start_all(void) {
    can_fd_start(hfdcan1);
    can_fd_start(hfdcan3);
}

///// can sending

void carcan_send(uint16_t id, uint8_t *data, uint8_t len) {
    FDCAN_TxHeaderTypeDef hdr = {
        .Identifier = id,
        .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = FDCAN_DLC_BYTES(len),
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0
    };

    can_fd_send(hfdcan3, &hdr, data, 0);
}

void vcucan_send(uint16_t id, uint8_t *data, uint8_t len) {
    FDCAN_TxHeaderTypeDef hdr = {
        .Identifier = id,
        .IdType = FDCAN_STANDARD_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = FDCAN_DLC_BYTES(len),
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = FDCAN_BRS_OFF,
        .FDFormat = FDCAN_CLASSIC_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0
    };

    can_fd_send(hfdcan1, &hdr, data, 0);
}


void send_motor_drive_cmd(float velocity, float current) {
    uint8_t data[8] = {0};

    memcpy(&data[0], &velocity, sizeof(float));
    memcpy(&data[4], &current, sizeof(float));

    vcucan_send(CAN_ID_MC_DRIVECOMMAND, data, 8);
}


// unpack 


void unpack_driver_input(const uint8_t *b, driver_input_status_t *o) {
    o->Ignition_Array      = (b[0] >> 0) & 1;
    o->Ignition_Motor      = (b[0] >> 1) & 1;
    o->Ignition_Off        = (b[0] >> 2) & 1;
    o->Cruise_Enable       = (b[0] >> 3) & 1;
    o->Cruise_Set          = (b[0] >> 4) & 1;
    o->Gear_Forward        = (b[0] >> 5) & 1;
    o->Gear_Neutral        = (b[0] >> 6) & 1;
    o->Gear_Reverse        = (b[0] >> 7) & 1;
    o->Hazard_Pressed      = (b[1] >> 0) & 1;
    o->Horn_Pressed        = (b[1] >> 1) & 1;
    o->Blinker_Left        = (b[1] >> 2) & 1;
    o->Blinker_Right       = (b[1] >> 3) & 1;
    o->PushToTalk_Pressed  = (b[1] >> 4) & 1;
    o->Regen_Activate      = (b[1] >> 5) & 1;
    o->Regen_Enable        = (b[1] >> 6) & 1;
}

void unpack_accel_brake(const uint8_t *b, accel_brake_position_t *o) {
    o->Accel_Pos_Main            = b[0];
    o->Accel_Pos_Redundant       = b[1];
    o->Brake_Pos_Main            = b[2];
    o->Brake_Pos_Redundant       = b[3];
    o->Accel_Pos_Main_Fault      = (b[4] >> 0) & 1;
    o->Accel_Pos_Redundant_Fault = (b[4] >> 1) & 1;
    o->Brake_Pos_Main_Fault      = (b[4] >> 2) & 1;
    o->Brake_Pos_Redundant_Fault = (b[4] >> 3) & 1;
    o->Brake_Pressure_1_Fault    = (b[4] >> 4) & 1;
    o->Brake_Pressure_2_Fault    = (b[4] >> 5) & 1;
}

void unpack_lws(const uint8_t *b, lws_standard_t *o) {
    o->LWS_Angle = (int16_t)((uint16_t)b[0] | ((uint16_t)b[1] << 8));
    o->LWS_Speed = b[2];
    o->LWS_OK    = (b[3] >> 0) & 1;
    o->LWS_CAL   = (b[3] >> 1) & 1;
    o->LWS_TRIM  = (b[3] >> 2) & 1;
}

void unpack_controls_status(const uint8_t *b, controls_status_t *o) {
    o->Controls_Leader_Fault   = b[0];
    o->Controls_Lighting_Fault = (uint16_t)(b[2] | ((uint16_t)b[3] << 8));
}

void unpack_bps_status(const uint8_t *b, bps_status_t *o) {
    o->BPS_Fault = b[0];
    o->BPS_Regen_OK = (b[1] >> 0) & 1;
    o->BPS_Charge_OK = (b[1] >> 1) & 1;
    o->HV_Plus_Contactor_State = (b[1] >> 2) & 1;
    o->HV_Minus_Contactor_State = (b[1] >> 3) & 1;
    o->Array_Contactor_State = (b[1] >> 4) & 1;
    o->Array_Precharge_Contactor_State = (b[1] >> 5) & 1;
    o->Main_Battery_Voltage = (uint32_t)(b[4] | ((uint32_t)b[5] << 8) | ((uint32_t)b[6] << 16) | ((uint32_t)b[7] << 24));
    o->Main_Battery_Avg_Temperature = (int16_t)((uint16_t)b[2] | ((uint16_t)b[3] << 8));    
}

void unpack_motor_velocity(const uint8_t *b, mc_velocitymeasurement_t *o) {
    o->MC_MotorVelocity = *((float *)&b[0]);
    o->MC_VehicleVelocity = *((float *)&b[4]);
}

void unpack_motor_status(const uint8_t *b, mc_status_t *o) {
    o->MC_LIMIT_OutputVoltagePWM = (b[0] >> 0) & 0x1;
    o->MC_LIMIT_MotorCurrent     = (b[0] >> 1) & 0x1;
    o->MC_LIMIT_Velocity         = (b[0] >> 2) & 0x1;
    o->MC_LIMIT_BusCurrent       = (b[0] >> 3) & 0x1;
    o->MC_LIMIT_BusVoltageUpper  = (b[0] >> 4) & 0x1;
    o->MC_LIMIT_BusVoltageLower  = (b[0] >> 5) & 0x1;
    o->MC_LIMIT_MotorTemp        = (b[0] >> 6) & 0x1;
    o->MC_LIMIT_Reserved         = ((uint16_t)(b[0] >> 7) & 0x1) | 
                                   ((uint16_t)b[1] << 1);

    o->MC_FAULT_HardwareOverCurrent      = (b[2] >> 0) & 0x1;
    o->MC_FAULT_SoftwareOverCurrent      = (b[2] >> 1) & 0x1;
    o->MC_FAULT_DcBusOverVoltage         = (b[2] >> 2) & 0x1;
    o->MC_FAULT_BadMotorPositionHallSeq  = (b[2] >> 3) & 0x1;
    o->MC_FAULT_WatchdogCausedLastReset  = (b[2] >> 4) & 0x1;
    o->MC_FAULT_ConfigRead               = (b[2] >> 5) & 0x1;
    o->MC_FAULT_15vRailUnderVoltage      = (b[2] >> 6) & 0x1;
    o->MC_FAULT_DesaturationFault        = (b[2] >> 7) & 0x1;
    o->MC_FAULT_MotorOverSpeed           = (b[3] >> 0) & 0x1;
    o->MC_FAULT_Reserved                 = (b[3] >> 1) & 0x7F;

    o->MC_ActiveMotor = (uint16_t)b[4] | ((uint16_t)b[5] << 8);
    o->MC_TxErrorCount = b[6];
    o->MC_RxErrorCount = b[7];
}



/// handlers

void handle_driver_input(uint8_t *buf, void *driver_input) {
    driver_input_status_t *status = (driver_input_status_t *) driver_input;
    unpack_driver_input(buf, status);
    watchdog_received_can_message(WD_IDX_DRIVER_INPUT);
}

void handle_accel_brake(uint8_t *buf, void *accel_brake) {
    accel_brake_position_t *status = (accel_brake_position_t *) accel_brake;
    unpack_accel_brake(buf, accel_brake);
    watchdog_received_can_message(WD_IDX_ACCEL_BRAKE);

    if (status->Accel_Pos_Main_Fault || status->Accel_Pos_Redundant_Fault ||
        status->Brake_Pos_Main_Fault || status->Brake_Pos_Redundant_Fault ||
        status->Brake_Pressure_1_Fault || status->Brake_Pressure_2_Fault) {
        faults_throw_fault(FAULT_ID_PEDAL_BOARD_FAULT);
    }
}

void handle_lws(uint8_t *buf, void *lws) {
    lws_standard_t *status = (lws_standard_t *) lws;
    unpack_lws(buf,  status);
    watchdog_received_can_message(WD_IDX_STEERING_ANGLE);

    if (status->LWS_OK) {
        faults_throw_fault(FAULT_ID_STEERING_SENSOR_BAD_DATA);
    }
}

void handle_controls_status(uint8_t *buf, void *controls_status) {
    controls_status_t *status = (controls_status_t *) controls_status;
    unpack_controls_status(buf, status);
    watchdog_received_can_message(WD_IDX_CONTROLS_STATUS);

    if (status->Controls_Leader_Fault) {
        faults_throw_fault(FAULT_ID_CONTROLS_FAULT);
    }
}

void handle_bps(uint8_t *buf, void *bps_status) {
    bps_status_t *status = (bps_status_t *) bps_status;
    unpack_bps_status(buf, status);
    watchdog_received_can_message(WD_IDX_BPS_STATUS);

    if (status->BPS_Fault) {
        faults_throw_fault(FAULT_ID_BPS_FAULT);
    }
}

void handle_motor_velocity(uint8_t *buf, void *motor_velocity) {
    mc_velocitymeasurement_t *status = (mc_velocitymeasurement_t *) motor_velocity;
    unpack_motor_velocity(buf, status);
    watchdog_received_can_message(WD_IDX_MOCO_VELOCITY);
}

void handle_motor_status(uint8_t *buf, void *motor_status) {
    mc_status_t *status = (mc_status_t *)motor_status;
    unpack_motor_status(buf, status);
    watchdog_received_can_message(WD_IDX_MOCO_STATUS);

    EventBits_t active = 0;
    active |= (
        (!!status->MC_FAULT_HardwareOverCurrent       * FAULT_ID_MOTOR_HARDWARE_OVERCURRENT_BIT) |
        (!!status->MC_FAULT_SoftwareOverCurrent       * FAULT_ID_MOTOR_SOFTWARE_OVERCURRENT_BIT) |
        (!!status->MC_FAULT_DcBusOverVoltage          * FAULT_ID_MOTOR_DC_BUS_OVERVOLTAGE_BIT) |
        (!!status->MC_FAULT_BadMotorPositionHallSeq   * FAULT_ID_MOTOR_BAD_HALL_SEQUENCE_BIT) |
        (!!status->MC_FAULT_WatchdogCausedLastReset   * FAULT_ID_MOTOR_WD_RESET_BIT) |
        (!!status->MC_FAULT_ConfigRead                * FAULT_ID_MOTOR_CONFIG_READ_BIT) |
        (!!status->MC_FAULT_15vRailUnderVoltage       * FAULT_ID_MOTOR_15V_UNDERVOLTAGE_BIT) |
        (!!status->MC_FAULT_DesaturationFault         * FAULT_ID_MOTOR_DESATURATION_BIT) |
        (!!status->MC_FAULT_MotorOverSpeed            * FAULT_ID_MOTOR_OVERSPEED_BIT)
    );
    faults_throw_faults_using_bitfield(active);
}