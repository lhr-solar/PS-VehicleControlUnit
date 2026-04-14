#pragma once

#include "inits.h"
#include "CANbus.h"
#include "FSMTask.h"

#define BRAKE_THRESH      42.0f  // percent
#define BRAKE_THRESH_HYST 30.0f  // percent

#define ACCEPTABLE_PEDAL_DEVIATION 3 // percent

typedef struct {
    driver_input_status_t driver_input;
    pedal_status_t accel_brake;
    lws_standard_t lws;
    controls_status_t controls_status;
    mc_status_t motor_status;
    bps_status_t bps_status;
    mc_velocitymeasurement_t motor_velocity;
    set_motor_cmd_src_t motor_controls_src;
} VCUDataIn_t;

extern VCUDataIn_t * volatile g_data_read;
extern VCUDataIn_t * volatile g_data_write;

void Task_UpdateVCUInputs(void *args __attribute__((unused)));
