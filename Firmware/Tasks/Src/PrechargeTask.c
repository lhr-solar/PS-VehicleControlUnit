#include "PrechargeTask.h"

/* handle for the Precharge task, defined here */
TaskHandle_t hprecharge_task = NULL;

StaticEventGroup_t xPrechargeEventGroup;
EventGroupHandle_t xPrechargeEventGroup_handle;

uint32_t Battery_Voltage = 0;
uint32_t Motor_Voltage = 0;
static Precharge_State_t State;
uint8_t Ignition_State = 0;

static StaticQueue_t driverInputQueueBuffer;
static uint8_t driverInputQueueStorage[DRIVER_INPUT_QUEUE_SIZE * sizeof(can_rx_payload_t)];
static QueueHandle_t driverInputQueue;

static FDCAN_TxHeaderTypeDef VCUSendVoltageHeader;

static void initVCUSendVoltageHeader(FDCAN_TxHeaderTypeDef *tx_header)
{
    tx_header->Identifier = CAN_ID_VCU_PRECHARGE_VOLTAGES;
    tx_header->IdType = FDCAN_STANDARD_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->DataLength = FDCAN_DLC_BYTES_8;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header->MessageMarker = 0;
}

static void initDriverInputQueue()
{
    driverInputQueue = xQueueCreateStatic(
        DRIVER_INPUT_QUEUE_SIZE,
        sizeof(can_rx_payload_t),
        driverInputQueueStorage,
        &driverInputQueueBuffer);

    if (driverInputQueue == NULL)
    {
        return;
    }
}

void Check_Ignition_State()
{
    if (xQueueReceive(driverInputQueue, &payload, pdMS_TO_TICKS(1000)) == pdTRUE)
    {
        if (payload.header.Identifier == CAN_ID_DRIVER_INPUT_STATUS && payload.data[IGNITION_MOTOR_INDEX] == 1 && Ignition_State == 0) // index of ignition_motor = 1
        {
            Ignition_State = 1;
            State = PRECHARGE_STATE_INITIAL;
            printf("Ignition ON, starting precharge sequence\r\n");
        }
        else if (payload.header.Identifier == CAN_ID_DRIVER_INPUT_STATUS && payload.data[IGNITION_MOTOR_INDEX] == 0 && Ignition_State == 1) // index of ignition_motor = 1
        {
            Ignition_State = 0;
            State = PRECHARGE_STATE_WAITING; // Reset to initial state?
            printf("Ignition OFF, stopping precharge sequence\r\n");
            Ignition_Off();
        }
    }
}

void Ignition_Off() // TODO: Open contactors one by one
{
    printf("Ignition OFF, opening motor precharge contactor\r\n");

    if (contactor_set(MOTOR_PRE_CONTACTOR, OPEN, CALLBACK_BLOCKING_TIME, NORMAL) != SUCCESS)
    {
        set_faultBit(PRECHARGE_SENSE_TIMEOUT_FAULT);
    }

    printf("Ignition OFF, opening motor contactor\r\n");

    if (contactor_set(MOTOR_CONTACTOR, OPEN, CALLBACK_BLOCKING_TIME, NORMAL) != SUCCESS)
    {
        set_faultBit(MOTOR_SENSE_TIMEOUT_FAULT);
    }

    // Return task to idle/waiting state
    State = PRECHARGE_STATE_WAITING;

    printf("Ignition OFF, shutdown complete\r\n");
}

void Init_PrechargeTask()
{
    // Event Group init
    xPrechargeEventGroup_handle = xEventGroupCreateStatic(&xPrechargeEventGroup);
    configASSERT(xPrechargeEventGroup_handle); // check if handle is set
    // xEventGroupClearBits(xReadADCEventGroup_handle,    /* The event group being updated. */
    //                      0xFF );                    /* The bits being cleared. */

    // Inits ADC & contactors
    ADC_Sense_Init();
    contactor_init();
    MotorSafeBits_Init();
    initVCUSendVoltageHeader(&VCUSendVoltageHeader);
    initDriverInputQueue();
}

void can_fd_rx_callback_hook(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs, can_rx_payload_t recv_payload)
{

    BaseType_t higherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(
        driverInputQueue,
        &recv_payload,
        &higherPriorityTaskWoken);
}

// encodes battery and motor voltage into an array of bytes for can_send
static void packMotorVoltage(vcu_precharge_voltages_t voltages, uint8_t tx_data[8])
{
    memcpy(&tx_data[0], &(voltages.Precharge_Battery_Voltage), sizeof(uint32_t)); // TODO: Have Precharge task push adc readings into the can messages struct
    memcpy(&tx_data[4], &(voltages.Precharge_Motor_Voltage), sizeof(uint32_t));
}

