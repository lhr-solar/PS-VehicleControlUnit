#include "ReadMotorCAN.h"

void Init_ReadMotorCAN(){

}

void Task_ReadMotorCAN(){

    // Motor CANbus must be initialized by now

    Init_ReadMotorCAN();

    FDCAN_RxHeaderTypeDef motorstatus_rx_header = {0};
    uint8_t motorstatus_rx_data[8] = {0};

    uint8_t can_recv_errors = 0;

    while(1){
        

        if(MotorCAN_Recv(CAN_ID_MC_STATUS, &motorstatus_rx_header, motorstatus_rx_data, portMAX_DELAY) == CAN_OK){
            can_recv_errors = 0;
        }else{
            can_recv_errors++;
        }


    }
}
