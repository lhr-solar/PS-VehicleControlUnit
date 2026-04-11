#include "CANbus.h"
#include "stm32xx_hal.h"
#include "inits.h"
#include "StatusLEDs.h"
#include "pinDefs.h"
#include "CanTxTelemetryTask.h"

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

        if (Motor_CANBus_Send( &tx_header, tx_data, portMAX_DELAY) == CAN_ERR){}

        Toggle_LED(HB);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(){

    HAL_Init();

    SystemClock_Config();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    LEDs_init();


    Motor_CANBus_Init();

    Init_UART_Printf();


    xTaskCreateStatic(
        Task_CanTxTelemetry,            // Task function
        "Can TX Telemetry Thread",      // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,       // Stack size in words
        NULL,                           // Task input parameter
        CAN_TX_TELEMETRY_THREAD_PRIO,   // Task priority
        Can_Tx_Telemetry_Task_Stack,     // Task handle
        &Can_Tx_Telemetry_Task_Buffer    // Static task buffer (optional)
    );

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
    return 0;
}