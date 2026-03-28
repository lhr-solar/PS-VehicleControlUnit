#include "VCUSendVoltageTask.h"

static FDCAN_TxHeaderTypeDef VCUSendVoltageHeader;

static void initVCUSendVoltageHeader(FDCAN_TxHeaderTypeDef *tx_header);

void VCUSendVoltageTask_Init()
{
    initVCUSendVoltageHeader(&VCUSendVoltageHeader);
}

// helper function to inialize motor drive command headers
static void initVCUSendVoltageHeader(FDCAN_TxHeaderTypeDef *tx_header)
{
    tx_header->Identifier = CAN_ID_VCU_PRECHARGE_VOLTAGES;
    tx_header->IdType = FDCAN_STANDARD_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->DataLength = FDCAN_DLC_BYTES_8;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header->MessageMarker = 0;
}

// encodes battery and motor voltage into an array of bytes for can_send
static void packBatteryVoltage(vcu_precharge_voltages_t voltages, uint8_t tx_data[8])
{
    memcpy(&tx_data[0], &(voltages.Precharge_Battery_Voltage), sizeof(uint32_t)); // TODO: Have Precharge task push adc readings into the can messages struct
    memcpy(&tx_data[4], &(voltages.Precharge_Motor_Voltage), sizeof(uint32_t));
}

void Task_VCUSendVoltage()
{
    uint8_t VCU_tx_data[8];
    vcu_precharge_voltages_t precharge_voltages = {0};

    uint8_t can_send_errors = 0;
    // uint8_t print_debug_counter = 0;

    while (1)
    {
        precharge_voltages.Precharge_Battery_Voltage = Battery_Voltage;
        precharge_voltages.Precharge_Motor_Voltage = Motor_Voltage;
        packBatteryVoltage(precharge_voltages, VCU_tx_data);

        if (Car_CANBus_Send(&VCUSendVoltageHeader, VCU_tx_data, portMAX_DELAY) == CAN_ERR)
        {
            can_send_errors++;
        }
        else
        {
            can_send_errors = 0;
        }

        vTaskDelay(1000);
    }
}