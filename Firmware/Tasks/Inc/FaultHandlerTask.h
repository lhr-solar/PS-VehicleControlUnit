#pragma once

#include "FaultBits.h"
#include "StatusLEDs.h"
#include "Contactors.h"
#include "task.h"
#include "InitTask.h"
#include "CANbus.h"
#include "CarCAN_can_msgs.h"

#define PRINTF_DELAY_MS 250

// enables the fdcan3 recieve hook, calls can_fd_rx_callback_hook everytime a can rx interrupt happens
#define FDCAN3_RECV_HOOK_EN
#define BPS_QUEUE_SIZE 32 // TODO: Placeholder

/** @brief Initialize the fault handler task. */
void Init_FaultHandlerTask();

/** @brief Run the fault handler task. */
void Task_FaultHandler();

/** @brief Kill the precharge task. */
void Kill_Precharge_Task();

void Fault_Loop();

void Set_Fault_LED();

extern EventBits_t fault_bits;
extern EventBits_t state_bits;