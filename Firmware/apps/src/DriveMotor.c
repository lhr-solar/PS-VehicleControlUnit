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

// Outputs
// static float currentSetpoint = 0.0f;
// static float velocitySetpoint = 0.0f;
int thresholdBrake = BRAKE_THRESH;

// CAN messages
// static FSMCANDATA_t CAR_MSGS[NUM_FSM_CANFilters] = {0};

CANDATA_t driveCmd = {.ID = MOTOR_DRIVE, .idx = 0, .data = {0.0f, 0.0f}};

CANDATA_t powerCmd = {.ID = MOTOR_POWER, .idx = 0, .data = {0.0f, 0.0f}};

CANDATA_t motorSafeCmd = {.ID = MOTOR_CONTROLLER_SAFE, .idx = 0, .data = {0}};

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
}

void init() {
  assert(FSM_SIGNAL_COUNT != SEND_TRITIUM_IDS); //ensuring signal for every ID
  currentState = FSM[STATE_INIT];  // Start in init state
  currentState.stateHandler();     // Run init handler
}

void initFSM() {
  // The states in the for loop may be overwritten later, they are just a base

  // for (int i = 0; i < NUM_STATES; i++) {
  //   for (int j = 0; j < NEXT_STATES_LENGTH - 1; j++) {
  //     if (i == CAR_NOT_READY) {
  //       FSM[i].NextStates[j] =
  //           CAR_NOT_READY;  // If we are not ready, stay in this state
  //       continue;
  //     }

  //     if (i == DISABLED) {
  //       FSM[i].NextStates[j] =
  //           DISABLED;  // If we are disabled, stay in this state
  //       continue;
  //     }

  //     if (j & BRAKING_BIT) {
  //       // Braking, -> check regen if ok do that, if not neutral
  //       if ((j & REGEN_ENABLED_BIT) &&
  //           (j & READY_TO_REGEN_BIT)) {  // Checking if regen enabled AND ready
  //                                        // to regen
  //         FSM[i].NextStates[j] = REGEN;  // Blended braking
  //       } else {
  //         FSM[i].NextStates[j] = FSM[i].stateName; //next is same as before
  //       }
  //     } else if ((j & FORWARD_BIT) && (j & REGEN_ENABLED_BIT) && (j & READY_TO_REGEN_BIT) &&
  //                (j & REGEN_BUTTON_BIT) &&
  //                !(j & CRUISE_CONTROL_BUTTON_BIT)) {  // Checking if forward AND regen enabled
  //                                                 // AND regen button AND ready
  //                                                 // to regen AND NOT cruising
  //       // Regen
  //       FSM[i].NextStates[j] = REGEN;
  //     } else if ((j & CRUISE_CONTROL_BUTTON_BIT) && ((j & FORWARD_BIT) == 1) &&
  //                (j & REGEN_ENABLED_BIT)) {  // If we tryna cruise control AND
  //                                            // forward AND Regen ok
  //       // Cruise control
  //       FSM[i].NextStates[j] = CRUISE_CONTROL;
  //     } else if ((j & 0x3) == FORWARD_BIT) {  // If we are tryna go forward
  //       // Forward drive, check if we are going from rev to forward as well
  //       if (i == REVERSE_DRIVE) {
  //         // If we are going from rev to forward, go to neutral first
  //         FSM[i].NextStates[j] = NEUTRAL;
  //       } else {
  //         FSM[i].NextStates[j] = FORWARD_DRIVE;
  //       }
  //     } else if ((j & 0x3) == REVERSE_BIT) {  // If we are tryna go reverse
  //       // Reverse drive, check if we are going from forward to rev as well
  //       if (i == FORWARD_DRIVE) {
  //         // If we are going from forward to rev, go to neutral first
  //         FSM[i].NextStates[j] = NEUTRAL;
  //       } else {
  //         FSM[i].NextStates[j] = REVERSE_DRIVE;
  //       }
  //     } else if ((j & 0x3) == NOT_READY_BITS) {
  //       // Not ready
  //       FSM[i].NextStates[j] = CAR_NOT_READY;
  //     } else {
  //       // Neutral
  //       FSM[i].NextStates[j] = NEUTRAL;
  //     }
  //   }
  // }

    for (int i = 0; i < NUM_STATES; i++) {
      for (int j = 0; j < NEXT_STATES_LENGTH; j++) {
        FSM[i].NextStates[j] = NEUTRAL; //default... if we get something that's a weird combo I go to neutral

        if (i == CAR_NOT_READY || i == STATE_INIT) {
          FSM[i].NextStates[j] = CAR_NOT_READY;  // If we are not ready, stay in this state
          continue;
        }

        if (i == DISABLED) {
          FSM[i].NextStates[j] = DISABLED;  // If we are disabled, stay in this state
          continue;
        }

        //this is all state specific now (using j) but must first get the bitifield for each state to get allowed/tracked inputs
        //ORDER MATTERS WITHIN EACH STATE  
        switch (i) {

          case NEUTRAL: {
            if ((j & FORWARD_BIT) && !(j & REVERSE_BIT)) {
              FSM[i].NextStates[j] = FORWARD_DRIVE;
            } 
            else if ((j & REVERSE_BIT)) {
              FSM[i].NextStates[j] = REVERSE_DRIVE;
            }
            break;
          }

          case FORWARD_DRIVE: {
            //do gear switching first
            if((j & REVERSE_BIT) || (j & NEUTRAL_BIT)){
                FSM[i].NextStates[j] = NEUTRAL;
            }
            // Regen button
            else if ((j & REGEN_ENABLED_BIT) &&
                    (j & READY_TO_REGEN_BIT) &&
                    (j & REGEN_BUTTON_BIT)) {
              FSM[i].NextStates[j] = REGEN;
            }
            // Cruise control
            else if ((j & CRUISE_CONTROL_BUTTON_BIT) &&
                    (j & REGEN_ENABLED_BIT)) {
              FSM[i].NextStates[j] = CRUISE_CONTROL;
            }else{
              FSM[i].NextStates[j] = FORWARD_DRIVE;
            }
            break;
          }

          case REVERSE_DRIVE: {
            // Stay in reverse
            if ((j & REVERSE_BIT) && !(j & FORWARD_BIT)) {
              FSM[i].NextStates[j] = REVERSE_DRIVE;
            }
            // Not reverse -> neutral
            else {
              FSM[i].NextStates[j] = NEUTRAL;
            }
            break;
          }

          case REGEN: {
            // Regen ends -> forward if still in forward gear
            if((j & REGEN_BUTTON_BIT) && (j & READY_TO_REGEN_BIT) && (j & REGEN_ENABLED_BIT) && (j & FORWARD_BIT)){
                FSM[i].NextStates[j] = REGEN;
            }
            else{
              FSM[i].NextStates[j] = FORWARD_DRIVE;
            }
            break;
          }

          case CRUISE_CONTROL: {
            // Cruise disabled -> back to forward
            if (!(j & CRUISE_CONTROL_BUTTON_BIT)) {
              FSM[i].NextStates[j] = FORWARD_DRIVE;
            }else{
              FSM[i].NextStates[j] = CRUISE_CONTROL;
            }
            break;
          }

          default:
            FSM[i].NextStates[j] = NEUTRAL;
            break;
        }

      }
    }
  

  //Rewriting the FSM generation logic... look at it from the point of each individual state

  // init -> notReady (forced)
  // not ready -> neu (forced)
  // NEUTRAL (based on the gear bit, but if rev -> fwd or fwd -> rev make sure speed goes below thresh)
  //   -> Rev
  //   -> Fwd
  // fwd (if regen conditions + button regen OR if cruise control button cruise control or neu if gear)
  //   -> Regen
  //   -> Neutral
  //   -> Cruise control
  // regen -> fwd (once regen stopped)
  // cruis control -> fwd (once disabled)
  // rev -> neutral (gear shift to neu or fw)

  // SPECIFIC STATE SETUPS (none atm)
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
  mmotor_can_send(driveCmd.data, MOTOR_DRIVE); //can hardcode this for now
}

