#include "DriveMotor.h"
#include <stdint.h>
#include "stdbool.h"



// CAN MSG VARIABLES
static gear_t gear = DASH_NEU;
static bool regenButtonPressed = false;
static bool cruiseControlButton = false;
static bool regenEnabled = false;
static bool okToRegen = false;
static bool bpsTripped = false;
static float brakePedalPercent = 0.0f;
static float accelPedalPercent = 0.0f;

static ignitionState_t ignitionState = IGN_OFF;

static float thresholdBrake = BRAKE_THRESH;
static bool isBraking = false; //used for exiting cruise control/other things (could/should be in bitfield but will do later)

//add a start up sequence bool + a fault recovery status bool later


void initStatusEventGroup() {
    carStatusEventGroup = xEventGroupCreate();
    configASSERT(carStatusEventGroup != NULL); //making sure it's created
}

void getControlsBitfield() {
  // Return the current state of the Car, this is through CAN commands
  uint8_t status = 0;

  // Update the IO state forceably, will change this later...
  getAndUpdateControlStatus();

  // Figuring out all the bits for the bitfield, this is only the logic, we have
  // already gotten the data, i just need to update from the variables I have
  // now

  //Gear state bits

  if (gear == DASH_FWD) {
    status |= FORWARD_BIT;
  } else if (gear == DASH_REV) {
    status |= REVERSE_BIT;
  } else if (gear == DASH_NEU) {
    status |= NEUTRAL_BIT;
  }

  // If braking, thresh for braking goes down (hysterisis)
  if (brakePedalPercent >= thresholdBrake) {
    thresholdBrake = BRAKE_THRESH_HYST;
  } else {
    thresholdBrake = BRAKE_THRESH;
  }    

  //Buttons
    status |= regenButtonPressed ? REGEN_BUTTON_BIT : 0;
    status |= regenEnabled ? REGEN_ENABLED_BIT : 0;
    status |= okToRegen ? READY_TO_REGEN_BIT : 0;
    status |= cruiseControlButton ? CRUISE_CONTROL_BUTTON_BIT : 0;

    xEventGroupClearBits(carStatusEventGroup, ALL_STATUS_BITS);
    xEventGroupSetBits(carStatusEventGroup, status);
}


// For CAN Architecture, all of the decoding is speculative, need to see how the
// packaging is done
void getAndUpdateControlStatus() {  // This is for getting the data, the logic will
                            // happen seperately
  // Reading all of the relevant data on the BUS
  // updateFSMCANMessages();

  // Have a list of IDs and update the associated one? Needs timestamp as well
  // for watchdogs, have a thread calling this constantly

  for (int i = 0; i < FSM_SIGNAL_COUNT; i++) {

    uint8_t dataBuf[8] = {0};
    can_status_t status = car_can_read(dataBuf, (FSM_Signal_t)i);
    if(status == CAN_ERR) {
      //Throw some error, will do this later
    }else if(status == CAN_EMPTY) {
      //No new data, skip
      continue;
    }

    //signal has data, decode based on ID and update event here for watchdogs
    //I have the actual DBC now so let's parse this fr fr
    recieved_CAN_message((FSM_Signal_t)i); //for wd

      switch (fsm_signal_to_can_id[i]) {
        case CAN_ID_PEDALS:
          // Byte 0 = brake raw, Byte 1 = accel raw
          brakePedalPercent = dataBuf[0];
          accelPedalPercent = dataBuf[1];

          brakePedalPercent = brakePedalPercent;
          accelPedalPercent = accelPedalPercent;
          
          break;
        case CAN_ID_GEARS:
          // Assume gear encoded in byte 0
          gear = dataBuf[0] & 0x03;
          break;
        case CAN_ID_REGEN_BUTTON:
          // Assume byte 0 bit0 = regen button pressed
          regenButtonPressed =
              (dataBuf[0] & 0x01);
          break;
        case CAN_ID_CRUISE_CONTROL:
          // Assume byte 0 bit0 = cruise button toggle
          cruiseControlButton =
              (dataBuf[0] & 0x01);
          break;
        case CAN_ID_REGEN_ENABLED:
            // Assume byte 0 bit0 = regen enabled
            regenEnabled = (dataBuf[0] & 0x01);
            break;
        case CAN_ID_BPS_OK_TO_REGEN:
          // Assume byte 0 bit0 = OK-to-regen flag
          okToRegen = (dataBuf[0] & 0x01);
          break;

        case CAN_ID_BPS_TRIP:
          // Assume byte 0 bit0 = BPS tripped
          bpsTripped = (dataBuf[0] & 0x01);
          break;

        case CAN_ID_IGNITION_STATE:
          // Assume byte 0 = ignition mode enum
          ignitionState = dataBuf[0];
          break;

        default:
          // Future CAN IDs handled here
          break;
      }
  }
}



//A little abstraction for readability
can_status_t car_can_read(uint8_t *data, FSM_Signal_t id) {
  // Fill the data based on the ID, call the acc receive function here
  // Header can be null, we got the ID and doing nun complex w rtr/ids/etc.
  CAN_RxHeaderTypeDef header;
  return can_recv(hcan2, fsm_signal_to_can_id[id], &header, data, 0);
}

void Task_UpdateControlStatus(void *p_arg) {
    while(true) {
        getControlsBitfield();
        vTaskDelay(pdMS_TO_TICKS(50)); //update every 50ms
    }
}



