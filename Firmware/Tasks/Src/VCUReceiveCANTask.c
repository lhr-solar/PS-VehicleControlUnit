#include "VCUReceiveCANTask.h"

static StaticQueue_t driverInputQueueBuffer;
static uint8_t driverInputQueueStorage[DRIVER_INPUT_QUEUE_SIZE * sizeof(can_rx_payload_t)];
static QueueHandle_t driverInputQueue;

static StaticQueue_t BPSQueueBuffer;
static uint8_t BPSQueueStorage[BPS_QUEUE_SIZE * sizeof(can_rx_payload_t)];
static QueueHandle_t BPSQueue;

can_rx_payload_t payload;
uint8_t Start_Precharge = 0;
uint8_t End_Precharge = 0;

static void initDriverInputQueue()
{
    driverInputQueue = xQueueCreateStatic(
        DRIVER_INPUT_QUEUE_SIZE,
        sizeof(can_rx_payload_t),
        driverInputQueueStorage,
        &driverInputQueueBuffer);

    if (driverInputQueue == NULL)
    {
        return;
    }
}

static void initBPSQueue()
{
    BPSQueue = xQueueCreateStatic(
        BPS_QUEUE_SIZE,
        sizeof(can_rx_payload_t),
        BPSQueueStorage,
        &BPSQueueBuffer);

    if (BPSQueue == NULL)
    {
        return;
    }
}

void Init_VCUReceiveCANTask()
{
    initDriverInputQueue();
    initBPSQueue();
}

void can_fd_rx_callback_hook(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs, can_rx_payload_t recv_payload)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    if (recv_payload.header.Identifier == CAN_ID_DRIVER_INPUT_STATUS)
    {
        if (driverInputQueue != NULL)
        {
            xQueueSendFromISR(driverInputQueue, &recv_payload, &higherPriorityTaskWoken);
        }
    }
    else if (recv_payload.header.Identifier == CAN_ID_BPS_STATUS)
    {
        if (BPSQueue != NULL)
        {
            xQueueSendFromISR(BPSQueue, &recv_payload, &higherPriorityTaskWoken);
        }
    }

    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void Task_VCUReceiveCAN()
{
    Init_VCUReceiveCANTask();

    while(1)
    {
        if (xQueueReceive(driverInputQueue, &payload, pdMS_TO_TICKS(CAN_BLOCKING_TIME_MS)) == pdTRUE)
        {
            if (payload.header.Identifier == CAN_ID_DRIVER_INPUT_STATUS && payload.data[IGNITION_MOTOR_INDEX] == 1 && Ignition_State == 0)
            {
                Start_Precharge = 1;
            }
            else if (payload.header.Identifier == CAN_ID_DRIVER_INPUT_STATUS && payload.data[IGNITION_MOTOR_INDEX] == 0 && Ignition_State == 1)
            {
                // offTick = xTaskGetTickCount();
                End_Precharge = 1; // Turning off ignition
            }
        }

        if (xQueueReceive(BPSQueue, &payload, pdMS_TO_TICKS(CAN_BLOCKING_TIME_MS)) == pdTRUE)
        {
            if(payload.header.Identifier == CAN_ID_BPS_STATUS && payload.data[0] != BPS_STATUS_BPS_FAULT_OK)
            {
                set_faultBit(BPS_FAULT);
            }
        }
    }
}