#include "UpdateFSMInputsTask.h"
#include "event_groups.h"

static void rebuild_inputs(void) {
    EventBits_t s = 0;

    if (driver_input.Gear_Forward)
        s |= FORWARD_BIT;
    else if (driver_input.Gear_Reverse)
        s |= REVERSE_BIT;
    else
        s |= NEUTRAL_BIT;

    if (driver_input.Regen_Activate) s |= REGEN_BUTTON_BIT;
    if (driver_input.Regen_Enable) s |= REGEN_ENABLED_BIT;
    if (driver_input.Cruise_Enable) s |= CRUISE_CONTROL_BUTTON_BIT;
    if (bps_status.BPS_Regen_OK) s |= READY_TO_REGEN_BIT;
    if (precharge_complete) s |= PRECHARGE_COMPLETE_BIT;

    

    if (brake_pedal_pct >= brake_threshold) {
        s |= BRAKE_BIT;
        brake_threshold = BRAKE_THRESH_HYST;
    } else {
        brake_threshold = BRAKE_THRESH;
    }

    fsm_set_all_inputs(s);
}

void Task_UpdateControlStatus(void *args __attribute__((unused))) {
    TickType_t last = xTaskGetTickCount();
    while (1) {
        // update from can
        FSMDataIn_t *update = g_data_write;
        MotorCAN_Recv_Status(&update->motor_status, 0);
        MotorCAN_Recv_Velocity(&update->motor_velocity, 0);
        CarCAN_Recv_BPS_Status(&update->bps_status, 0);
        CarCAN_Recv_Pedals_Position(&update->accel_brake, 0);
        CarCAN_Recv_Controls_Status(&update->controls_status, 0);
        CarCAN_Recv_Driver_Input(&update->driver_input, 0);
        CarCAN_Recv_LWS(&update->lws, 0);

        FSMDataIn_t *tmp;
        taskENTER_CRITICAL();
        tmp = g_data_read;
        g_data_read = g_data_write;
        g_data_write = tmp;
        taskEXIT_CRITICAL();

        rebuild_inputs(&);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(50));
    }
}

void Task_UpdateInputs(void *args)
{
    TickType_t last = xTaskGetTickCount();

    while (1)
    {
        fsm_inputs_t* w = (fsm_inputs_t*)write_ptr;

        // ---- CAN updates ----
        carcan_try_recv(CAN_ID_DRIVER_INPUT_STATUS, handle_driver_input, &w->driver_input);
        carcan_try_recv(CAN_ID_ACCEL_BRAKE_POSITION, handle_accel_brake, &w->accel_brake);
        carcan_try_recv(CAN_ID_LWS_STANDARD, handle_lws, &w->lws);
        carcan_try_recv(CAN_ID_CONTROLS_STATUS, handle_controls_status, &w->controls_status);
        carcan_try_recv(CAN_ID_BPS_STATUS, handle_bps, &w->bps_status);
        vcucan_try_recv(CAN_ID_MC_VELOCITYMEASUREMENT, handle_motor_velocity, &w->motor_velocity);
        vcucan_try_recv(CAN_ID_MC_STATUS, handle_motor_status, &w->motor_status);

        // ---- Build event bits (level-triggered) ----
        EventBits_t set = 0;
        EventBits_t clr = 0;

        // 🚗 Gear (mutually exclusive)
        clr |= (FORWARD_BIT | REVERSE_BIT | NEUTRAL_BIT);
        if (w->driver_input.Gear_Forward)      set |= FORWARD_BIT;
        else if (w->driver_input.Gear_Reverse) set |= REVERSE_BIT;
        else                                  set |= NEUTRAL_BIT;

        // 🎮 Buttons
        if (w->driver_input.Regen_Activate) set |= REGEN_BUTTON_BIT;
        else                                clr |= REGEN_BUTTON_BIT;

        if (w->driver_input.Regen_Enable)   set |= REGEN_ENABLED_BIT;
        else                                clr |= REGEN_ENABLED_BIT;

        if (w->driver_input.Cruise_Enable)  set |= CRUISE_CONTROL_BUTTON_BIT;
        else                                clr |= CRUISE_CONTROL_BUTTON_BIT;

        // 🔋 System
        if (w->bps_status.BPS_Regen_OK) set |= READY_TO_REGEN_BIT;
        else                            clr |= READY_TO_REGEN_BIT;

        if (precharge_complete) set |= PRECHARGE_COMPLETE_BIT;
        else                    clr |= PRECHARGE_COMPLETE_BIT;

        // 🦶 Brake (with hysteresis)
        float brake = w->accel_brake.Brake_Pos_Main;

        if (brake >= brake_threshold) {
            set |= BRAKE_BIT;
            brake_threshold = BRAKE_THRESH_HYST;
        } else {
            clr |= BRAKE_BIT;
            brake_threshold = BRAKE_THRESH;
        }

        // 🛡 Pedals OK
        bool pedals_ok =
            !w->accel_brake.Accel_Pos_Main_Fault &&
            !w->accel_brake.Accel_Pos_Redundant_Fault &&
            !w->accel_brake.Brake_Pos_Main_Fault &&
            !w->accel_brake.Brake_Pos_Redundant_Fault &&
            !w->accel_brake.Brake_Pressure_1_Fault &&
            !w->accel_brake.Brake_Pressure_2_Fault;

        if (pedals_ok) set |= PEDALS_OK_BIT;
        else           clr |= PEDALS_OK_BIT;

        // 🎛 Driver input OK
        bool driver_ok =
            !faults_is_active(FAULT_ID_CONTROLS_STATUS_WATCHDOG) &&
            !faults_is_active(FAULT_ID_CONTROLS_FAULT);

        if (driver_ok) set |= DRIVER_INPUT_OK_BIT;
        else           clr |= DRIVER_INPUT_OK_BIT;

        // ⚠️ Faults
        if (faults_any_active()) set |= FAULT_ACTIVE_BIT;
        else                     clr |= FAULT_ACTIVE_BIT;

        // ---- Apply bits atomically-ish ----
        xEventGroupClearBits(fsm_event_group, clr);
        xEventGroupSetBits(fsm_event_group, set);

        // ---- Swap buffers ----
        taskENTER_CRITICAL();
        fsm_inputs_t* tmp = (fsm_inputs_t*)read_ptr;
        read_ptr = write_ptr;
        write_ptr = tmp;
        taskEXIT_CRITICAL();

        vTaskDelayUntil(&last, pdMS_TO_TICKS(50));
    }
}