void sendMotorPowerCommand(float powerSetpoint) {
  memset(&powerCmd.data[0], 0, sizeof(powerCmd.data));  //change later

  memcpy(&powerCmd.data[0], &powerSetpoint,
         sizeof(float));  // Set power setpoint

  motor_can_send(powerCmd.data, MOTOR_POWER); //can hardcode this for now change later
}

//MUST REINTERPRET and not just cast
float getCarSpeed(){
  uint64_t packedData = moco_full_status_arr[MOTOR_VELOCITY];
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

  return getCarSpeed() < 1.0f && ignitionState == MOTOR_EN; //must be stationary to exit not ready
}

// OS_ERR checkForAllFaults() {
//     //checking the bps trip and the ignition state for now
//     OS_ERR err = OS_ERR_NONE;
//     if(bpsTripped) {
//         err = BPS_FAULT;
//     }else if(ignitionState != MOTOR && currentState.stateName != STATE_NOT_READY) {
//         err = IGNITION_FAULT;
//     }
    
//   return err;
// }

// void checkWatchDogs() { OS_ERR err; }

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
  if(getCarSpeed() < -1.0f){ //dont go fwd in reverse
    currentState = FSM[NEUTRAL];
    return;
  }

  sendMotorDriveCommand(MAX_VELOCITY, accelPedalPercent);
  return;
}
void handleFSMNeutralState() {
  sendMotorDriveCommand(0, 0);
  //must wait here until speed is under a certain thresh
  return;
}

