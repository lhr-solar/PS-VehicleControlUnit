#include "CanTxTelemetryTask.h"

#define CAN_TX_TELEMETRY_QUEUE_SIZE    10   

static StaticQueue_t canTxTelemetryQueueBuffer;
static uint8_t canTxTelemetryQueueStorage[CAN_TX_TELEMETRY_QUEUE_SIZE * sizeof(can_tx_payload_t)];
static QueueHandle_t canTxTelemetryQueue;


void can_tx_print_slcan(const can_tx_payload_t payload)
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

void CanTxTelemetryTask_Init(void){
    canTxTelemetryQueue = xQueueCreateStatic(
        CAN_TX_TELEMETRY_QUEUE_SIZE,
        sizeof(can_tx_payload_t),
        canTxTelemetryQueueStorage,
        &canTxTelemetryQueueBuffer
    );

    if(canTxTelemetryQueue == NULL){
        return;
    }
}

void can_fd_tx_callback_hook(FDCAN_HandleTypeDef* hfdcan, const can_tx_payload_t* payload){

    BaseType_t higherPriorityTaskWoken = pdFALSE;

    if(canTxTelemetryQueue != NULL){
        xQueueSendFromISR(
            canTxTelemetryQueue,
            payload,
            &higherPriorityTaskWoken
        );

    }
    // don't yield at the end of this since the rest of the ISR needs to run
}


void Task_CanTxTelemetry(){

    // car canbus MUST be initialized by now
    CanTxTelemetryTask_Init();

    can_tx_payload_t payload;

    while(1){
        // forward all transmitted can messages to USB
        if (xQueueReceive(canTxTelemetryQueue, &payload, portMAX_DELAY) == pdTRUE){
            // TODO: use the embedded-sharepoint slcan formatter
            can_tx_print_slcan(payload);
            // TODO: should also forward data to ESP32
        }
    }
}