/**
 * @copyright Copyright (c) 2018-2023 UT Longhorn Racing Solar
 * @file SendTritium.c
 * @brief Function implementations for the SendTritium application minimum
 * viable product with cruise control abilities.
 *
 * This contains functions relevant to updating the velocity and current
 * setpoints of the Tritium motor controller. The implementation includes a
 * normal current controlled mode and a cruise control mode, but doesn't include
 * a one pedal mode/regen braking capabilities. The logic is determined through
 * a finite state machine implementation.
 *
 * If the macro SENDTRITIUM_EXPOSE_VARS is defined prior to including
 * SendTritium.h, relevant setters will be exposed as externs for unit testing
 * and hardware inputs won't be read and motor commands won't be sent over
 * MotorCAN. If the macro SENDTRITIUM_PRINT_MES is also defined prior to
 * including SendTritium.h, debug info will be printed via UART.
 */

#include <stdio.h>
#include "CAN.h"
// #include "CANConfig.h"
// #include "CANbus.h"
// #include "Dashboard.h"
// #include "DebugIO.h"
// #include "Lights.h"
// #include "Pedals.h"
// #include "ReadTritium.h"
// #include "SendCarCAN.h"
// #include "StatusLeds.h"
// #include "Tasks.h"
// #include "UpdateDisplay.h"
// #include "os_cfg_app.h"
#include "ReceiveMotor.h"
#include "DriveMotor.h"

#include "stdbool.h"

// #define USING_PROFINITY

#include <math.h>
#include "FSM.h"

// #define NEXT_STATES_LENGTH 128

// #define REGEN_STEP 0.02f
// #define CAN_MSG_BUFFER_SIZE 1

// CURRENT FSM STATE
TritiumState_t currentState;

typedef struct FSMCANDATA {  // May want to add more later..
  CANDATA_t CANMessage;
  uint64_t timestamp;
} FSMCANDATA_t;

static float busCurrentSetPoint =
    1.0f;  // This gets manipulated if the battery not ok


// // Gear fault counter
// static uint8_t gearFaultCnt = 0;

static bool isBrakeOn = false;  // Used for updating display & brakelight
static uint8_t carStatus = 0;  // Bitfield for car status
bool accelerator_reset = false;
float cruiseControlPercent = -1;


// CAN messages
// static FSMCANDATA_t CAR_MSGS[NUM_FSM_CANFilters] = {0};

CANDATA_t driveCmd = {.can_id = 0x1234, .idx = 0, .data = {0.0f, 0.0f}};

CANDATA_t powerCmd = {.can_id = 0x4321, .idx = 0, .data = {0.0f, 0.0f}};


// NOTE: Instead of a "velocityObserved" variable, we can just use
// Motor_Velocity_Get() from ReadTritium when doing cruise logic

// // Getter functions for local variables in SendTritium.c
// GETTER(uint8_t, brakePedalPercent)
// GETTER(uint8_t, accelPedalPercent)
// GETTER(gear_t, gear)
// GETTER(float, currentSetpoint)
// GETTER(float, velocitySetpoint)
// GETTER(bool, isBrakeOn)

// Create the FSM data strcuture, make all the possible states, to do this
// prolly use
//  an enum for every state and have a decision handler for each, logic will
//  be embedded within this, alos we would have flags represented by bits for
//  whatever we care about

// Todo: Make an RTOS Task for this one with watchdogs for BPS, Pedals, and
// gear, make a display function,  have another task searching for BPS and
// updating state, and update regen not ready, and then finally display all the
// faults.. Questions: Do i need to make an rtos task for all this? Am i just
// replacing read/send tritium or also read car can (ignition + IO stuff
// tracking + faults anyways)? Can i throw a flag into not ready for BPS stuff?
void runFSM() {                         // a bitfield
    currentState =
        FSM[currentState.NextStates[carStatus]];  // Get the next state based on
                                                  // the current state and car
                                                  // status
    currentState.stateHandler();  // Run the handler for the current state
    vcu_status.vcu_fsm_state = currentState.stateName; //update the vcu status for telemetry
}

