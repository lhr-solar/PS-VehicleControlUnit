#ifndef RECEIVE_MOTOR_H
#define RECEIVE_MOTOR_H

#include <stm32g4xx.h>
#include <stdint.h>
#include "stm32g4xx.h"
#include "FreeRTOS.h"
#include "semphr.h"
#define MOCO_BASE_ADDR 0x69420

typedef enum {
    MOTOR_IDENTITY       = 0x00,  
    MOTOR_ERROR_STATUS   = 0x01,
    BUS_STATUS           = 0x02,
    MOTOR_VELOCITY       = 0x03,
    MOTOR_PHASE_CURRENT  = 0x04,
    MOTOR_VOLTAGE_VECTOR = 0x05,  // Vd, Vq
    MOTOR_CURRENT_VECTOR = 0x06,  // Id, Iq
    MOTOR_BACK_EMF       = 0x07,  // BEMFd, BEMFq
    RAIL_15V_STATUS      = 0x08,  // 15V rail
    RAIL_3V3_1V9_STATUS  = 0x09,  // 3.3V + 1.9V rails
    // 0x0A reserved
    HEATSINK_MOTOR_TEMP  = 0x0B,  // heatsink + motor temp
    DSP_BOARD_TEMP       = 0x0C,  // DSP board temp
    // 0x0D reserved
    ODOMETER_BUS_AH      = 0x0E,  // DC bus Ah + odometer
    // 0x0F .. 0x16 reserved
    SLIP_SPEED           = 0x17,  // slip speed
    MAX_STATUS          = 0x18
} moco_status_t;

//may want to receive can messages faster than the broadcast rate, so maybe add a method to quick send as well as periodic recieve

//list of motor status messages to poll/receive
static const moco_status_t poll_list[] = {
    MOTOR_IDENTITY,
    MOTOR_ERROR_STATUS,
    BUS_STATUS,
    MOTOR_VELOCITY,
    MOTOR_PHASE_CURRENT,
    MOTOR_VOLTAGE_VECTOR,
    MOTOR_CURRENT_VECTOR,
    MOTOR_BACK_EMF,
    RAIL_15V_STATUS,
    RAIL_3V3_1V9_STATUS,
    HEATSINK_MOTOR_TEMP,
    DSP_BOARD_TEMP,
    ODOMETER_BUS_AH,
    SLIP_SPEED,
};

#define MOCO_FULL_STATUS_ARR_LEN sizeof(poll_list)/sizeof(moco_status_t)

extern uint64_t moco_full_status_arr[MOCO_FULL_STATUS_ARR_LEN];
extern SemaphoreHandle_t moco_status_lock;

void initStatusEventGroup();

#endif /* RECEIVE_MOTOR_H */
