#include "MotorTelemetryTask.h"

#define MOTOR_TELEMETRY_QUEUE_SIZE    32

static StaticQueue_t motorTelemetryQueueBuffer;
static uint8_t motorTelemetryQueueStorage[MOTOR_TELEMETRY_QUEUE_SIZE * sizeof(can_rx_payload_t)];
static QueueHandle_t motorTelemetryQueue;

// enables the fdcan3 recieve hook, calls can_fd_rx_callback_hook everytime a can rx interrupt happens
#define FDCAN3_RECV_HOOK_EN

// bus current for power
// static float busCurrentSetPoint = 1.0f;

void print_slcan(const can_rx_payload_t payload)
{

    uint32_t id  = payload.header.Identifier;
    uint8_t  len = (payload.header.DataLength);

    /* SLCAN supports max 8 bytes */
    if (len > 8)
        len = 8;

    if (payload.header.IdType == FDCAN_STANDARD_ID)
    {
        /* tIII DLC DATA... */
        printf("t%03lX%1X", id & 0x7FF, len);
    }
    else
    {
        /* TIIIIIIII DLC DATA... */
        printf("T%08lX%1X", id & 0x1FFFFFFF, len);
    }

    for (uint8_t i = 0; i < len; i++)
    {
        printf("%02X", payload.data[i]);
    }

    printf("\r\n");
}

void MotorTelemetryTask_Init(void){
    motorTelemetryQueue = xQueueCreateStatic(
        MOTOR_TELEMETRY_QUEUE_SIZE,
        sizeof(can_rx_payload_t),
        motorTelemetryQueueStorage,
        &motorTelemetryQueueBuffer
    );

    if(motorTelemetryQueue == NULL){
        return;
    }
}
void can_fd_rx_callback_hook(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs, can_rx_payload_t recv_payload ){
    
    // only forward motorCAN messages to CarCAN
    if(hfdcan->Instance == motorfdcan->Instance){

        BaseType_t higherPriorityTaskWoken = pdFALSE;

        xQueueSendFromISR(
            motorTelemetryQueue,
            &recv_payload,
            &higherPriorityTaskWoken
        );
    }
}

void Task_MotorTelemetry(){

    // motor canbus MUST be initialized by now
    MotorTelemetryTask_Init();

    can_rx_payload_t payload;

    // This is the header for data we're forwarding from motor to carCAN
    // need to set DataLength and ID
    FDCAN_TxHeaderTypeDef carCanTransmitHeader;
    carCanTransmitHeader.IdType = FDCAN_STANDARD_ID;
    carCanTransmitHeader.TxFrameType = FDCAN_DATA_FRAME;
    carCanTransmitHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    carCanTransmitHeader.BitRateSwitch = FDCAN_BRS_OFF;
    carCanTransmitHeader.FDFormat = FDCAN_CLASSIC_CAN;
    carCanTransmitHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    carCanTransmitHeader.MessageMarker = 0;

    while(1){
        if (xQueueReceive(motorTelemetryQueue, &payload, portMAX_DELAY) == pdTRUE){

            // copy the incoming message's ID and data length
            carCanTransmitHeader.Identifier = payload.header.Identifier;
            carCanTransmitHeader.DataLength = payload.header.DataLength;
            // forward the motorCAN message to CarCAN
            Car_CANBus_Send(&carCanTransmitHeader, payload.data, portMAX_DELAY);

            // print the incoming message over 
            print_slcan(payload);
        }
    }
}