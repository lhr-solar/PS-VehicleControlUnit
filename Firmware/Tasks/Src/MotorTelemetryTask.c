#include "MotorTelemetryTask.h"

#define MOTOR_TELEMETRY_QUEUE_SIZE 32

static StaticQueue_t motorTelemetryQueueBuffer;
static uint8_t motorTelemetryQueueStorage[MOTOR_TELEMETRY_QUEUE_SIZE * sizeof(can_rx_payload_t)];
static QueueHandle_t motorTelemetryQueue;

static uint32_t esp32SendErrors = 0;

void MotorTelemetryTask_Init(void) {
    motorTelemetryQueue =
        xQueueCreateStatic(MOTOR_TELEMETRY_QUEUE_SIZE, sizeof(can_rx_payload_t),
                           motorTelemetryQueueStorage, &motorTelemetryQueueBuffer);

    if (motorTelemetryQueue == NULL) {
        return;
    }
}

void can_fd_rx_callback_hook(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs, can_rx_payload_t recv_payload ){
    
    // only forward motorCAN messages to CarCAN
    if (motorfdcan != NULL && hfdcan->Instance == motorfdcan->Instance){

        BaseType_t higherPriorityTaskWoken = pdFALSE;

         if (motorTelemetryQueue != NULL) {
            xQueueSendFromISR(
                motorTelemetryQueue,
                &recv_payload,
                &higherPriorityTaskWoken
            );
        }
        // don't yield at the end of this since the rest of the ISR needs to run

        
        FDCAN_TxHeaderTypeDef tx_header = {0};   
        tx_header.Identifier = recv_payload.header.Identifier;
        tx_header.IdType = recv_payload.header.IdType;
        tx_header.TxFrameType = FDCAN_DATA_FRAME;
        tx_header.DataLength = recv_payload.header.DataLength;
        tx_header.ErrorStateIndicator = recv_payload.header.ErrorStateIndicator;
        tx_header.BitRateSwitch = recv_payload.header.BitRateSwitch;
        tx_header.FDFormat = recv_payload.header.FDFormat;
        tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        tx_header.MessageMarker = 0;

        can_fd_send_isr(carfdcan, &tx_header, recv_payload.data, &higherPriorityTaskWoken);
    }
}


void Task_MotorTelemetry() {

    // motor canbus MUST be initialized by now
    MotorTelemetryTask_Init();

    can_rx_payload_t payload;

    while (1) {
        if (xQueueReceive(motorTelemetryQueue, &payload, portMAX_DELAY) == pdTRUE) {

            if (payload.header.IdType == FDCAN_STANDARD_ID) {
                char slcanBuf[SLCAN_MAX_FRAME_LEN];
                uint8_t dlc = (uint8_t)payload.header.DataLength;

                // only supports standard length messages
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
                    else{
                        esp32SendErrors = 0;
                    }
                } 
                else {
                    esp32SendErrors++;
                }
            }
            taskYIELD();
        }
    }
}