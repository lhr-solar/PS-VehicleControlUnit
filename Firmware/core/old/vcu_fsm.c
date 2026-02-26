#include "vcu_fsm.h"
#include "../Inc/vcu_errors.h"
#include "math.h"

// Constants
#define BRAKE_THRESH 42
#define BRAKE_THRESH_HYST 30
#define ACCEL_PEDAL_THRESHOLD 5
#define MAX_VELOCITY 100.0f
#define REGEN_CURRENT 100.0f
#define CRUISE_VELOCITY 80.0f

// Forward declarations for state handlers
static void handle_init_state(FSMData_t *fsm, uint8_t bitfield);
static void handle_not_ready_state(FSMData_t *fsm, uint8_t bitfield);
static void handle_forward_drive_state(FSMData_t *fsm, uint8_t bitfield);
static void handle_neutral_state(FSMData_t *fsm, uint8_t bitfield);
static void handle_reverse_drive_state(FSMData_t *fsm, uint8_t bitfield);
static void handle_regen_state(FSMData_t *fsm, uint8_t bitfield);
static void handle_cruise_control_state(FSMData_t *fsm, uint8_t bitfield);
static void handle_disabled_state(FSMData_t *fsm, uint8_t bitfield);

// FSM definition
static FSMStateDef_t fsm_states[NUM_STATES] = {
    {STATE_INIT, handle_init_state, {0}},
    {STATE_NOT_READY, handle_not_ready_state, {0}},
    {STATE_FORWARD_DRIVE, handle_forward_drive_state, {0}},
    {STATE_NEUTRAL, handle_neutral_state, {0}},
    {STATE_REVERSE_DRIVE, handle_reverse_drive_state, {0}},
    {STATE_REGEN, handle_regen_state, {0}},
    {STATE_CRUISE_CONTROL, handle_cruise_control_state, {0}},
    {STATE_DISABLED, handle_disabled_state, {0}},
};

// Build the transition matrix (moved to separate function for clarity)
static void build_transition_matrix(void) {
    for (int state = 0; state < NUM_STATES; state++) {
        for (int bitfield = 0; bitfield < 128; bitfield++) {
            
            // CAR_NOT_READY state - stay in not ready
            if (state == STATE_NOT_READY) {
                fsm_states[state].next_states[bitfield] = STATE_NOT_READY;
                continue;
            }
            
            // DISABLED state - stay disabled
            if (state == STATE_DISABLED) {
                fsm_states[state].next_states[bitfield] = STATE_DISABLED;
                continue;
            }
            
            // Extract key bits for decision logic
            uint8_t gear_bits = bitfield & 0x03;
            bool is_braking = bitfield & INPUT_BRAKING;
            bool regen_enabled = bitfield & INPUT_REGEN_EN;
            bool ready_regen = bitfield & INPUT_READY_REGEN;
            bool regen_btn = bitfield & INPUT_REGEN_BTN;
            bool cruise_btn = bitfield & INPUT_CRUISE_BTN;
            
            // Decision tree (cleaner than nested ifs)
            FSMState_e next_state = STATE_NEUTRAL;  // Default
            
            if (is_braking) {
                // Braking logic
                if (regen_enabled && ready_regen) {
                    next_state = STATE_REGEN;
                } else {
                    next_state = STATE_NEUTRAL;
                }
            } else if (regen_enabled && ready_regen && regen_btn && !cruise_btn) {
                // Manual regen (button press)
                next_state = STATE_REGEN;
            } else if (cruise_btn && (gear_bits == INPUT_FORWARD) && regen_enabled) {
                // Cruise control
                next_state = STATE_CRUISE_CONTROL;
            } else if (gear_bits == INPUT_FORWARD) {
                // Forward drive
                if (state == STATE_REVERSE_DRIVE) {
                    // Rev to fwd transition: go through neutral first
                    next_state = STATE_NEUTRAL;
                } else {
                    next_state = STATE_FORWARD_DRIVE;
                }
            } else if (gear_bits == INPUT_REVERSE) {
                // Reverse drive
                if (state == STATE_FORWARD_DRIVE) {
                    // Fwd to rev transition: go through neutral first
                    next_state = STATE_NEUTRAL;
                } else {
                    next_state = STATE_REVERSE_DRIVE;
                }
            } else if (gear_bits == INPUT_NOT_READY) {
                // Not ready bits set
                next_state = STATE_NOT_READY;
            } else {
                // Default: neutral
                next_state = STATE_NEUTRAL;
            }
            
            fsm_states[state].next_states[bitfield] = next_state;
        }
    }
}

