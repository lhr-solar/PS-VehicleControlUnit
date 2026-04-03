#include "VCUSendStatusTask.h"

static FDCAN_TxHeaderTypeDef VCUSendStatusHeader;

static void initVCUSendStatusHeader(FDCAN_TxHeaderTypeDef *tx_header);

void VCUSendStatusTask_Init()
{
    initVCUSendStatusHeader(&VCUSendStatusHeader);
}

static void initVCUSendStatusHeader(FDCAN_TxHeaderTypeDef *tx_header)
{
    tx_header->Identifier = CAN_ID_VCU_STATUS;
    tx_header->IdType = FDCAN_STANDARD_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->DataLength = FDCAN_DLC_BYTES_8;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header->MessageMarker = 0;
}

static uint8_t convertFaultBitsToIndex(uint32_t faults)
{
    for (int i = 0; i < FAULT_NUM; i++)
    {
        if (faults & (1U << i))
        {
            return i + 1; // fault codes start at 1, 0 = no fault
        }
    }

    return 0; // no fault active
}

// encodes VCU status struct into an array of bytes for can_send
static void packVCUStatus(vcu_status_t status, uint8_t tx_data[8])
{
    uint32_t faults = fault_bits;
    uint8_t fault_code = convertFaultBitsToIndex(faults);
    tx_data[0] = fault_code;

    uint8_t motor_contactor_state = (state_bits >> 1) & 0x1;
    uint8_t precharge_contactor_state = (state_bits >> 2) & 0x1;
    uint8_t ready_to_drive = (state_bits >> 3) & 0x1;

    tx_data[1] = motor_contactor_state;     // Motor_Contactor_State
    tx_data[2] = precharge_contactor_state; // Motor_Precharge_Contactor_State
    tx_data[3] = ready_to_drive;            // Motor_Ready_To_Drive
    tx_data[4] = 0;                         // VCU_Driver_Input_OK
    tx_data[5] = 0;                         // VCU_Pedals_OK
    tx_data[6] = 0;                         // VCU_Regen_OK
    tx_data[7] = 0;                         // VCU_Regen_Active / FSM_State
}

void Task_VCUSendStatus()
{
    uint8_t VCU_tx_data[8];
    vcu_status_t VCUStatus = {0};

    uint8_t can_send_errors = 0;
    // uint8_t print_debug_counter = 0;

    while (1)
    {
        packVCUStatus(VCUStatus, VCU_tx_data);

        if (Car_CANBus_Send(&VCUSendStatusHeader, VCU_tx_data, portMAX_DELAY) == CAN_ERR)
        {
            can_send_errors++;
        }
        else
        {
            can_send_errors = 0;
        }

        vTaskDelay(100);
    }
}