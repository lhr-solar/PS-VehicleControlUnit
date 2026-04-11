#include "PrechargeTask.h"

#define PRECHARGE_PRINTF_DEBUG_PERIOD_MS 10000
#define PRECHARGE_PRINTF_DEBUG_COUNTER (PRECHARGE_PRINTF_DEBUG_PERIOD_MS/PRECHARGE_TASK_DELAY_MS)

/* handle for the Precharge task, defined here */
TaskHandle_t hprecharge_task = NULL;

StaticEventGroup_t xPrechargeEventGroup;
EventGroupHandle_t xPrechargeEventGroup_handle;

uint32_t Battery_Voltage = 0;
uint32_t Motor_Voltage = 0;
static Precharge_State_t State;
uint8_t Ignition_State = 0;
// uint8_t Ignition_Off_Initiated = 0;
TickType_t offTick = 0;

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

void Ignition_Off()
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

    State = PRECHARGE_STATE_WAITING; // Return task to idle/waiting state

    printf("Ignition OFF, shutdown complete\r\n");
}

void Check_Ignition_State()
{
    // if (Ignition_Off_Initiated && (xTaskGetTickCount() - offTick) > pdMS_TO_TICKS(IGNITION_OFF_DELAY_MS))

    if (Start_Precharge)
    {
        Start_Precharge = 0;
        Ignition_State = 1;
        State = PRECHARGE_STATE_INITIAL;
        printf("Ignition ON, starting precharge sequence\r\n");
    }
    else if (End_Precharge)
    {
        End_Precharge = 0;
        Ignition_State = 0;
        printf("Ignition OFF, stopping precharge sequence\r\n");
        Ignition_Off();
    }
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
}

// encodes battery and motor voltage into an array of bytes for can_send
static void packMotorVoltage(vcu_precharge_voltages_t voltages, uint8_t tx_data[8])
{
    memcpy(&tx_data[0], &(voltages.VCU_Precharge_Battery_Voltage), sizeof(uint32_t)); // TODO: Have Precharge task push adc readings into the can messages struct
    memcpy(&tx_data[4], &(voltages.VCU_Precharge_Motor_Voltage), sizeof(uint32_t));
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
}

static void print_Precharge_State(Precharge_State_t State){
    switch (State)
    {
        case PRECHARGE_STATE_INITIAL:
            printf("Precharge State: Initial\r\n");
            break;
        case PRECHARGE_STATE_PRECHARGING:
            printf("Precharge State: Precharging\r\n");
            break;
        case PRECHARGE_STATE_RUN:
            printf("Precharge State: Run\r\n");
            break;
        default:
            printf("Unknown\r\n");
            break;
    }
}

void Task_Precharge()
{
    Init_PrechargeTask();

    State = PRECHARGE_STATE_WAITING;
    static TickType_t Start_Tick = 0;

    ADC_Sense_Result ADC_Result = {0};

    uint8_t VCU_tx_data[8];
    vcu_precharge_voltages_t precharge_voltages = {0};
    uint8_t can_send_errors = 0;

    while (1)
    {

        if (Read_ADC(ADC_TIMEOUT_MS, &ADC_Result) != ADC_SENSE_OK)
        {
            Error_Handler();
        }

        Battery_Voltage = ADC_Result.Battery_Voltage;
        Motor_Voltage = ADC_Result.Motor_Voltage;
        precharge_voltages.VCU_Precharge_Battery_Voltage = Battery_Voltage;
        precharge_voltages.VCU_Precharge_Motor_Voltage = Motor_Voltage;

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
                    set_faultBit(MOTOR_SENSE_TIMEOUT_FAULT);
                }
                State = PRECHARGE_STATE_PRECHARGING;

                // Start a timer for precharging
                Start_Tick = xTaskGetTickCount();
                break;
            case PRECHARGE_STATE_PRECHARGING: // Precharging state: Waits for battery voltage to reach 90% of motor voltage, then closes precharge contactor and moves to run state

                Fault_Checker(Motor_Voltage, Battery_Voltage); // Check for faults while precharging, if any fault conditions are met, will call fault handler and not proceed with precharge sequence

                const TickType_t Current_Tick = xTaskGetTickCount(); // Check how long we've been precharging for, fault if not precharged after PRECHARGE_TIMEOUT_MS
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
                    }
                    else
                    {
                        // Precharging took too long
                        set_faultBit(PRECHARGE_TIMEOUT_FAULT);
                    }
                    State = PRECHARGE_STATE_RUN;

                    set_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);
                }
                break;
            case PRECHARGE_STATE_RUN: // Run state: Continuously checks that motor voltage stays within 80% of battery voltage

                Fault_Checker(Motor_Voltage, Battery_Voltage); // Check for faults while precharging, if any fault conditions are met, will call fault handler and not proceed with precharge sequence

                // Use 80% threshold for hysteresis
                if (Motor_Voltage * RATIO_SCALE < Battery_Voltage * PRECHARGE_THRESHOLD_80)
                {
                    
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

        if(printDebugCounter >= PRECHARGE_PRINTF_DEBUG_COUNTER){

            // prints battery and motor voltage
            printf("Motor: %ld mV | Battery: %ld mV\r\n",
               Motor_Voltage,
               Battery_Voltage);

            // prints current precharge state
            print_Precharge_State(State);
            printDebugCounter = 0;
        }

        // set the Precharge Complete LED 
        LED_set(PRECHARGE_COMPLETE, State == PRECHARGE_STATE_RUN ? LED_ON : LED_OFF);

        // update the motor safe bits with Contactor state        
        if(contactor_get_sense(MOTOR_PRE_CONTACTOR) == CLOSED){
            set_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);
        }
        else{
            clear_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);
        }

        if(contactor_get_sense(MOTOR_CONTACTOR) == CLOSED){
            set_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);
        }
        else{
            clear_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);
        }

        Toggle_LED(HB);
        vTaskDelay(PRECHARGE_TASK_DELAY_MS);
    }
}