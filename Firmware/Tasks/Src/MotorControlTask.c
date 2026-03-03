#include "MotorControlTask.h"
#include "CAN_FD.h"


static FDCAN_TxHeaderTypeDef mocoDriveCommandHeader;

static void initDriveCommandHeader(FDCAN_TxHeaderTypeDef *tx_header);


void MotorControlTask_Init(void){

    // set necessary motor drive command parameters
    initDriveCommandHeader(&mocoDriveCommandHeader);

}


// helper function to inialize motor drive command headers
static void initDriveCommandHeader(FDCAN_TxHeaderTypeDef *tx_header){

    tx_header->Identifier = CAN_ID_MC_DRIVECOMMAND;
    tx_header->IdType = FDCAN_STANDARD_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->DataLength = FDCAN_DLC_BYTES_8;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header->MessageMarker = 0;
}

// encodes a drive command struct into an array of bytes for can_send
static void packDriveCommand(mc_drivecommand_t motorDriveCommand, uint8_t tx_data[8]){
    memcpy(&tx_data[4], &(motorDriveCommand.MC_MotorCurrentSetpoint), sizeof(float));
    memcpy(&tx_data[0], &(motorDriveCommand.MC_MotorVelocitySetpoint), sizeof(float));
}

void Task_MotorControl(void){

    // motor canbus should be initalized by now

    mc_drivecommand_t motorDriveCommand = {0};
    motorDriveCommand.MC_MotorCurrentSetpoint = 0.1f;
    motorDriveCommand.MC_MotorVelocitySetpoint = 0.1f;
    uint8_t motor_drive_tx_data[8];

    uint8_t can_send_errors = 0;

    while(1){

        packDriveCommand(motorDriveCommand, motor_drive_tx_data);


        if (Motor_CANBus_Send(&mocoDriveCommandHeader, motor_drive_tx_data, portMAX_DELAY) == CAN_ERR){
            can_send_errors++;
        }
        else{
            can_send_errors = 0;
        }

        printf("Motor Current Setpoint: %f\r\n", motorDriveCommand.MC_MotorCurrentSetpoint);
        printf("Motor Velocity Setpoint: %f\r\n", motorDriveCommand.MC_MotorVelocitySetpoint);

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}