void init() {
  //set each handler individually here... will do later
FSM[STATE_INIT].stateHandler = &handleFSMInitState;
FSM[CAR_NOT_READY].stateHandler = &handleFSMNotReadyState;
FSM[FORWARD_DRIVE].stateHandler = &handleFSMForwardDriveState;
FSM[NEUTRAL].stateHandler = &handleFSMNeutralState;
FSM[REVERSE_DRIVE].stateHandler = &handleFSMReverseDriveState;
FSM[REGEN].stateHandler = &handleFSMRegenState;
FSM[CRUISE_CONTROL].stateHandler = &handleFSMCruiseControlState;
FSM[DISABLED].stateHandler = &handleFSMDisabledState;

  currentState = FSM[STATE_INIT];  // Start in init state
  currentState.stateHandler();     // Run init handler
}


//These are not packaging right atm (should do everything in floats)
void sendMotorDriveCommand(float velocitySetpoint, float currentSetpoint) {
  memset(&driveCmd.data[0], 0, sizeof(driveCmd.data));  // Clear it

  memcpy(&driveCmd.data[0], &velocitySetpoint,
         sizeof(float));  // Set velocity setpoint
  memcpy(&driveCmd.data[4], &currentSetpoint,
         sizeof(float));  // Set current setpoint

  // SendCarCAN_Put(
  //     driveCmd);  // Send the drive command to the car CAN bus for telemetry
  mmotor_can_send(driveCmd.data, DRIVER_CONTROLS_BASE_ADDR + 0x1); //can hardcode this for now
}

void sendMotorPowerCommand(float powerSetpoint) {
  memset(&powerCmd.data[0], 0, sizeof(powerCmd.data));  //change later

  memcpy(&powerCmd.data[0], &powerSetpoint,
         sizeof(float));  // Set power setpoint

  motor_can_send(powerCmd.data, DRIVER_CONTROLS_BASE_ADDR + 0x2); //can hardcode this for now change later
}

//MUST REINTERPRET and not just cast
float getCarSpeed(){
  int idx = -1;
  for(int i = 0; i < MOCO_FULL_STATUS_ARR_LEN; i++){
    if(moco_full_status_arr[i] == MOTOR_VELOCITY){
      idx = i;
      break;
    }
  }

  uint64_t packedData = moco_full_status_arr[idx];
  uint32_t rawSpeed = (uint32_t)(packedData >> 32);
  //convert to float
  float convertedSpeed;
  memcpy(&convertedSpeed, &rawSpeed, sizeof(float));
  return convertedSpeed;
}


bool readyToRoll() {  // Car can escape not ready state, this is all of our
                      // checks for recoverable faults and init states, but init
                      // stats will have associated flags

  // Check all of our status bits here for other random faults
  if(getCarSpeed() < 1.0f && ignitionState == MOT_EN && !isBraking && accelPedalPercent == 0.0f){
    return true;
  }

  return false;  //must be stationary to exit not ready
}

//It should just lock itself in this state until reset or recovery
void disableFSM(){
  currentState = FSM[DISABLED];
}

//recover FSM state
void recoverFSM(){
  currentState = FSM[CAR_NOT_READY];
}


// Methods for handling all the FSM states

void handleFSMNotReadyState() {
  // Starting up, should so something here but not sure, over here make sure
  // ignition good prolly and go back to neutral if motor ok
  handleFSMNeutralState();
  currentState = readyToRoll() ? FSM[NEUTRAL] : FSM[CAR_NOT_READY];
  return;
}

void handleFSMForwardDriveState() {
  cruiseControlPercent = -1; //reset cruise control when we go back into drive
  vcu_status.vcu_regen_active = false; //make sure regen is not active when we are trying to drive forward

  if(getCarSpeed() < -1.0f){ //dont go fwd in reverse
    currentState = FSM[NEUTRAL];
    return;
  }

  sendMotorDriveCommand(20000, accelPedalPercent);
  return;
}
void handleFSMNeutralState() {
  sendMotorDriveCommand(0, 0);
  //must route here until speed is under a certain thresh
  return;
}

