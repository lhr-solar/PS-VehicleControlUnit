/**
 * @file can_util.h
 * @brief CAN utility function declarations for VCU firmware
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#pragma once

#include <stdint.h>
#include "CarCAN_can_msgs.h"
#include "MotorCAN_can_msgs.h"
#include "BPSCAN_can_msgs.h"
#include "CAN_FD.h"

void can_init_all(void);
void can_start_all(void);

void carcan_send(uint16_t id, uint8_t *data, uint8_t len);
void vcucan_send(uint16_t id, uint8_t *data, uint8_t len);

void send_motor_drive_cmd(float velocity, float current);


typedef void (*can_msg_handler_t)(uint8_t *buf, void *input_buf);

// this is really just to avoid copy-pasting the same code a million times in 
// the FSM task, but it also nicely abstracts away the CAN receiving and watchdog petting logic
static inline void carcan_try_recv(uint32_t id, can_msg_handler_t handler, void *input_buf) {
    FDCAN_RxHeaderTypeDef hdr = {0};
    uint8_t buf[8] = {0};

    if (can_fd_recv(hfdcan3, id, &hdr, buf, 0) == CAN_OK) {
        handler(buf, input_buf);
    }
}

static inline void vcucan_try_recv(uint32_t id, can_msg_handler_t handler, void *input_buf) {
    FDCAN_RxHeaderTypeDef hdr = {0};
    uint8_t buf[8] = {0};

    if (can_fd_recv(hfdcan1, id, &hdr, buf, 0) == CAN_OK) {
        handler(buf, input_buf);
    }
}

void handle_motor_status(uint8_t *buf, void *motor_status);
void handle_motor_velocity(uint8_t *buf, void *motor_velocity);
void handle_controls_status(uint8_t *buf, void *controls_status);
void handle_bps(uint8_t *buf, void *bps_status);
void handle_lws(uint8_t *buf, void *steering_angle);
void handle_accel_brake(uint8_t *buf, void *accel_brake);
void handle_driver_input(uint8_t *buf, void *driver_input);