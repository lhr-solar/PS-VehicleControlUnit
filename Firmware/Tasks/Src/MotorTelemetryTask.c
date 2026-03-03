#include "MotorTelemetryTask.h"

#define MOTOR_TELEMETRY_QUEUE_SIZE    32

static StaticQueue_t motorTelemetryQueueBuffer;
static uint8_t motorTelemetryQueueStorage[MOTOR_TELEMETRY_QUEUE_SIZE * sizeof(can_rx_payload_t)];
static QueueHandle_t motorTelemetryQueue;

// enables the fdcan3 recieve hook, calls can_fd_rx_callback_hook everytime a can rx interrupt happens
#define FDCAN3_RECV_HOOK_EN



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

    BaseType_t higherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(
        motorTelemetryQueue,
        &recv_payload,
        &higherPriorityTaskWoken
    );
    HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
}

void Task_MotorTelemetry(){

    // motor canbus should be initialized here

    printf("balls\r\n");

    can_rx_payload_t payload;

    while(1){
        if (xQueueReceive(motorTelemetryQueue, &payload, portMAX_DELAY) == pdTRUE){
            print_slcan(payload);
        }
    }
}