void Fault_Checker(uint32_t Motor_Voltage, uint32_t Battery_Voltage)
{
    if (Motor_Voltage > (Battery_Voltage * VOLTAGE_TOLERANCE_NUMERATOR / VOLTAGE_TOLERANCE_DENOMINATOR))
    {
        // Fault handler
        set_faultBit(MOTOR_GREATER_THAN_BATTERY_FAULT);
    }

    if (Battery_Voltage > OVERVOLTAGE_THRESHOLD_MV)
    {
        /* BATTERY ABOUT TO GO BOOM */
        // Fault handler
        set_faultBit(BATTERY_OVERVOLTAGE_FAULT);
    }

    if (Battery_Voltage < UNDERVOLTAGE_THRESHOLD_MV)
    {
        /* Battery voltage is too low or battery is disconnected, treat as fault */
        // Fault handler
        set_faultBit(BATTERY_UNDERVOLTAGE_FAULT);
    }

    if (contactor_get_sense(MOTOR_CONTACTOR) != contactor_get_commanded_state(MOTOR_CONTACTOR))
    {
        // Fault handler
        set_faultBit(MOTOR_SENSE_MISMATCH_FAULT);
    }

    if (contactor_get_sense(MOTOR_PRE_CONTACTOR) != contactor_get_commanded_state(MOTOR_PRE_CONTACTOR))
    {
        // Fault handler
        set_faultBit(PRECHARGE_SENSE_MISMATCH_FAULT);
    }

    printf("Motor Sense Pin Reading: %d\r\n", contactor_get_sense(MOTOR_CONTACTOR));
    printf("Precharge Sense Pin Reading: %d\r\n", contactor_get_sense(MOTOR_PRE_CONTACTOR));
    printf("Motor Contactor State: %d\r\n", contactor_get_commanded_state(MOTOR_CONTACTOR));
    printf("Precharge Contactor State: %d\r\n", contactor_get_commanded_state(MOTOR_PRE_CONTACTOR));
}

void Task_Precharge()
{
    Init_PrechargeTask();

    State = PRECHARGE_STATE_WAITING;
    static TickType_t Start_Tick = 0;

    ADC_Sense_Result ADC_Result = {0};

    can_rx_payload_t payload;

    uint8_t VCU_tx_data[8];
    vcu_precharge_voltages_t precharge_voltages = {0};
    uint8_t can_send_errors = 0;

    while (1)
    {
        LED_set(HB, ON);
        vTaskDelay(500);
        LED_set(HB, OFF);
        vTaskDelay(500);

        if (Read_ADC(ADC_TIMEOUT_MS, &ADC_Result) != ADC_SENSE_OK)
        {
            Error_Handler();
        }

        // TODO: Send voltages through car can
        Battery_Voltage = ADC_Result.Battery_Voltage;
        Motor_Voltage = ADC_Result.Motor_Voltage;
        precharge_voltages.Precharge_Battery_Voltage = Battery_Voltage;
        precharge_voltages.Precharge_Motor_Voltage = Motor_Voltage;

        packMotorVoltage(precharge_voltages, VCU_tx_data);

        if (Car_CANBus_Send(&VCUSendVoltageHeader, VCU_tx_data, portMAX_DELAY) == CAN_ERR)
        {
            can_send_errors++;
        }
        else
        {
            can_send_errors = 0;
        }

        printf("Motor: %ld mV | Battery: %ld mV\r\n", Motor_Voltage, Battery_Voltage);

        switch (State)
        {
        case PRECHARGE_STATE_WAITING:

            set_stateBit(PRECHARGE_WAITING_STATE);
            Check_Ignition_State(); // Wait for ignition on message from driver input task, then move to initial precharge state
            printf("Precharge State: Waiting for Ignition\r\n");
            
            break;
        case PRECHARGE_STATE_INITIAL: // Startup state: Closes main contactor and moves to precharging state

            Check_Ignition_State();

            set_stateBit(PRECHARGE_INITIAL_STATE);

            printf("Precharge State: Initial\r\n");

            if (contactor_set(MOTOR_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, NORMAL) != SUCCESS)
            {
                set_faultBit(MOTOR_SENSE_TIMEOUT_FAULT);
            }

            State = PRECHARGE_STATE_PRECHARGING;

            set_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);

            // Start a timer for precharging
            Start_Tick = xTaskGetTickCount();

            break;
        case PRECHARGE_STATE_PRECHARGING: // Precharging state: Waits for battery voltage to reach 90% of motor voltage, then closes precharge contactor and moves to run state
            
            Check_Ignition_State();

            set_stateBit(PRECHARGE_PRECHARGING_STATE);

            Fault_Checker(Motor_Voltage, Battery_Voltage); // Check for faults while precharging, if any fault conditions are met, will call fault handler and not proceed with precharge sequence

            const TickType_t Current_Tick = xTaskGetTickCount(); // Check how long we've been precharging for, fault if not precharged after PRECHARGE_TIMEOUT_MS
            printf("Precharge State: Precharging\r\n");
            if ((Current_Tick - Start_Tick) > pdMS_TO_TICKS(PRECHARGE_TIMEOUT_MS)) // Faults if precharging takes too long
            {
                // Check if motor voltage is within 90% of battery voltage (precharge complete)
                if (Motor_Voltage * RATIO_SCALE >= Battery_Voltage * PRECHARGE_THRESHOLD_90)
                {
                    if (contactor_set(MOTOR_PRE_CONTACTOR, CLOSED, CALLBACK_BLOCKING_TIME, false) != SUCCESS)
                    {
                        set_faultBit(PRECHARGE_SENSE_TIMEOUT_FAULT);
                    }
                    State = PRECHARGE_STATE_RUN;

                    set_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);
                }
                else
                {
                    // Precharging took too long
                    set_faultBit(PRECHARGE_TIMEOUT_FAULT);
                }
            }
            break;
        case PRECHARGE_STATE_RUN: // Run state: Continuously checks that motor voltage stays within 80% of battery voltage

            Check_Ignition_State();

            set_stateBit(PRECHARGE_RUN_STATE);

            Fault_Checker(Motor_Voltage, Battery_Voltage); // Check for faults while precharging, if any fault conditions are met, will call fault handler and not proceed with precharge sequence

            // Use 80% threshold for hysteresis
            printf("Precharge State: Run\r\n");
            if (Motor_Voltage * RATIO_SCALE < Battery_Voltage * PRECHARGE_THRESHOLD_80)
            {
            }
            break;
        default:
            break;
        }

        vTaskDelay(PRECHARGE_TASK_DELAY_MS);
    }
}