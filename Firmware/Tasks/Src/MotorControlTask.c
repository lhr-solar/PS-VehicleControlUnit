#include "MotorControlTask.h"
#include "CAN_FD.h"

// Period the motor control thread runs at
#define MOTOR_CONTROL_TASK_PERIOD_MS 100

#define PRINT_DEBUG_PERIOD 1000

// number of times the thread will run before we print out debug info
#define PRINT_DEBUG_COUNT (PRINT_DEBUG_PERIOD/MOTOR_CONTROL_TASK_PERIOD_MS)


static FDCAN_TxHeaderTypeDef mocoDriveCommandHeader;

static void initDriveCommandHeader(FDCAN_TxHeaderTypeDef *tx_header);
static void initMotorPowerCommandHeader(FDCAN_TxHeaderTypeDef *tx_header);


void MotorControlTask_Init(void){
    // set necessary motor drive command parameters
    initDriveCommandHeader(&mocoDriveCommandHeader);
}

static void initMotorPowerCommandHeader(FDCAN_TxHeaderTypeDef *tx_header){

    tx_header->Identifier = CAN_ID_MC_POWERCOMMAND;
    tx_header->IdType = FDCAN_STANDARD_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->DataLength = FDCAN_DLC_BYTES_8;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header->MessageMarker = 0;
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

static void packPowerCommand(mc_powercommand_t motorPowerCommand, uint8_t tx_data[8]){
     memcpy(&tx_data[4], &(motorPowerCommand.MC_MotorPowerSetpoint), sizeof(float));
}

static float busCurrentSetPoint = 1.0f;

void Task_MotorControl(void){

    // motor canbus MUST be initalized by now
    // the motor safe bits MUST be initalized by now
    MotorControlTask_Init();

    // current and velocity setpoint control speed of motor
    mc_drivecommand_t motorDriveCommand = {0};
    motorDriveCommand.MC_MotorCurrentSetpoint = 0.0f;
    motorDriveCommand.MC_MotorVelocitySetpoint = 0.0f;
    uint8_t motor_drive_tx_data[8];

    uint8_t can_send_errors = 0;
    uint8_t print_debug_counter = 0;

    // sets the max power of the motor
    mc_powercommand_t motorPowerCommand = {0};
    motorPowerCommand.MC_MotorPowerSetpoint = busCurrentSetPoint;
    FDCAN_TxHeaderTypeDef mocoPowerCommandHeader;
    uint8_t motor_power_tx_data[8]; // the message being sent on the CANbus
    initMotorPowerCommandHeader(&mocoPowerCommandHeader); // initializes the can tx header
    packPowerCommand(motorPowerCommand, motor_power_tx_data); // packs the motorPower struct into an array of bytes

    EventBits_t motorSafeBits;
    while(1){

        // Send power command
        Motor_CANBus_Send(&mocoPowerCommandHeader, motor_power_tx_data, portMAX_DELAY);

        // check if the correct motor safe bits are set
        motorSafeBits = MotoSafeBits_WaitForSafe(pdMS_TO_TICKS(0));

        // no bits are set, so the motor should not be run
        if(motorSafeBits == 0){
            motorDriveCommand.MC_MotorCurrentSetpoint = 0.0f;
            motorDriveCommand.MC_MotorVelocitySetpoint = 0.0f;
        }
        else{
            motorDriveCommand.MC_MotorCurrentSetpoint = 0.2f;
            motorDriveCommand.MC_MotorVelocitySetpoint = 12000.0f;
        }

        packDriveCommand(motorDriveCommand, motor_drive_tx_data);

        if (Motor_CANBus_Send(&mocoDriveCommandHeader, motor_drive_tx_data, portMAX_DELAY) == CAN_ERR){
            can_send_errors++;
        }
        else{
            can_send_errors = 0;
        }

        print_debug_counter++;
        if(print_debug_counter > PRINT_DEBUG_COUNT){
            printf("Motor Current Setpoint: %f\r\n", motorDriveCommand.MC_MotorCurrentSetpoint);
            printf("Motor Velocity Setpoint: %f\r\n", motorDriveCommand.MC_MotorVelocitySetpoint);
            printf("Drive command can send errors: %d\r\n", can_send_errors);
            print_debug_counter = 0;
        }


        // minimum delay for drive command is 250ms, or else the wavesculptor will reset to neutral
        vTaskDelay(pdMS_TO_TICKS(MOTOR_CONTROL_TASK_PERIOD_MS));

    }
}
