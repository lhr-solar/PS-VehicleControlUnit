#include "DriverInputTask.h"

#define DRIVER_INPUT_TEST_TASK_STACK configMINIMAL_STACK_SIZE
#define DRIVER_INPUT_TEST_TASK_PRIO  (tskIDLE_PRIORITY + 1)

static void initDriverInputHeader(FDCAN_TxHeaderTypeDef *tx_header)
{
    tx_header->Identifier = CAN_ID_DRIVER_INPUT_STATUS;
    tx_header->IdType = FDCAN_STANDARD_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->DataLength = FDCAN_DLC_BYTES_8;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header->MessageMarker = 0;
}

void Task_DriverInputTest(void *pvParameters)
{
    FDCAN_TxHeaderTypeDef header;
    uint8_t tx_data[8] = {0};

    initDriverInputHeader(&header);

    vTaskDelay(pdMS_TO_TICKS(2000));   // let everything initialize first

    tx_data[IGNITION_MOTOR_INDEX] = 1;

    if (Car_CANBus_Send(&header, tx_data, portMAX_DELAY) == CAN_OK)
    {
        printf("TEST: Sent ignition ON frame\r\n");
    }
    else
    {
        printf("TEST: Failed to send ignition ON frame\r\n");
    }

    vTaskDelete(NULL);
}