void handleFSMReverseDriveState() {
  if(getCarSpeed() > 11.0f){ //dont go rev while in fwd
    currentState = FSM[NEUTRAL];
    return;
  }

  sendMotorDriveCommand(MAX_VELOCITY, -accelPedalPercent);
  return;
}

void handleFSMRegenState() {
  float regenCurrent = 100.0f;
  sendMotorDriveCommand(0, regenCurrent);
  return;
}

void handleFSMCruiseControlState() {
  float cruiseControlPercent =
      accelPedalPercent;  // Set this to some actual velocity later, when button
                          // pressed
  sendMotorDriveCommand(MAX_VELOCITY, cruiseControlPercent);
  return;
}

void handleFSMDisabledState() {
  handleFSMNeutralState();  // Just stop motor and current and throw/handle
                            // faults
  // OS_FLAGS flags = MotorStatus_GetBits();  // Do something with this later...
  return;
}

void handleFSMInitState() {
  // Was going to init CANBUS but we shall see
  // Relavant CAN messages to read
  // carCANFilterList[0] = IO_STATE;
  //...Finish this in a bit

  // for (int i = 0; i < NUM_CARCAN_FILTERS; i++) {
  //   CAR_MSGS[i].ID =
  //       carCANFilterList[i];  // Shouldn't matter if all the IDs aren't filled
  // }

  can_init(hcan1, NULL);
  can_init(hcan2, NULL);

  can_start(hcan1);
  can_start(hcan2);

  //initialize timers here, could make a timer/wd object in the header and just pass that in as well, might do when i have time
  CAN_MSG_Watchdog_Create("PedalWD", FSM_PEDALS, 100); //100ms timeout
  CAN_MSG_Watchdog_Create("GearsWD", FSM_GEARS, 100); //100ms timeout
  CAN_MSG_Watchdog_Create("RegenBtnWD", FSM_REGEN_BUTTON, 100); //100ms timeout
  CAN_MSG_Watchdog_Create("RegenEnWD", FSM_REGEN_ENABLED, 100); //100ms timeout
  CAN_MSG_Watchdog_Create("CruiseWD", FSM_CRUISE_CONTROL, 100); //100ms timeout
  CAN_MSG_Watchdog_Create("BPSOkRegenWD", FSM_BPS_OK_TO_REGEN, 100); //100ms timeout
  CAN_MSG_Watchdog_Create("BPSTripWD", FSM_BPS_TRIP, 100); //100ms timeout
  CAN_MSG_Watchdog_Create("IgnitionWD", FSM_IGNITION_STATE, 100); //100ms timeout

  initFSM();

  currentState = FSM[CAR_NOT_READY];  // Go to not ready after init, CAN needs time + startup sequence
  return;
}


uint8_t generateCustomBitfield(BitfieldInputs_t[] inputs) {
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

void handleWatchdogFSMFault(){
    Event_bits_t bits = Faults_GetCurrentFaults();
    for(int i = 0; i < FSM_SIGNAL_COUNT; i++){
        if(bits & (1 << i)){
            printf("Watchdog timeout for FSM Signal %d (CAN Message %x). \n", i, fsm_signal_to_can_id[i]);
        }
    }
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
//figure out ignition states
//Look into defining watchdogs for each msg + adding it to the faults bitfield later alligator
//make sure everything is clean and makes sense...
//add mutexes for shared vars / threads
//make compile...