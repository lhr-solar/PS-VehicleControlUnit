#include "MotorTelemetryTask.h"

#define MOTOR_TELEMETRY_QUEUE_SIZE 32
#define SLCAN_MAX_FRAME_LEN 32

static StaticQueue_t motorTelemetryQueueBuffer;
static uint8_t motorTelemetryQueueStorage[MOTOR_TELEMETRY_QUEUE_SIZE * sizeof(can_rx_payload_t)];
static QueueHandle_t motorTelemetryQueue;
static volatile uint32_t motorTelemetryDroppedFrames = 0;
static volatile uint32_t esp32SendErrors = 0;

void print_slcan(const can_rx_payload_t payload) {
    uint32_t id = payload.header.Identifier;
    uint8_t len = (payload.header.DataLength);

    /* SLCAN supports max 8 bytes */
    if (len > 8) len = 8;

    if (payload.header.IdType == FDCAN_STANDARD_ID) {
        /* tIII DLC DATA... */
        printf("t%03lX%1X", id & 0x7FF, len);
    } else {
        /* TIIIIIIII DLC DATA... */
        printf("T%08lX%1X", id & 0x1FFFFFFF, len);
    }

    for (uint8_t i = 0; i < len; i++) {
        printf("%02X", payload.data[i]);
    }

    printf("\r\n");
}

void MotorTelemetryTask_Init(void) {
    if (motorTelemetryQueue != NULL) {
        return;
    }
    motorTelemetryQueue =
        xQueueCreateStatic(MOTOR_TELEMETRY_QUEUE_SIZE, sizeof(can_rx_payload_t),
                           motorTelemetryQueueStorage, &motorTelemetryQueueBuffer);
}

void can_fd_rx_callback_hook(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs, can_rx_payload_t recv_payload ){
    (void)RxFifo0ITs;

    if (hfdcan == NULL || motorfdcan == NULL) {
        return;
    }

    // only forward motorCAN messages to CarCAN
    if (hfdcan->Instance == motorfdcan->Instance) {

        BaseType_t higherPriorityTaskWoken = pdFALSE;

        if (motorTelemetryQueue == NULL) {
            motorTelemetryDroppedFrames++;
        } else {
            const BaseType_t queued = xQueueSendFromISR(
                motorTelemetryQueue,
                &recv_payload,
                &higherPriorityTaskWoken
            );
            if (queued != pdTRUE) {
                motorTelemetryDroppedFrames++;
            }
        }
        // don't yield at the end of this since the rest of the ISR needs to run

        
        FDCAN_TxHeaderTypeDef tx_header = {0};   
        tx_header.Identifier = recv_payload.header.Identifier;
        tx_header.IdType = FDCAN_STANDARD_ID;
        tx_header.TxFrameType = FDCAN_DATA_FRAME;
        tx_header.DataLength = FDCAN_DLC_BYTES(CAN_DLC_VCU_STATUS);
        tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
        tx_header.BitRateSwitch = FDCAN_BRS_OFF;
        tx_header.FDFormat = FDCAN_CLASSIC_CAN;
        tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        tx_header.MessageMarker = 0;

        can_fd_send_isr(motorfdcan, &tx_header, recv_payload.data, &higherPriorityTaskWoken);
    }
}


void Task_MotorTelemetry(void *args) {
    (void)args;

    /* Queue must already exist (InitTask calls MotorTelemetryTask_Init before MotorCAN_Init). */
    if (motorTelemetryQueue == NULL) {
        MotorTelemetryTask_Init();
    }
    if (motorTelemetryQueue == NULL) {
        Error_Handler();
    }

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

    while (1) {
        if (xQueueReceive(motorTelemetryQueue, &payload, portMAX_DELAY) == pdTRUE) {

            // copy the incoming message's ID and data length
            carCanTransmitHeader.Identifier = payload.header.Identifier;
            carCanTransmitHeader.DataLength = payload.header.DataLength;
            // forward the motorCAN message to CarCAN
            CarCAN_Send(&carCanTransmitHeader, payload.data, portMAX_DELAY);

            // print the incoming message over
            print_slcan(payload);

            // forward SLCAN to ESP32 for WiFi broadcast bridge
            if (payload.header.IdType == FDCAN_STANDARD_ID) {
                char slcanBuf[SLCAN_MAX_FRAME_LEN];
                uint8_t dlc = (uint8_t)payload.header.DataLength;
                if (dlc > 8U) {
                    dlc = 8U;
                }
                const int slen = can_to_slcan((uint16_t)(payload.header.Identifier & 0x7FFU),
                                              payload.data,
                                              dlc,
                                              slcanBuf,
                                              sizeof(slcanBuf));
                if (slen > 0) {
                    if (ESP32_Send((const uint8_t *)slcanBuf, (uint8_t)slen, 0) != UART_OK) {
                        esp32SendErrors++;
                    }
                } else {
                    esp32SendErrors++;
                }
            }
            taskYIELD();
        }
    }
}