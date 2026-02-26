#include "DriveMotor.h"
#include <stdint.h>
#include "stdbool.h"
#include "semphr.h"
#include "faults.h"
#include "SendAndRecieveCarStatus.h"

SemaphoreHandle_t vcu_status_lock;
SemaphoreHandle_t controls_lock;


// CAN MSG VARIABLES
static gear_t gear = DASH_NEU;
static bool regenButtonPressed = false;
static bool cruiseControlSet = false;
static bool cruiseControlEnabled = false;
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
    configASSERT(carStatusEventGroup != NULL);
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
    isBraking = true;
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

  xSemaphoreTake(controls_lock, portMAX_DELAY);
  for (int i = 0; i < FSM_SIGNAL_COUNT; i++) {
      switch (i) {  // use enum index directly
        case BPS_STATUS: {
            struct filtered_bps_status_t bpsData;
            bps_status_unpack(&bpsData, dataBuf, FILTERED_BPS_STATUS_LENGTH);

            okToRegen = bpsData.bps_regen_ok;

            xSemaphoreTake(vcu_status_lock, portMAX_DELAY);
            vcu_status.vcu_regen_ok = okToRegen;
            xSemaphoreGive(vcu_status_lock);

            bpsTripped = (bpsData.bps_fault != 0);

            if (bpsTripped) {
                Faults_ThrowFault(FAULT_ID_GENERIC_CUZ_IM_LAZY);
            }

            break;
        }

        case ACCEL_BRAKE_POS: {
            struct filtered_accel_brake_position_t pedalsData;
            accel_brake_position_unpack(
                &pedalsData,
                dataBuf,
                FILTERED_ACCEL_BRAKE_POSITION_LENGTH
            );

            accelPedalPercent = pedalsData.accel_pos_main_fault
                ? pedalsData.accel_pos_redundant
                : pedalsData.accel_pos_main;

            brakePedalPercent = pedalsData.brake_pos_redundant_fault
                ? pedalsData.brake_pos_redundant
                : pedalsData.brake_pos_main;

            if ((pedalsData.brake_pressure_1_fault &&
                pedalsData.brake_pressure_2_fault) ||
                (pedalsData.accel_pos_main_fault &&
                pedalsData.accel_pos_redundant_fault)) {

                Faults_ThrowFault(FAULT_ID_GENERIC_CUZ_IM_LAZY);
            }

            break;
        }

        case DRIVER_INPUT_STATUS: {
            struct filtered_driver_input_status_t driverData;
            driver_input_status_unpack(
                &driverData,
                dataBuf,
                FILTERED_DRIVER_INPUT_STATUS_LENGTH
            );

            regenButtonPressed   = driverData.regen_activate && driverData.regen_enable; //combining these
            cruiseControlSet  = driverData.cruise_set;
            cruiseControlEnabled = driverData.cruise_enable;

            // Gear mapping
            if (driverData.gear_forward) {
                gear = DASH_FWD;
            } else if (driverData.gear_reverse) {
                gear = DASH_REV;
            } else if (driverData.gear_neutral) {
                gear = DASH_NEU;
            } else {
                gear = DASH_NEU; // Default/fallback
            }

            // Ignition state
            if (driverData.ignition_array) {
                ignitionState = ARR_EN;
            } else if (driverData.ignition_motor) {
                ignitionState = MOT_EN
            } else if (driverData.ignition_off) {
                ignitionState = IGN_OFF;
            } else {
                ignitionState = IGN_OFF; // Default/fallback
            }

            break;
        }

    default:
        break;
      }

    }
  xSemaphoreGive(controls_lock);
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

void Task_SendVCUStatus(void *p_arg) {
    while(true) {
        //send vcu status every 100ms or so, this is separate from the update task since this is not needed for the logic and can be slower
        //also we want to make sure this goes out on time for telemetry reasons
        uint8_t data[8] = {0};
        xSemaphoreTake(vcu_status_lock, portMAX_DELAY);
        vcu_status.vcu_driver_input_ok = 1;
        vcu_status.vcu_pedals_ok = 1;
        filtered_vcu_status_pack(data, &vcu_status, FILTERED_VCU_STATUS_LENGTH);
        xSemaphoreGive(vcu_status_lock);
        car_can_send(data, FILTERED_VCU_STATUS_FRAME_ID);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}



