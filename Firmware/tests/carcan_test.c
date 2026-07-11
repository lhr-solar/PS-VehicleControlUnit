/**
 * canbus test!!
 *
 * Sends and receives as 2 different tasks.
 * For rx: send data from candapter at ID 0x123 to see an led blink
 * For tx: use candapter to see if 123456789ABCDEFF is being received at 0x321
 *
 */

#include "CANbus.h"
#include "StatusLEDs.h"
#include "inits.h"
#include "pinDefs.h"
#include "stm32xx_hal.h"

#define TESTING_STACK_SIZE          (configMINIMAL_STACK_SIZE * 8)

StaticTask_t rx_task_buffer;
StackType_t rx_task_stack[TESTING_STACK_SIZE];

StaticTask_t tx_task_buffer;
StackType_t tx_task_stack[TESTING_STACK_SIZE];

#define TEST_CAN_ID_RX 0x123
#define TEST_CAN_ID_TX 0x321
#define TEST_CAN_DATA_TX {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xFF}
#define TEST_CAN_DATA_LENGTH 8

void can_error_handler() {
    while (1) {
        LED_set(MOTOR_FAULT, LED_ON);
    }
}

static void rx_task(void *pvParameters) {
    FDCAN_RxHeaderTypeDef rx_header = {0};
    uint8_t rx_data[TEST_CAN_DATA_LENGTH] = {0};
    while (1) {
        if (CarCAN_Recv(TEST_CAN_ID_RX, &rx_header, rx_data, portMAX_DELAY) == CAN_ERR) {
            can_error_handler();
        }

        LED_toggle(PRECHARGE_COMPLETE);

        // Optional delay if the can recv isnt blocking
        // vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void tx_task(void *pvParameters) {
    FDCAN_TxHeaderTypeDef tx_header = {0};
    tx_header.Identifier = TEST_CAN_ID_TX;
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0;

    uint8_t tx_data[TEST_CAN_DATA_LENGTH] = TEST_CAN_DATA_TX;
    while (1) {
        if (CarCAN_Send(&tx_header, tx_data, portMAX_DELAY) == CAN_ERR) {
            can_error_handler();
        }

        LED_toggle(HB);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main() {
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LED_init();

    if (CarCAN_Init() != CAN_OK) {
        can_error_handler();
    }

    xTaskCreateStatic(
        tx_task, 
        "tx_task", 
        TESTING_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 2, 
        tx_task_stack, 
        &tx_task_buffer
    );

    xTaskCreateStatic(
        rx_task, 
        "rx_task", 
        TESTING_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        rx_task_stack, 
        &rx_task_buffer
    );

    vTaskStartScheduler();

    while (1) {
    }
}