void handleFSMReverseDriveState() {
  if(getCarSpeed() > 1.0f){ //dont go rev while in fwd
    currentState = FSM[NEUTRAL];
    return;
  }

  sendMotorDriveCommand(-20000, accelPedalPercent);
  return;
}

void handleFSMRegenState() {
  float regenCurrent = 100.0f;
  sendMotorDriveCommand(0, regenCurrent);
  vcu_status.vcu_regen_active = true;
  return;
}

void handleFSMCruiseControlState() {
  if(cruiseControlPercent == -1){
    cruiseControlPercent = getCarSpeed(); //set this to current speed when we first enter cruise control, will update later with buttons and stuff
  }

  sendMotorDriveCommand(20000, cruiseControlPercent);
  return;
}

void handleFSMDisabledState() {
  handleFSMNeutralState();  // Just stop motor and current and throw/handle
                            // faults
  // OS_FLAGS flags = MotorStatus_GetBits();  // Do something with this later...
  return;
}

void handleFSMInitState() {

  can_init(hcan1, NULL);
  can_init(hcan2, NULL);

  can_start(hcan1);
  can_start(hcan2);

  //initialize timers here, could make a timer/wd object in the header and just pass that in as well, might do when i have time
  CAN_MSG_Watchdog_Create("BPSWD",           BPS_STATUS,          100); //100ms timeout
  CAN_MSG_Watchdog_Create("VCUWD",           VCU_STATUS,          100); //100ms timeout
  CAN_MSG_Watchdog_Create("AccelBrakeWD",    ACCEL_BRAKE_POS,     100); //100ms timeout
  CAN_MSG_Watchdog_Create("DriverInputWD",   DRIVER_INPUT_STATUS, 100); //100ms timeout

  initFSM();

  currentState = FSM[CAR_NOT_READY];  // Go to not ready after init, CAN needs time + startup sequence
  return;
}


uint8_t generateCustomBitfield(BitfieldInputs_t inputs[]) {
  // Inputs are in order of most significant bit to least significant bit
  uint8_t bitfield = 0;
  for (int i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
    bitfield |= inputs[i];
  }
  return bitfield;
}

can_status_t motor_can_send(uint8_t *data, int motor_can_id) {
  // Send the data based on the ID, call the acc send function here
  //Change this later to motor enum and hash...

  CAN_TxHeaderTypeDef header = {
    .StdId = motor_can_id,
    .IDE = CAN_ID_STD, 
    .RTR = CAN_RTR_DATA,
    .DLC = 8
};

  return can_send(hcan1, &header, data, 0);
}

/**
 * @brief Linearly map range of integers to another range of integers, and
 * provide the pecentage result. in_min to in_max is mapped to out_min to
 * out_max.
 * @param input input integer value
 * @param in_min minimum value of input range
 * @param in_max maximum value of input range
 * @param out_min minimum value of output range
 * @param out_max maximum value of output range
 * @returns float value from (out_min / 100.0) to (out_max / 100.0)
 */
float mapToPercent(uint8_t input, uint8_t in_min, uint8_t in_max,
                   uint8_t out_min, uint8_t out_max) {
  // The minimum of the input range should never be greater than the maximum of
  // the input range
  if (in_min >= in_max) {
    in_max = in_min;
  }

  // Lower bound the input to the minimum possible output
  if (input <= in_min) {
    return out_min / 100.0;
  } else if (input >= in_max) {
    // Upper bound the input to the maximum output
    return out_max / 100.0;
  } else {
    // Linear mapping between ranges
    uint8_t offset_in =
        input - in_min;  // If input went from A -> B, it now goes from 0 -> B-A
    uint8_t in_range = in_max - in_min;     // Input range
    uint8_t out_range = out_max - out_min;  // Output range
    uint8_t offset_out = out_min;
    // slope = out_range/in_range. y=mx+b so output=slope*offset_in+offset_out
    return ((offset_in * out_range) / in_range + offset_out) / 100.0;
  }
}

#define SWOC_LIMIT

typedef struct {
  float speed_mph;
  uint8_t max_percent;
} swoc_threshold_t;

