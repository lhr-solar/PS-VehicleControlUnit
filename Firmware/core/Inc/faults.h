/**
 * @file faults.h
 * @brief Fault management for VCU firmware
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 *
 * Fault bits are stored in a FreeRTOS EventGroup.
 * Motor controller faults (bits 0-8) map directly to mc_status error flags
 * so they can be set with a single bitfield shift in getMotorStatus().
 */

#pragma once

#include "FreeRTOS.h"
#include "event_groups.h"
#include <stdint.h>
#include <stdbool.h>

#define FAULT_ID_LIST(X) \
    /* moco faults */ \
    X(MOTOR_HARDWARE_OVERCURRENT) \
    X(MOTOR_SOFTWARE_OVERCURRENT) \
    X(MOTOR_DC_BUS_OVERVOLTAGE) \
    X(MOTOR_BAD_HALL_SEQUENCE) \
    X(MOTOR_WD_RESET) \
    X(MOTOR_CONFIG_READ) \
    X(MOTOR_15V_UNDERVOLTAGE) \
    X(MOTOR_DESATURATION) \
    X(MOTOR_OVERSPEED) \
    /* precharge faults (prolly need to add more) */ \
    X(PRECHARGE_TIMEOUT) \
    /* other boards*/ \
    X(STEERING_SENSOR_WATCHDOG) \
    X(STEERING_SENSOR_BAD_DATA) \
    X(PEDAL_BOARD_WATCHDOG) \
    X(PEDAL_SENSOR_BAD_DATA) \
    X(PEDAL_BOARD_FAULT) \
    X(CONTROLS_STATUS_WATCHDOG) \
    X(CONTROLS_FAULT) \
    X(BPS_STATUS_WATCHDOG) \
    X(BPS_FAULT)

// Fault IDs, each one maps to one bit in the fault event group
typedef enum {
#define X(name) FAULT_ID_##name,
    FAULT_ID_LIST(X)
#undef X
    FAULT_ID_COUNT
} FaultID_e;

 // bit masks for each fault for easy or-ing
typedef enum {
#define X(name) FAULT_ID_##name##_BIT = (1U << FAULT_ID_##name),
    FAULT_ID_LIST(X)
#undef X
} FaultIDBit_t;


// make sure only 24 bits because thats all Freertos can do
_Static_assert(FAULT_ID_COUNT <= 24, "Too many fault bits for fault EventGroup");

// All motor fault bits as a mask (for bulk motor fault operations)
#define FAULT_MASK_MOTOR_ALL  ((1U << FAULT_ID_MOTOR_OVERSPEED + 1) - 1)
#define FAULT_MASK_ALL        ((1U << FAULT_ID_COUNT) - 1)

void Faults_Init(void);

// Throw a single fault by ID
void Faults_ThrowFault(FaultID_e fault_id);

// Throw multiple faults at once using a pre-shifted bitfield
void Faults_ThrowFaultsUsingBitfield(EventBits_t fault_bits);

// Clear a single fault (for recoverable faults only)
void Faults_ClearFault(FaultID_e fault_id);

// Check if any fault is currently active
bool Faults_AnyActive(void);

// Check if a specific fault is active
bool Faults_IsActive(FaultID_e fault_id);

// Get the full fault bitfield
EventBits_t Faults_GetCurrentFaults(void);



void Task_FaultHandler(void);