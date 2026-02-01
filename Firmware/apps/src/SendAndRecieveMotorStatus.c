//A little abstraction for readability
#include <stdint.h>
#include "CAN.h"
#include "ReceiveMotor.h"
#include "FreeRTOS.h"
#include <event_groups.h>
#include "faults.h"
// #include <stm32f4xx_hal_can.h>
// #include <stm32l4xx_hal_can.h>

#define UPDATE_RATE_MS 200 //ms
#define MOTOR_BROADCAST_ID_1 0xABCD
#define MOTOR_BROADCAST_ID_2 0xABCD

can_status_t motor_can_read(uint8_t *data, uint16_t can_id) {
  // Fill the data based on the ID, call the acc receive function here
  // Header can be null, we got the ID and doing nun complex w rtr/ids/etc.
  CAN_RxHeaderTypeDef header;
  return can_recv(hcan1, can_id, &header, data, 0);
}

void getMotorStatus() {
  // Read all motor status messages and update the bitfield
  uint8_t dataBuf[8] = {0};

    for (int i = 0; i < sizeof(poll_list)/sizeof(moco_status_t); i++) {
      can_status_t can_status = motor_can_read(dataBuf, MOCO_BASE_ADDR + poll_list[i]);
      if (can_status != CAN_RECV) {
          //some error, may want to handle somehow
          continue; // Skip processing this message on error
      }

      //Store packed data into full status array
      uint64_t packed_data = 0;
      for (int byte_index = 0; byte_index < 8; byte_index++) {
          packed_data |= ((uint64_t)dataBuf[byte_index] << (8 * byte_index));
      }
      moco_full_status_arr[i] = packed_data;
  }

  //check error flags and flip anything that's high + this is why motor fault enums have to be grouped
  EventBits_t motorErrorFlags = (EventBits_t)(((uint16_t)(moco_full_status_arr[MOTOR_ERROR_STATUS] >> 16)) & 0x01FF); //extracts bits 0-8 of third packet (error flags)
  Faults_ThrowFaultsUsingBitfield(motorErrorFlags << FAULT_ID_MOTOR_OVERSPEED);
}



/**
 * @brief Sends remote frame CAN messages to the motor controller to request status messages. For cases where we want the motor status more often than the broadcast time
 */

void nudgeMotorForStatus(){
    CAN_TxHeaderTypeDef header = {
    .StdId = motor_can_id,
    .IDE = CAN_ID_STD, 
    .RTR = CAN_RTR_REMOTE,
    .DLC = 8
    };

    //sending remote messages to the motor to get all the can messages that should fill the buffer in the CAN we have

    for(int i = 0; i < sizeof(poll_list)/sizeof(moco_status_t); i++){
        header.StdId = MOCO_BASE_ADDR + poll_list[i];
        can_send(hcan1, &header, NULL, 0); //null since its the remote frame
    }
}

void car_can_send(uint8_t *data, uint16_t can_id) {
  // Send the data based on the ID, call the acc send function here
  //Change this later to motor enum and hash...

  CAN_TxHeaderTypeDef header = {
    .StdId = can_id,
    .IDE = CAN_ID_STD, 
    .RTR = CAN_RTR_DATA,
    .DLC = 8
};

  can_send(hcan2, &header, data, 0);
}

void Task_BroadcastMotorStatus(void *p_arg) {
    // Broadcast the motor status to other systems as needed
    // This function can be expanded based on specific requirements
    // For example, sending status over CAN bus or updating shared memory

    while(true){
        if(UPDATE_RATE_MS < 200){
          nudgeMotorForStatus(); // Request updated status if needed
        }

        getMotorStatus();

        //package/send over CAR CAN 
        uint64_t motor_broadcast_data_packet_1[8] = {0};
        uint64_t motor_broadcast_data_packet_2[8] = {0};
        
        memcpy(&motor_broadcast_data_packet_1[0], &moco_full_status_arr[0], 8 * sizeof(uint64_t)); //first 8 messages
        memcpy(&motor_broadcast_data_packet_2[0], &moco_full_status_arr[8], 6 * sizeof(uint64_t)); //next 6 messages

        car_can_send((uint8_t*)motor_broadcast_data_packet_1, MOTOR_BROADCAST_ID_1);
        car_can_send((uint8_t*)motor_broadcast_data_packet_2, MOTOR_BROADCAST_ID_2);

        // make the task sleep for whatever update rate is
        vTaskDelay(pdMS_TO_TICKS(UPDATE_RATE_MS)); //update every UPDATE_RATE ms
    }

}