// **Key improvement: Separate bitfield generation from CAN I/O**
static uint8_t generate_bitfield(const FSMData_t *fsm) {
    uint8_t bitfield = 0;
    
    // Gear state bits
    if (fsm->gear == 1) {  // DASH_FWD
        bitfield |= INPUT_FORWARD;
    } else if (fsm->gear == 2) {  // DASH_REV
        bitfield |= INPUT_REVERSE;
    } else if (fsm->gear == 0) {  // DASH_NEU
        bitfield |= INPUT_NEUTRAL;
    } else {  // DASH_INIT or unknown
        bitfield |= INPUT_NOT_READY;
    }
    
    // Brake threshold with hysteresis
    if (fsm->brake_pedal_percent >= fsm->brake_threshold) {
        bitfield |= INPUT_BRAKING;
        fsm->brake_on = true;
        fsm->brake_threshold = BRAKE_THRESH_HYST;  // Lower threshold when braking
    } else {
        fsm->brake_on = false;
        fsm->brake_threshold = BRAKE_THRESH;  // Restore normal threshold
    }
    
    // Button and mode bits
    if (fsm->regen_btn_pressed) bitfield |= INPUT_REGEN_BTN;
    if (fsm->regen_enabled) bitfield |= INPUT_REGEN_EN;
    if (fsm->ok_to_regen) bitfield |= INPUT_READY_REGEN;
    if (fsm->cruise_ctrl_btn) bitfield |= INPUT_CRUISE_BTN;
    
    return bitfield;
}

// **Check if FSM is in a state that allows motor commands**
static bool is_fsm_ready_for_commands(FSMState_e state) {
    return (state != STATE_INIT && 
            state != STATE_DISABLED && 
            state != STATE_NOT_READY);
}

// **State handlers - cleaner and more readable**

static void handle_init_state(FSMData_t *fsm, uint8_t bitfield) {
    // Initialization - CAN bus setup happens in task init, not here
    // Just transition to not_ready
    fsm->current_state = STATE_NOT_READY;
}

static void handle_not_ready_state(FSMData_t *fsm, uint8_t bitfield) {
    // Car is not ready - zero out commands
    fsm->velocity_setpoint = 0.0f;
    fsm->current_setpoint = 0.0f;
    
    // Check if we can transition to ready
    bool ready = (fsm->accel_pedal_percent < ACCEL_PEDAL_THRESHOLD) && 
                 !fsm->bps_tripped &&
                 (fsm->ignition_state > 0);  // Ignition on
    
    if (ready) {
        fsm->current_state = STATE_NEUTRAL;
    }
}

static void handle_forward_drive_state(FSMData_t *fsm, uint8_t bitfield) {
    fsm->velocity_setpoint = MAX_VELOCITY;
    fsm->current_setpoint = fsm->accel_pedal_percent;  // 0-100%
}

static void handle_neutral_state(FSMData_t *fsm, uint8_t bitfield) {
    fsm->velocity_setpoint = 0.0f;
    fsm->current_setpoint = 0.0f;
}

static void handle_reverse_drive_state(FSMData_t *fsm, uint8_t bitfield) {
    fsm->velocity_setpoint = MAX_VELOCITY;
    fsm->current_setpoint = -fsm->accel_pedal_percent;  // Negative for reverse
}

static void handle_regen_state(FSMData_t *fsm, uint8_t bitfield) {
    fsm->velocity_setpoint = 0.0f;
    fsm->current_setpoint = REGEN_CURRENT;  // Positive current = regen
}

static void handle_cruise_control_state(FSMData_t *fsm, uint8_t bitfield) {
    // Cruise control maintains set velocity
    fsm->velocity_setpoint = CRUISE_VELOCITY;  // Should be set when button first pressed
    fsm->current_setpoint = 0.0f;  // Let speed controller handle it
}

