#include "FaultHandlerTask.h"
#include "PrechargeTask.h" // for hprecharge_task handle

#define FAULT_LOOP_PRINTF_DELAY_MS 10000

#define FAULT_PRINTF_COUNTER (FAULT_LOOP_PRINTF_DELAY_MS/FAULT_LOOP_PERIOD_MS)

EventBits_t fault_bits = 0;
EventBits_t state_bits = 0;

static FDCAN_TxHeaderTypeDef VCUSendStatusHeader;

static void initVCUSendStatusHeader(FDCAN_TxHeaderTypeDef *tx_header)
{
    tx_header->Identifier = CAN_ID_VCU_STATUS;
    tx_header->IdType = FDCAN_STANDARD_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->DataLength = FDCAN_DLC_BYTES_8;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_CLASSIC_CAN;
    tx_header->TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header->MessageMarker = 0;
}

void Init_FaultHandlerTask()
{
    if (faultBits_init() != 1)
    {
        // Fault bit initialization failed
        Error_Handler();
    }

    if (stateBits_init() != 1)
    {
        // Fault bit initialization failed
        Error_Handler();
    }

    initVCUSendStatusHeader(&VCUSendStatusHeader);
}

void Kill_Precharge_Task()
{
    if (hprecharge_task != NULL)
    {
        vTaskDelete(hprecharge_task);
    }
}

static void print_fault(){
    switch (fault_bits) // compare against individual bitmasks
        {
            case FAULT_BIT(MOTOR_GREATER_THAN_BATTERY_FAULT):
                printf("Fault: Motor Voltage Greater Than Battery Voltage\r\n");
                break;
            case FAULT_BIT(BATTERY_OVERVOLTAGE_FAULT):
                printf("Fault: Battery Overvoltage\r\n");
                break;
            case FAULT_BIT(BATTERY_UNDERVOLTAGE_FAULT):
                printf("Fault: Battery Undervoltage\r\n");
                break;
            case FAULT_BIT(MOTOR_SENSE_TIMEOUT_FAULT):
                printf("Fault: Motor Sense Timeout\r\n");
                break;
            case FAULT_BIT(PRECHARGE_SENSE_TIMEOUT_FAULT):
                printf("Fault: Precharge Sense Timeout\r\n");
                break;
            case FAULT_BIT(PRECHARGE_TIMEOUT_FAULT):
                printf("Fault: Precharge Sequence Timeout\r\n");
                break;
            case FAULT_BIT(CALLBACK_FAULT):
                printf("Fault: Contactor Sense Fault\r\n");
                break;
            case FAULT_BIT(MOTOR_SENSE_MISMATCH_FAULT):
                printf("Fault: Motor Sense Mismatch\r\n");
                break;
            case FAULT_BIT(PRECHARGE_SENSE_MISMATCH_FAULT):
                printf("Fault: Precharge Sense Mismatch\r\n");
                break;
            default:
                printf("Fault: Unknown\r\n");
                break;
        }
}

