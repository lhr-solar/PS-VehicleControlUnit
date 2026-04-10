#include "UpdateFSMInputsTask.h"
#include "Contactors.h"
#include "event_groups.h"

static float brake_threshold = BRAKE_THRESH;

static void rebuild_inputs(void) {
    EventBits_t s = 0;

    if (g_data_read->driver_input.Gear_Forward)
        s |= FORWARD_BIT;
    else if (g_data_read->driver_input.Gear_Reverse)
        s |= REVERSE_BIT;
    else
        s |= NEUTRAL_BIT;

    if (g_data_read->driver_input.Regen_Activate) s |= REGEN_BUTTON_BIT;
    if (g_data_read->driver_input.Regen_Enable) s |= REGEN_ENABLED_BIT;
    if (g_data_read->driver_input.Cruise_Enable) s |= CRUISE_CONTROL_BUTTON_BIT;
    if (g_data_read->bps_status.BPS_Regen_OK) s |= READY_TO_REGEN_BIT;
    if (contactor_get_sense(MOTOR_CONTACTOR) && contactor_get_sense(MOTOR_PRE_CONTACTOR))
        s |= PRECHARGE_COMPLETE_BIT;

    if (g_data_read->accel_brake.BrakePedal_Main_Pos >= brake_threshold) {
        s |= BRAKE_BIT;
        brake_threshold = BRAKE_THRESH_HYST;
    } else {
        brake_threshold = BRAKE_THRESH;
    }

    fsm_set_all_inputs(s);
}

void Task_UpdateFSMInputs(void *args __attribute__((unused))) {
    TickType_t last = xTaskGetTickCount();
    FDCAN_TxHeaderTypeDef tx_header = {0};
    tx_header.Identifier = 0x123;
    tx_header.IdType = FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = FDCAN_DLC_BYTES_8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    tx_header.MessageMarker = 0;
    uint8_t buf[8] = {0};

    while (1) {
        // update from can
        FSMDataIn_t *update = g_data_write;
        MotorCAN_Recv_Status(&update->motor_status, 0);
        MotorCAN_Recv_Velocity(&update->motor_velocity, 0);

        // making these motor can should be CARCAN

        CarCAN_Recv_BPS_Status(&update->bps_status, 0);
        CarCAN_Recv_Pedals_Position(&update->accel_brake, 0);
        CarCAN_Recv_Controls_Status(&update->controls_status, 0);
        if (CarCAN_Recv_Driver_Input(&update->driver_input, 0) == CAN_OK) {
            // Handle driver input
            buf[0] = update->driver_input.Gear_Forward;
            tx_header.Identifier = 0x123;
            CarCAN_Send(&tx_header, buf, sizeof(buf));
        }
        CarCAN_Recv_LWS(&update->lws, 0);

        FSMDataIn_t *tmp;
        taskENTER_CRITICAL();
        tmp = g_data_read;
        g_data_read = g_data_write;
        g_data_write = tmp;
        taskEXIT_CRITICAL();

        buf[0] = g_data_read->driver_input.Gear_Forward;
        tx_header.Identifier = 0x321;
        CarCAN_Send(&tx_header, buf, sizeof(buf));

        rebuild_inputs();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(50));
    }
}
