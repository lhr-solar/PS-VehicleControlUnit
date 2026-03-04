#include "CANbus.h"
#include "stm32xx_hal.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"

StaticTask_t task_buffer;
StackType_t task_stack[512];

static void task(void *pvParameters){

    int test_id = 0x321;
    FDCAN_TxHeaderTypeDef tx_header = {0};   
    tx_header.Identifier = test_id;
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header.MessageMarker = 0;

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
    
    while(1){

        if (can_fd_send(hfdcan3, &tx_header, tx_data, portMAX_DELAY) == CAN_ERR){
            Error_Handler();
        }

        // TODO: add status LED
        HAL_GPIO_TogglePin(HB_LED_PORT, HB_LED_PIN);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void can_error_handler(){
    while(1){
        LED_set(MOTOR_FAULT, GPIO_PIN_SET);
    }
}

int main(){
    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LEDs_init();


    if(Motor_CANBus_Init() != CAN_OK){
        can_error_handler();
    }

    xTaskCreateStatic(
                task,
                "task",
                512,
                NULL,
                tskIDLE_PRIORITY + 2,
                task_stack,
                &task_buffer);

    
    vTaskStartScheduler();

    while(1){

    }
}