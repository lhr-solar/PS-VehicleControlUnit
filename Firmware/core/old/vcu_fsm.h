#ifndef VCU_FSM_H
#define VCU_FSM_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// FSM State Enum
typedef enum {
    STATE_INIT              = 0,
    STATE_NOT_READY         = 1,
    STATE_FORWARD_DRIVE     = 2,
    STATE_NEUTRAL           = 3,
    STATE_REVERSE_DRIVE     = 4,
    STATE_REGEN             = 5,
    STATE_CRUISE_CONTROL    = 6,
    STATE_DISABLED          = 7,
    NUM_STATES
} FSMState_e;

// Bitfield Input Enum (unchanged, but organized)
typedef enum {
    INPUT_NEUTRAL           = 0,
    INPUT_FORWARD           = (1 << 0),
    INPUT_REVERSE           = (1 << 1),
    INPUT_NOT_READY         = (1 << 2),
    INPUT_CRUISE_BTN = 0x04,
    INPUT_REGEN_BTN = 0x08,
    INPUT_READY_REGEN = 0x10,
    INPUT_REGEN_EN = 0x20,
    INPUT_BRAKING = 0x40,
} InputBits_e;

typedef struct {
    SemaphoreHandle_t mutex;
    
    // Inputs from CAN (protected by mutex)
    float brake_pedal_percent;
    float accel_pedal_percent;
    uint8_t gear;
    bool regen_btn_pressed;
    bool cruise_ctrl_btn;
    bool regen_enabled;
    bool ok_to_regen;
    bool bps_tripped;
    uint8_t ignition_state;
    
    // Outputs (protected by mutex)
    float velocity_setpoint;
    float current_setpoint;
    
    // State tracking
    FSMState_e current_state;
    uint8_t current_bitfield;
    bool brake_on;
    int brake_threshold;
    bool accelerator_reset;
    
    // Timing for hysteresis
    TickType_t last_brake_change;
    
} FSMData_t;

// **State handler function pointer type**
typedef void (*StateHandler_t)(FSMData_t *fsm_data, uint8_t bitfield);

// **FSM State definition**
typedef struct {
    FSMState_e state_name;
    StateHandler_t handler;
    // Transition matrix: NextStates[bitfield] = next_state
    FSMState_e next_states[128];
} FSMStateDef_t;

// Public API
void vcu_fsm_init(FSMData_t *fsm);
void vcu_fsm_run(FSMData_t *fsm);
void vcu_fsm_set_inputs(FSMData_t *fsm, const FSMData_t *inputs_snapshot);
FSMData_t* vcu_fsm_get_outputs(FSMData_t *fsm);
void vcu_fsm_disable(FSMData_t *fsm);
void vcu_fsm_recover(FSMData_t *fsm);

#endif
