#pragma once

#include "inits.h"
#include "CANbus.h"
#include "FSMTask.h"

#define BRAKE_THRESH      2000.0f  // psi
#define BRAKE_THRESH_HYST 0.5f  // psi

#define ACCEPTABLE_PEDAL_DEVIATION 3 // percent

typedef struct {
    driver_input_status_t driver_input;
    pedal_status_t accel_brake;
    brake_pressure_1_t brake_pressure1;
    brake_pressure_2_t brake_pressure2;
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
