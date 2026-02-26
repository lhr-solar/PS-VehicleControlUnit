#pragma once

#include <stdint.h>

/**
 * Refer to the can dbc. must be updated to match the dbc!!!!
 */
typedef enum {
    // BPS
    BPS_STATUS              = 0x1,
    
    // Controls
    CTRL_STATUS             = 0x15,
    CTRL_DRIVER_INPUTS      = 0x60,
    CTRL_STEERING_ANGLE     = 0x2B0,

    // Pedals
    PEDL_ACCEL_BRAKE_POS    = 0x50,
    PEDL_ACCEL_BRAKE_VOLTS  = 0x51, // only need this if pos is cooked

    // VCU
    VCU_STATUS              = 0x10,
    VCU_PRECHRG_VOLTS       = 0x21,
    
} CarCAN_CANID_e;


/**
 * Relevant documentation for prohelion msgs at 
 * https://docs.prohelion.com/Motor_Controllers/WaveSculptor22/User_Manual/Appendix_C.html
 */

// The controller CAN ID scheme is CAN ID = BASE_ADDRESS + OFFSET
#define MOTOR_CTRL_BASE_ADDR   0x400U
#define DRIVER_CTRL_BASE_ADDR  0x500U


typedef enum {
    // Drive Command IDs
    MOCO_CMD_MOTOR_DRIVE          = (uint16_t) (0x01 + DRIVER_CTRL_BASE_ADDR),
    MOCO_CMD_MOTOR_POWER          = (uint16_t) (0x02 + DRIVER_CTRL_BASE_ADDR),
    MOCO_CMD_RESET_SOFTWARE       = (uint16_t) (0x03 + DRIVER_CTRL_BASE_ADDR),

    // Broadcasted telemetry msgs
    MOCO_IDENTIFICATION_INFO      = (uint16_t) (0x00 + MOTOR_CTRL_BASE_ADDR),
    MOCO_STATUS_INFO              = (uint16_t) (0x01 + MOTOR_CTRL_BASE_ADDR), 
    MOCO_BUS_MEASUREMENT          = (uint16_t) (0x02 + MOTOR_CTRL_BASE_ADDR),
    MOCO_VELOCITY_MEASUREMENT     = (uint16_t) (0x03 + MOTOR_CTRL_BASE_ADDR), 
    MOCO_PHASE_CURRENT            = (uint16_t) (0x04 + MOTOR_CTRL_BASE_ADDR),
    MOCO_MOTOR_VOLTAGE_VECTOR     = (uint16_t) (0x05 + MOTOR_CTRL_BASE_ADDR), 
    MOCO_MOTOR_CURRENT_VECTOR     = (uint16_t) (0x06 + MOTOR_CTRL_BASE_ADDR), 
    MOCO_BACKEMF_MEASUREMENT      = (uint16_t) (0x07 + MOTOR_CTRL_BASE_ADDR), 
    MOCO_15V_RAIL_MEASUREMENT     = (uint16_t) (0x08 + MOTOR_CTRL_BASE_ADDR), 
    MOCO_3V3_19V_RAIL_MEASUREMENT = (uint16_t) (0x09 + MOTOR_CTRL_BASE_ADDR), 
    MOCO_HEAT_MOTOR_TEMP          = (uint16_t) (0x0B + MOTOR_CTRL_BASE_ADDR), 
    MOCO_DSP_BOARD_TEMP           = (uint16_t) (0x0C + MOTOR_CTRL_BASE_ADDR),
    MOCO_AMPHOURS_ODOMETER        = (uint16_t) (0x0E + MOTOR_CTRL_BASE_ADDR), 
    MOCO_CMD_SLIP_SPEED           = (uint16_t) (0x17 + MOTOR_CTRL_BASE_ADDR),

} VCUCAN_CANID_e;
