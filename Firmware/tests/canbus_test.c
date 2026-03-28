#include "CANbus.h"
#include "StatusLEDs.h"
#include "inits.h"
#include "pinDefs.h"
#include "stm32xx_hal.h"

StaticTask_t task_buffer;
StackType_t task_stack[512];

#define TEST_CAN_ID 0x321
#define TEST_CAN_DATA {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xFF}
#define TEST_CAN_DATA_LENGTH 8

static void task(void *pvParameters) {
    FDCAN_TxHeaderTypeDef tx_header = {0};
    tx_header.Identifier = TEST_CAN_ID;
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header.MessageMarker = 0;

    uint8_t tx_data[TEST_CAN_DATA_LENGTH] = TEST_CAN_DATA;
    while (1) {
        if (MotorCAN_Send(&tx_header, tx_data, portMAX_DELAY) != CAN_OK) {
            Error_Handler();
        }
        
        HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void can_error_handler() {
    while (1) {
        LED_set(MOTOR_FAULT, LED_ON);
    }
}

int main() {
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LED_init();

    if (MotorCAN_Init() != CAN_OK) {
        can_error_handler();
    }

    xTaskCreateStatic(task, "task", 512, NULL, tskIDLE_PRIORITY + 2, task_stack, &task_buffer);

    vTaskStartScheduler();

    while (1) {
    }
}