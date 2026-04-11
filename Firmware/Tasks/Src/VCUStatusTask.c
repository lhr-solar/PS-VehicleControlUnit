#include "VCUStatusTask.h"
#include "CANbus.h"
#include "event_groups.h"
#include "FaultBits.h"
#include "Contactors.h"
#include "FSM.h"
#include "Watchdogs.h"
#include "StatusLEDs.h"

void Task_BroadcastVCUStatus(void *args __attribute__((unused))) {
    uint8_t buf[CAN_DLC_VCU_STATUS];

    while (1) {
        // Byte 0: VCU_Fault — map internal faults to DBC enum
        uint8_t vcu_fault = VCU_STATUS_VCU_FAULT_NO_FAULT;
        if (faults_is_active(FAULT_ID_PRECHARGE_TIMEOUT))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_PRECHARGE_TIMEOUT;
        else if(faults_is_active(VCU_STATUS_VCU_FAULT_MOTOR_PCHG_CONTACTOR_SENSE))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_PCHG_CONTACTOR_SENSE;
        else if(faults_is_active(VCU_STATUS_VCU_FAULT_MOTOR_CONTACTOR_SENSE))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_CONTACTOR_SENSE;
        else if (faults_is_active(FAULT_ID_MOTOR_DC_BUS_OVERVOLTAGE))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_HV_OVERVOLTAGE;
        else if (faults_is_active(FAULT_ID_MOTOR_15V_UNDERVOLTAGE))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_HV_UNDERVOLTAGE;
        else if (faults_is_active(FAULT_ID_MOTOR_HARDWARE_OVERCURRENT) ||
                 faults_is_active(FAULT_ID_MOTOR_SOFTWARE_OVERCURRENT) ||
                 faults_is_active(FAULT_ID_MOTOR_BAD_HALL_SEQUENCE) ||
                 faults_is_active(FAULT_ID_MOTOR_WD_RESET) ||
                 faults_is_active(FAULT_ID_MOTOR_CONFIG_READ) ||
                 faults_is_active(FAULT_ID_MOTOR_DESATURATION) ||
                 faults_is_active(FAULT_ID_MOTOR_OVERSPEED))
            vcu_fault = VCU_STATUS_VCU_FAULT_MOTOR_CONTROLLER_FAULT;
        buf[0] = vcu_fault;

        

        
        // Byte 1: status bits per DBC positions 8-14
        bool pedals_ok = !g_data_read->accel_brake.AccelPedal_Main_Fault &&
                         !g_data_read->accel_brake.AccelPedal_Redundant_Fault &&
                         !g_data_read->accel_brake.BrakePedal_Main_Fault &&
                         !g_data_read->accel_brake.BrakePedal_Redundant_Fault;

        bool driver_input_ok = !faults_is_active(FAULT_ID_GENERIC_WATCHDOG_FAULT) &&
                               !faults_is_active(FAULT_ID_CONTROLS_FAULT);

        bool steering_angle_ok = g_data_read->lws.LWS_Fault == 0;

    //          while(1){
    //             LED_toggle(HB);
    //     vTaskDelay(pdMS_TO_TICKS(100));

    // }
        buf[1] = ((uint8_t)contactor_get_sense(MOTOR_CONTACTOR) << 0) | // Motor_Contactor_State
                 ((uint8_t)contactor_get_sense(MOTOR_PRE_CONTACTOR) << 1) | // Motor_Precharge_Contactor_State
                 ((uint8_t) (contactor_get_sense(MOTOR_PRE_CONTACTOR) && vcu_fault == 0) << 2) | // Motor_Ready_To_Drive
                 ((uint8_t)driver_input_ok << 3) |    // VCU_Driver_Input_OK
                 ((uint8_t)pedals_ok << 4) |          // VCU_Pedals_OK
                 ((uint8_t)!!(fsm_get_inputs() & READY_TO_REGEN_BIT) << 5) | // VCU_Regen_OK
                 ((uint8_t)(current_state.stateName == REGEN) << 6) |     // VCU_Regen_Active
                 ((uint8_t)steering_angle_ok << 7); // VCU_Steering_Angle_OK (not implemented, set to OK)

        // Byte 2: VCU_FSM_State bits [3:0]
        buf[2] = (uint8_t)(current_state.stateName & 0x0FU);

        buf[3] = fsm_get_inputs() ; //this supposed to have fsm states as well but stateName covers it for now
   
    
        FDCAN_TxHeaderTypeDef tx_header = {0};   
        tx_header.Identifier = CAN_ID_VCU_STATUS;
        tx_header.IdType = FDCAN_STANDARD_ID;
        tx_header.TxFrameType = FDCAN_DATA_FRAME;
        tx_header.DataLength = FDCAN_DLC_BYTES(CAN_DLC_VCU_STATUS);
        tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
        tx_header.BitRateSwitch = FDCAN_BRS_OFF;
        tx_header.FDFormat = FDCAN_CLASSIC_CAN;
        tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        tx_header.MessageMarker = 0;

        CarCAN_Send(&tx_header, buf, sizeof(buf));

        LED_toggle(HB);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}