static void handle_disabled_state(FSMData_t *fsm, uint8_t bitfield) {
    fsm->velocity_setpoint = 0.0f;
    fsm->current_setpoint = 0.0f;
}

// **Public API Implementation**

void vcu_fsm_init(FSMData_t *fsm) {
    fsm->mutex = xSemaphoreCreateMutex();
    fsm->current_state = STATE_INIT;
    fsm->brake_threshold = BRAKE_THRESH;
    fsm->current_bitfield = 0;
    fsm->last_brake_change = xTaskGetTickCount();
    
    // Build transition matrix once at startup
    build_transition_matrix();
}

void vcu_fsm_run(FSMData_t *fsm) {
    // **CRITICAL: Take mutex to safely read all FSM inputs**
    if (xSemaphoreTake(fsm->mutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        // Timeout - previous task still has lock, skip this cycle
        return;
    }
    
    // Generate bitfield from current inputs
    uint8_t new_bitfield = generate_bitfield(fsm);
    fsm->current_bitfield = new_bitfield;
    
    // Get next state from transition matrix
    FSMState_e next_state = fsm_states[fsm->current_state].next_states[new_bitfield];
    fsm->current_state = next_state;
    
    // Only run state handler if ready for commands
    if (is_fsm_ready_for_commands(fsm->current_state)) {
        fsm_states[fsm->current_state].handler(fsm, new_bitfield);
    } else {
        // In init/disabled/not_ready - zero commands
        fsm->velocity_setpoint = 0.0f;
        fsm->current_setpoint = 0.0f;
    }
    
    xSemaphoreGive(fsm->mutex);
}

// **Safe way to update inputs from CAN task**
void vcu_fsm_set_inputs(FSMData_t *fsm, const FSMData_t *inputs_snapshot) {
    if (xSemaphoreTake(fsm->mutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        return;  // Couldn't acquire - skip this update
    }
    
    // Atomically update all CAN inputs
    fsm->brake_pedal_percent = inputs_snapshot->brake_pedal_percent;
    fsm->accel_pedal_percent = inputs_snapshot->accel_pedal_percent;
    fsm->gear = inputs_snapshot->gear;
    fsm->regen_btn_pressed = inputs_snapshot->regen_btn_pressed;
    fsm->cruise_ctrl_btn = inputs_snapshot->cruise_ctrl_btn;
    fsm->regen_enabled = inputs_snapshot->regen_enabled;
    fsm->ok_to_regen = inputs_snapshot->ok_to_regen;
    fsm->bps_tripped = inputs_snapshot->bps_tripped;
    fsm->ignition_state = inputs_snapshot->ignition_state;
    
    xSemaphoreGive(fsm->mutex);
}

// **Safe way to get motor commands from TX task**
FSMData_t* vcu_fsm_get_outputs(FSMData_t *fsm) {
    static FSMData_t output_snapshot;
    
    if (xSemaphoreTake(fsm->mutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        // Return last known state if mutex timeout
        return &output_snapshot;
    }
    
    output_snapshot.velocity_setpoint = fsm->velocity_setpoint;
    output_snapshot.current_setpoint = fsm->current_setpoint;
    output_snapshot.current_state = fsm->current_state;
    
    xSemaphoreGive(fsm->mutex);
    return &output_snapshot;
}

void vcu_fsm_disable(FSMData_t *fsm) {
    if (xSemaphoreTake(fsm->mutex, pdMS_TO_TICKS(5)) != pdTRUE) return;
    fsm->current_state = STATE_DISABLED;
    fsm->velocity_setpoint = 0.0f;
    fsm->current_setpoint = 0.0f;
    xSemaphoreGive(fsm->mutex);
}

void vcu_fsm_recover(FSMData_t *fsm) {
    if (xSemaphoreTake(fsm->mutex, pdMS_TO_TICKS(5)) != pdTRUE) return;
    fsm->current_state = STATE_NOT_READY;
    fsm->velocity_setpoint = 0.0f;
    fsm->current_setpoint = 0.0f;
    xSemaphoreGive(fsm->mutex);
}
