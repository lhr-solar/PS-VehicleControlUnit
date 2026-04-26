#include "InitTask.h"
#include "StatusLEDs.h"
#include "UART.h"
#include "uart_bootloader.h"

StaticTask_t FaultHandlerTask_Buffer;
StackType_t FaultHandlerTask_Stack[FAULT_HANDLER_TASK_STACK_SIZE];

StaticTask_t Precharge_Task_Buffer;
StackType_t Precharge_Task_Stack[PRECHARGE_TASK_STACK_SIZE];

StaticTask_t Init_Task_Buffer;
StackType_t Init_Task_Stack[INIT_TASK_STACK_SIZE];


StaticTask_t Motor_Control_Task_Buffer;
StackType_t Motor_Control_Task_Stack[MOTOR_CONTROL_TASK_STACK_SIZE];

StaticTask_t Motor_Telemetry_Task_Buffer;
StackType_t Motor_Telemetry_Task_Stack[MOTOR_TELEMETRY_TASK_STACK_SIZE];

StaticTask_t Can_Tx_Telemetry_Task_Buffer;
StackType_t Can_Tx_Telemetry_Task_Stack[CAN_TX_TELEMETRY_STACK_SIZE];

StaticTask_t VCUReceiveCAN_Task_Buffer;
StackType_t VCUReceiveCAN_Task_Stack[configMINIMAL_STACK_SIZE];

StaticTask_t Driver_Input_Task_Buffer;
StackType_t Driver_Input_Task_Stack[configMINIMAL_STACK_SIZE];

#if defined(FIRMWARE_USES_BOOTLOADER)
static StaticTask_t Bootloader_Command_Task_Buffer;
static StackType_t Bootloader_Command_Task_Stack[configMINIMAL_STACK_SIZE];

static void Task_BootloaderCommand(void *argument)
{
    (void)argument;
    uart_bootloader_set_entry_allowed(true);
#if defined(VCU_BOOTLOADER_FULL_DIAG)
    uint32_t service_ticks = 0U;
    LED_set(CAR_HB, LED_ON);
#endif

    while (1)
    {
#if defined(VCU_BOOTLOADER_FULL_DIAG)
        (void)uart_bootloader_service(husart3, pdMS_TO_TICKS(50U));
        service_ticks++;
        if (service_ticks >= 10U)
        {
            service_ticks = 0U;
            Toggle_LED(HB);
        }
#else
        (void)uart_bootloader_service(husart3, portMAX_DELAY);
#endif
    }
}
#endif

void Task_Init()
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

#if defined(VCU_BOOTLOADER_FULL_DIAG)
    LED_set(PRECHARGE_TIMEOUT, LED_ON);
#endif
    Init_UART_Printf();
#if defined(VCU_BOOTLOADER_FULL_DIAG)
    LED_set(PRECHARGE_SENSE_TIMEOUT, LED_ON);
#endif
    CAN_Init();
#if defined(VCU_BOOTLOADER_FULL_DIAG)
    LED_set(MOTOR_SENSE_TIMEOUT, LED_ON);
#endif

    MotorSafeBits_Init();
#if defined(VCU_BOOTLOADER_FULL_DIAG)
    LED_set(CAR_DRIVABLE, LED_ON);
#endif

    xTaskCreateStatic(
        Task_FaultHandler,          // Task function
        "FaultHandler",             // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,   // Stack size in words
        NULL,                       // Task input parameter
        FAULT_HANDLER_THREAD_PRIO,  // Task priority
        FaultHandlerTask_Stack,     // Task handle
        &FaultHandlerTask_Buffer    // Static task buffer (optional)
    );

    hprecharge_task = xTaskCreateStatic(
        Task_Precharge,                 // Task function
        "Precharge",                    // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,       // Stack size in words
        NULL,                           // Task input parameter
        PRECHARGE_THREAD_PRIO,          // Task priority
        Precharge_Task_Stack,           // Task handle
        &Precharge_Task_Buffer          // Static task buffer (optional)
    );

    xTaskCreateStatic(
        Task_VCUReceiveCAN,        // Task function
        "VCUReceiveCAN",           // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,  // Stack size in words
        NULL,                      // Task input parameter
        tskIDLE_PRIORITY + 2,      // Task priority
        VCUReceiveCAN_Task_Stack,  // Task handle
        &VCUReceiveCAN_Task_Buffer // Static task buffer (optional)
    );

    xTaskCreateStatic(
        Task_DriverInputTest,
        "DriverInputTest",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 2,
        Driver_Input_Task_Stack,
        &Driver_Input_Task_Buffer);

#if defined(FIRMWARE_USES_BOOTLOADER)
    xTaskCreateStatic(
        Task_BootloaderCommand,
        "BootCommand",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 2,
        Bootloader_Command_Task_Stack,
        &Bootloader_Command_Task_Buffer);
#endif

#if defined(VCU_BOOTLOADER_FULL_DIAG)
    LED_set(CAR_DRIVING, LED_ON);
#endif

    vTaskDelete(NULL);
}
