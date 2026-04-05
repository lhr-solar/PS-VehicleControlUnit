// Wire the two CAN's together to use this test. On the first iteration, it will test the CAN Forwarding function. Screen UART.
// MUST UNCOMMENT LINE IN CAN FORWARDING TASK THAT ENABLES BENCHTOP TESTING

#include "inits.h"
#include "CANbus.h"
#include "StatusLEDs.h"
#include "UART_Init.h"

#define TEST_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define TEST_TASK_PRIORITY              tskIDLE_PRIORITY + 3

StaticTask_t task_buffer;
StackType_t task_stack[TEST_TASK_STACK_SIZE];

#define can_delay_ms 10

static bool verifyData(uint8_t tx[], uint8_t rx[])
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (tx[i] != rx[i])
            return false;
    }
    return true;
}

// static void initTxHeader(FDCAN_TxHeaderTypeDef *tx_header)
// {
//     tx_header->Identifier = 0x321;
//     tx_header->IdType = FDCAN_STANDARD_ID;
//     tx_header->TxFrameType = FDCAN_DATA_FRAME;
//     tx_header->DataLength = FDCAN_DLC_BYTES_8;
//     tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
//     tx_header->BitRateSwitch = FDCAN_BRS_OFF;
//     tx_header->FDFormat = FDCAN_CLASSIC_CAN;
//     tx_header->TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
//     tx_header->MessageMarker = 0;
// }

// static void task(void *pvParameters)
// {

//     Init_UART_Printf();
//     printf("printf initialized\r\n");
//     Motor_CANBus_Init();
//     printf("Motor CAN initialized successfully\r\n");
//     Car_CANBus_Init();
//     printf("Car CAN initialized successfully\r\n");

//     int test_id = 0x321;

//     // send x1234 to 0x11
//     uint8_t tx_data[8] = {0};
//     tx_data[0] = 0x12;
//     tx_data[1] = 0x34;
//     tx_data[2] = 0x56;
//     tx_data[3] = 0x78;
//     tx_data[4] = 0x9A;
//     tx_data[5] = 0xBC;
//     tx_data[6] = 0xDE;
//     tx_data[7] = 0xFF;

//     uint8_t fdcan1_rx_data[8] = {0};

//     uint8_t fdcan3_rx_data[8] = {0};

//     FDCAN_TxHeaderTypeDef tx_header;
//     initTxHeader(&tx_header);

//     FDCAN_RxHeaderTypeDef rx_header = {0};

//     while (1)
//     {

//         if (Motor_CANBus_Send(&tx_header, tx_data, can_delay_ms) == CAN_ERR)
//         {
//             printf("Motor CAN failed to send!\r\n");
//             Error_Handler();
//         }
//         printf("Motor CAN Sent successfully!\r\n");

//         vTaskDelay(pdMS_TO_TICKS(20));

//         if ((Car_CANBus_Recieve(test_id, &rx_header, tx_data, can_delay_ms) != CAN_OK) || !verifyData(fdcan3_rx_data, tx_data))
//         {
//             printf("CAR CAN failed to receive!\r\n");
//             Error_Handler();
//         }
//         printf("CAR CAN Receieve successfully!\r\n");

//         if (Car_CANBus_Send(&tx_header, fdcan3_rx_data, can_delay_ms) == CAN_ERR)
//         {
//             printf("CAR CAN failed to send!\r\n");
//             Error_Handler();
//         }
//         printf("CAR CAN Sent successfully!\r\n");

//         vTaskDelay(pdMS_TO_TICKS(20));

//         if ((Motor_CANBus_Recieve(test_id, &rx_header, fdcan1_rx_data, can_delay_ms) != CAN_OK) || !verifyData(fdcan1_rx_data, tx_data))
//         {
//             printf("Motor CAN failed to receive!\r\n");
//             Error_Handler();
//         }
//         printf("Motor CAN Receieve successfully!\r\n");

//         vTaskDelay(pdMS_TO_TICKS(20));

//         LED_set(HB, ON);
//         vTaskDelay(500);
//         LED_set(HB, OFF);
//         vTaskDelay(500);
//     }
// }

static void task(void *pvParameters) {

    Init_UART_Printf();
    printf("printf initialized\r\n");
    CAN_Init();
    printf("CAN initialized successfully\r\n");

    int test_id = 0x321;

    // send x1234 to 0x11
    uint8_t tx_data[8] = {0};
    tx_data[0] = 0x12;
    tx_data[1] = 0x34;
    tx_data[2] = 0x56;
    tx_data[3] = 0x78;
    tx_data[4] = 0x9A;
    tx_data[5] = 0xBC;
    tx_data[6] = 0xDE;
    tx_data[7] = 0xFF;

    uint8_t fdcan1_rx_data[8] = {0};

    uint8_t fdcan3_rx_data[8] = {0};

    while(1){

    if (bps_can_send(test_id, tx_data, FDCAN_DLC_BYTES_8, can_delay_ms) == CAN_ERR){
        printf("BPS CAN failed to send!\r\n");
        Error_Handler();
    }
    printf("BPS CAN Sent successfully!\r\n");

    vTaskDelay(pdMS_TO_TICKS(20));

    if((car_can_recv(test_id, tx_data, FDCAN_DLC_BYTES_8, can_delay_ms) != CAN_OK) && verifyData(fdcan1_rx_data, tx_data)){
        printf("CAR CAN failed to receive!\r\n");
        Error_Handler();
    }
    printf("CAR CAN Receieve successfully!\r\n");

    if (car_can_send(test_id, fdcan3_rx_data, FDCAN_DLC_BYTES_8, can_delay_ms) == CAN_ERR){
        printf("CAR CAN failed to send!\r\n");
        Error_Handler();
    }
    printf("CAR CAN Sent successfully!\r\n");

    vTaskDelay(pdMS_TO_TICKS(20));

    if((bps_can_recv(test_id, fdcan1_rx_data, FDCAN_DLC_BYTES_8, can_delay_ms) != CAN_OK) && verifyData(fdcan1_rx_data, tx_data)){
        printf("BPS CAN failed to receive!\r\n");
        Error_Handler();
    }
    printf("BPS CAN Receieve successfully!\r\n");

    vTaskDelay(pdMS_TO_TICKS(20));

    LED_set(HB, ON);
    vTaskDelay(500);
    LED_set(HB, OFF);
    vTaskDelay(500);

    }
}

int main(void)
{
    HAL_Init();

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // System clock config can change depending on the target MCU, since the clock tree can be different
    // If you need to use a different MCU, go to cubemx and generate a new system clock config function with the system clock being 80 Mhz
    // It especially varies with nucleo vs direct MCU
    // G473_SystemClockConfig();

    SystemClock_Config();

    LEDs_init(); // enable LED for LED_PORT

    // you can only send CAN messages within a FreeRTOS task
    xTaskCreateStatic(
        task,
        "task",
        TEST_TASK_STACK_SIZE,
        NULL,
        TEST_TASK_PRIORITY,
        task_stack,
        &task_buffer);

    vTaskStartScheduler();
    while (1)
    {
    }

    Error_Handler();
    return 0;
}