void Fault_Loop()
{
    uint32_t fault_printf_debug_counter = 0;
    while (1)
    {
        switch (fault_bits) // compare against individual bitmasks
        {
        case FAULT_BIT(MOTOR_GREATER_THAN_BATTERY_FAULT):
            printf("Fault: Motor Voltage Greater Than Battery Voltage\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(BATTERY_OVERVOLTAGE_FAULT):
            printf("Fault: Overvoltage\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(BATTERY_UNDERVOLTAGE_FAULT):
            printf("Fault: Undervoltage\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(MOTOR_SENSE_TIMEOUT_FAULT):
            printf("Fault: Motor Sense Timeout\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(PRECHARGE_SENSE_TIMEOUT_FAULT):
            printf("Fault: Precharge Sense Timeout\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(PRECHARGE_TIMEOUT_FAULT):
            printf("Fault: Precharge Sequence Timeout\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(CALLBACK_FAULT):
            printf("Fault: Contactor Sense Fault\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(MOTOR_SENSE_MISMATCH_FAULT):
            printf("Fault: Motor Sense Mismatch\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(PRECHARGE_SENSE_MISMATCH_FAULT):
            printf("Fault: Precharge Sense Mismatch\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        case FAULT_BIT(BPS_FAULT):
            printf("Fault: BPS Fault\r\n");
            vTaskDelay(PRINTF_DELAY_MS);
            break;
        default:
            break;
        fault_printf_debug_counter++;

        if(fault_printf_debug_counter >= FAULT_PRINTF_COUNTER){
            print_fault();
            fault_printf_debug_counter = 0;
        }

        Toggle_LED(HB);
        vTaskDelay(FAULT_LOOP_PERIOD_MS);
        }
    }
}

void Set_Fault_LED()
{
    switch (fault_bits) // compare against individual bitmasks
    {
    case FAULT_BIT(MOTOR_GREATER_THAN_BATTERY_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(BATTERY_OVERVOLTAGE_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(BATTERY_UNDERVOLTAGE_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(MOTOR_SENSE_TIMEOUT_FAULT):
        LED_set(MOTOR_SENSE_TIMEOUT, LED_ON);
        break;
    case FAULT_BIT(PRECHARGE_SENSE_TIMEOUT_FAULT):
        LED_set(PRECHARGE_SENSE_TIMEOUT, LED_ON);
        break;
    case FAULT_BIT(PRECHARGE_TIMEOUT_FAULT):
        LED_set(PRECHARGE_TIMEOUT, LED_ON);
        break;
    case FAULT_BIT(CALLBACK_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(MOTOR_SENSE_MISMATCH_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(PRECHARGE_SENSE_MISMATCH_FAULT):
        LED_set(CAR_BPSFAULT, LED_ON);
        break;
    case FAULT_BIT(BPS_FAULT):
        LED_set(CAR_BPSFAULT, ON);
        break;
    default:
        break;
    }
}

static uint8_t convertFaultBitsToIndex(uint32_t faults)
{
    for (int i = 0; i < FAULT_NUM; i++)
    {
        if (faults & (1U << i))
        {
            return i + 1; // fault codes start at 1, 0 = no fault
        }
    }

    return 0; // no fault active
}

// encodes VCU status struct into an array of bytes for can_send
static void packVCUStatus(vcu_status_t status, uint8_t tx_data[8])
{
    uint32_t faults = fault_bits;
    uint8_t fault_code = convertFaultBitsToIndex(faults);
    tx_data[0] = fault_code;

    uint8_t motor_contactor_state = (state_bits >> 1) & 0x1;
    uint8_t precharge_contactor_state = (state_bits >> 2) & 0x1;
    uint8_t ready_to_drive = (state_bits >> 3) & 0x1;

    tx_data[1] = motor_contactor_state;     // Motor_Contactor_State
    tx_data[2] = precharge_contactor_state; // Motor_Precharge_Contactor_State
    tx_data[3] = ready_to_drive;            // Motor_Ready_To_Drive
    tx_data[4] = 0;                         // VCU_Driver_Input_OK
    tx_data[5] = 0;                         // VCU_Pedals_OK
    tx_data[6] = 0;                         // VCU_Regen_OK
    tx_data[7] = 0;                         // VCU_Regen_Active / FSM_State
}

void Task_FaultHandler()
{
    Init_FaultHandlerTask();

    uint8_t VCU_tx_data[8];
    vcu_status_t VCUStatus = {0};

    uint8_t can_send_errors = 0;

    while (1)
    {
        packVCUStatus(VCUStatus, VCU_tx_data);

        if (Car_CANBus_Send(&VCUSendStatusHeader, VCU_tx_data, portMAX_DELAY) == CAN_ERR)
        {
            can_send_errors++;
        }
        else
        {
            can_send_errors = 0;
        }

        fault_bits = faultBit_wait(NUM_FAULTS, portMAX_DELAY);

        if (fault_bits != 0)
        {
            Kill_Precharge_Task();
            contactor_emergency_open_all();

            // prevents the motor from running
            clear_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);
            clear_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);


            printf("Fault Handler triggered with bitmask: 0x%02lX\r\n", fault_bits);

            Set_Fault_LED();
            Fault_Loop();
        }
        
        vTaskDelay(1000);
    }

    vTaskDelay(1000);
}