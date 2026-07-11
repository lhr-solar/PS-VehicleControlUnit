/**
 * @file motor.h
 * @brief Motor command calculations (current mapping, ramp, rollover limit)
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** Set to 1 to apply steering-angle rollover speed limiting; 0 to disable. */
#define APPLY_ROLLOVER 0

/**
 * Max-current vs speed curve mode for drive current:
 *   MOTOR_MAX_CURRENT_MODE_STEPS     — step thresholds in mph (vehicle speed)
 *   MOTOR_MAX_CURRENT_MODE_PIECEWISE — linear segments in rps/Hz (motor speed)
 */
#define MOTOR_MAX_CURRENT_MODE_STEPS     0
#define MOTOR_MAX_CURRENT_MODE_PIECEWISE 1

#define MOTOR_MAX_CURRENT_MODE MOTOR_MAX_CURRENT_MODE_PIECEWISE

#define MOTOR_MAX_RPM             12000
#define MOTOR_MAX_CURRENT_PERCENT 1.0f

/* Minimum pedal percent to register accel input, prevents ghost inputs */
#define MOTOR_ACCEL_DEADZONE_MIN  5u
#define MOTOR_METERS_SEC_TO_MPH   2.23694f

/* Max increase in commanded current (0.0-1.0) allowed per second. Decreases
 * are never limited. Untested against hardware — tune while watching for
 * MC_FAULT_SoftwareOverCurrent. */
#define MOTOR_CURRENT_RAMP_PER_SECOND 0.3f

/* Must match FSM_TASK_DELAY_MS in InitTask.h */
#define MOTOR_UPDATE_PERIOD_MS 100

#define MOTOR_CURRENT_RAMP_PER_TICK \
    (MOTOR_CURRENT_RAMP_PER_SECOND * ((float)MOTOR_UPDATE_PERIOD_MS / 1000.0f))

#define MOTOR_HARD_LIM_CURR_PWR   210.0f
#define MOTOR_HARD_LIM_CURR_DRIVE 200.0f

#define MOTOR_SOFT_LIM_CURR_PWR   60.0f
#define MOTOR_SOFT_LIM_CURR_DRIVE 70.0f

/** Step threshold: at/above speed_mph, max_current applies (0.0–1.0). */
typedef struct {
    float speed_mph;
    float max_current;
} motor_max_current_step_t;

/**
 * Linear segment: current lerps from start_current → end_current between
 * start_rps and end_rps (motor mechanical Hz = rpm/60). Add as many as needed.
 */
typedef struct {
    float start_rps;
    float end_rps;
    float start_current; /* 0.0–1.0 */
    float end_current;   /* 0.0–1.0 */
} motor_max_current_segment_t;

/**
 * @brief Speed-based max drive current fraction (0.0–1.0).
 * @param motor_rpm            MC_MotorVelocity (rpm); used in piecewise mode
 * @param vehicle_velocity_mps MC_VehicleVelocity (m/s); used in steps mode
 */
float motor_get_max_current(float motor_rpm, float vehicle_velocity_mps);

/**
 * @brief Drive current command (0.0–1.0):
 *        (soft/hard) * fmin(rollover_limit, max_current) * pedal
 */
float motor_get_drive_current(float motor_rpm, float vehicle_velocity_mps,
                              int16_t lws_angle, uint8_t accel_percent_0_100);

/** Power command current from pedal (unchanged soft/hard mapping). */
float motor_get_pwr_current(uint8_t accel_percent_0_100);

/** Rate-limit increases in commanded current; decreases pass through. */
float motor_ramp_current(float target_current);

/** Reset the current ramp accumulator (e.g. on leaving drive). */
void motor_reset_current_ramp(void);

/** True while rollover limiting is actively zeroing current. */
bool motor_is_over_rollover_speed(void);
