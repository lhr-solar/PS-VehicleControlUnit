// Wire the two CAN's together to use this test. On the first iteration, it will test the CAN Forwarding function. Screen UART.
// MUST UNCOMMENT LINE IN CAN FORWARDING TASK THAT ENABLES BENCHTOP TESTING

#include "inits.h"
#include "CANbus.h"
#include "StatusLEDs.h"
#include "UART_Init.h"

#define TEST_TASK_STACK_SIZE   configMINIMAL_STACK_SIZE
#define TEST_TASK_PRIORITY     (tskIDLE_PRIORITY + 3)

static StaticTask_t task_buffer;
static StackType_t task_stack[TEST_TASK_STACK_SIZE];

#define CAN_DELAY_MS  10
#define TEST_ID       0x321

static bool verifyData(const uint8_t tx[], const uint8_t rx[])
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (tx[i] != rx[i])
        {
            return false;
        }
    }
    return true;
}

static void init_tx_header(FDCAN_TxHeaderTypeDef *tx_header, uint32_t id, uint32_t dlc)
{
    tx_header->Identifier          = id;
    tx_header->IdType              = FDCAN_STANDARD_ID;
    tx_header->TxFrameType         = FDCAN_DATA_FRAME;
    tx_header->DataLength          = dlc;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch       = FDCAN_BRS_OFF;
    tx_header->FDFormat            = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl  = FDCAN_STORE_TX_EVENTS;
    tx_header->MessageMarker       = 0;
}

static void task(void *pvParameters)
{
    UNUSED(pvParameters);

    Init_UART_Printf();
    setvbuf(stdout, NULL, _IONBF, 0);

    if (CAN_Init() != CAN_OK)
    {
        printf("CAN init failed!\r\n");
        Error_Handler();
    }

    FDCAN_TxHeaderTypeDef tx_header = {0};
    init_tx_header(&tx_header, TEST_ID, FDCAN_DLC_BYTES_8);

    uint8_t tx_data[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xFF};
    uint8_t car_rx_data[8] = {0};
    uint8_t bps_rx_data[8] = {0};
    FDCAN_RxHeaderTypeDef rx_header = {0};

    while (1)
    {
        // Send from Motor side (FDCAN1) -> receive on Car side (FDCAN3)
        if (Motor_CANBus_Send(&tx_header, tx_data, pdMS_TO_TICKS(CAN_DELAY_MS)) != CAN_OK)
        {
            printf("Motor CAN send failed!\r\n");
            Error_Handler();
        }
        printf("Motor CAN send ok\r\n");

        vTaskDelay(pdMS_TO_TICKS(20));

        if (Car_CANBus_Receive(TEST_ID, &rx_header, car_rx_data, pdMS_TO_TICKS(CAN_DELAY_MS)) != CAN_OK)
        {
            printf("Car CAN receive failed!\r\n");
            Error_Handler();
        }

        if (!verifyData(tx_data, car_rx_data))
        {
            printf("Car CAN data mismatch!\r\n");
            Error_Handler();
        }
        printf("Car CAN receive ok\r\n");

        // Send from Car side (FDCAN3) -> receive on Motor side (FDCAN1)
        if (Car_CANBus_Send(&tx_header, tx_data, pdMS_TO_TICKS(CAN_DELAY_MS)) != CAN_OK)
        {
            printf("Car CAN send failed!\r\n");
            Error_Handler();
        }
        printf("Car CAN send ok\r\n");

        vTaskDelay(pdMS_TO_TICKS(20));

        if (Motor_CANBus_Receive(TEST_ID, &rx_header, bps_rx_data, pdMS_TO_TICKS(CAN_DELAY_MS)) != CAN_OK)
        {
            printf("Motor CAN receive failed!\r\n");
            Error_Handler();
        }

        if (!verifyData(tx_data, bps_rx_data))
        {
            printf("Motor/BPS CAN data mismatch!\r\n");
            Error_Handler();
        }
        printf("Motor CAN receive ok\r\n");

        LED_set(HB, ON);
        vTaskDelay(pdMS_TO_TICKS(250));
        LED_set(HB, OFF);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

int main(void)
{
    HAL_Init();

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    SystemClock_Config();
    LEDs_init();

    xTaskCreateStatic(
        task,
        "CAN_test",
        TEST_TASK_STACK_SIZE,
        NULL,
        TEST_TASK_PRIORITY,
        task_stack,
        &task_buffer
    );

    vTaskStartScheduler();

    while (1) {}
}