static const swoc_threshold_t swoc_thresholds[] = {{10.0f, 80}, {17.0f, 75},
                                                   {20.0f, 70}, {23.0f, 60},
                                                   {25.0f, 50}, {28.5f, 45}};

static uint8_t getSpeedDependentPower(float speed_mph) {
  uint8_t cap = CURRENT_SP_MAX;
  size_t i;
  for (i = 0; i < (sizeof(swoc_thresholds) / sizeof(swoc_thresholds[0])); ++i) {
    if (speed_mph >= swoc_thresholds[i].speed_mph) {
      cap = swoc_thresholds[i].max_percent;
    }
  }
  return cap;
}

// Task (main loop)

/**
 * @brief Follows the FSM to update the velocity of the car
 */
void Task_SendMotor(void *p_arg) {

  // By default assume we are below the motor swoc threshold at startup
  // MotorStatus_ModifyBits(MOTOR_SWOC_THRESHOLD, true, false);

  carStatus = xEventGroupGetBits(carStatusEventGroup); // Get the current status of the car as a bitfield
  runFSM();  // Run periodically

    // err = MotorStatus_Wait(MOTOR_SWOC_THRESHOLD, !OS_FLAG_BLOCKING);
    // maxCurrentPercentage =
    //     (err == OS_ERR_NONE) ? SWOC_CURRENT_SP_MAX : CURRENT_SP_MAX;

}

// static void assertSendTritiumError(controls_error_e sterr) {
//   throwTaskError(sterr);
// }

// Lin map, display, task finish and verify, fault check from helper, do some
// SWOC logic in fault thread, watch dogs... make faults tuff and unify all of
// em

// Lock in twin, copy paste send tritium and see how they send tasks, make the
// switch from micrium to free rtos, maybe switch up faults again, use their
// methods for everyting

// Trim, integrate w freertos to start w, add the tasks and handle faults
// accordingly...

// Im sort of making the whole architecture for this, so how should it be done?
// I think we have a faults.c for all faults and have an os pend fault task get
// called if any fault is called... Now, for the motor, what lets the FSM be
// enabled oh wait thats the timer task, put the faults at higher priority so
// any fault found will go to faulted state To check for faults, have a bitfield
// for everything, and all each method to set the fault bits In not ready have
// all my checks in the FSM to make sure the motor is ok to run, and then go to
// next, throw faults if anything out of order (have this happen later at the
// when i have time), EVERYTHING through CAN, so that's first order of
// business...

//ngl for the faults, i might throw it into disabled state, itll work, i can assert some os error later

//CAN integration, make the init + start now, make this an RTOS task called every 50ms or so, make a watchdog for certain CAN messages, prolly abstract to a method, handle faults before ready to roll, otherwise disabled, and god knows what i do with display think it should be handled by someone else...
//Make a main for this now and attach can macros at main init? ,Make rtos task be called, then fault handle, then watch dogs lowk

//Add a fault state now to activate once wd fails (put in disabled, fault task run indefinitely, print context), figure out the ready to roll 

//Add the RTOS tasks in the main, add the motor can support (Also add to the can 1 recv), make the task handler fr, check if it compiles, and add SWOC + other faults

//Get motor bits from the tritium status, then also extract things like speed / motor params for state logic

//Add other status signals for reading the car can, figure out logic for ready to roll, make sure faults are thrown maybe, do rest of list...

//FSM logic fix.. thinking bitmask for each state, then big if-else, ignored bits should be baked in... done for now
//switching from reverse to forward done
//figure out ignition states done
//Look into defining watchdogs for each msg + adding it to the faults bitfield later, have logic to differentiate in the ISR for now so will keep that...
//make sure everything is clean and makes sense... bro there's so so many random things that pmo, like its not clean at all, headers call each other + god knows if i need a chip to make it compile + it's all guesswork

//FINAL PUSHHH
//Make sure my CAN parsing makes sense for packaging -> made cantools do it so im green here
//switch to real CAN msgs... done!? Idk but lets see
//make an init task rq... 
//add mutexes for shared vars / threads
//have claude check it all since gemini keeps frying me and I miss things
//make compile...
//forwarding motor messages is a later issue...