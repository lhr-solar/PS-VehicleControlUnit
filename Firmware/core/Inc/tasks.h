/**
 * @file tasks.h
 * @brief FreeRTOS task declarations for VCU firmware
 * @copyright Copyright (c) 2018-2026 UT Longhorn Racing Solar
 */

#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "fsm.h"
#include "faults.h"



#define TASK_INIT_STACK_SIZE            (configMINIMAL_STACK_SIZE)
#define TASK_FSM_STACK_SIZE             (2 * configMINIMAL_STACK_SIZE)
#define TASK_UPDATE_STATUS_STACK_SIZE   (2 * configMINIMAL_STACK_SIZE)
#define TASK_BROADCAST_STACK_SIZE       (2 * configMINIMAL_STACK_SIZE)
#define TASK_FAULT_STACK_SIZE           (configMINIMAL_STACK_SIZE)

// SHould go Fault > FSM > UpdateStatus > Broadcast > Init
#define TASK_FAULT_PRIORITY          (tskIDLE_PRIORITY + 5)
#define TASK_FSM_PRIORITY            (tskIDLE_PRIORITY + 4)
#define TASK_UPDATE_STATUS_PRIORITY  (tskIDLE_PRIORITY + 3)
#define TASK_BROADCAST_PRIORITY      (tskIDLE_PRIORITY + 2)
#define TASK_INIT_PRIORITY           (tskIDLE_PRIORITY + 1)


extern TaskHandle_t Task_Init_Handle;
extern TaskHandle_t Task_FSM_Handle;
extern TaskHandle_t Task_UpdateControlStatus_Handle;
extern TaskHandle_t Task_BroadcastMotorStatus_Handle;
extern TaskHandle_t Task_FaultHandler_Handle;
