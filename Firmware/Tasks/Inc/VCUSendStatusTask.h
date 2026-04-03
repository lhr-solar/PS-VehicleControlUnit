#pragma once

#include "CarCAN_can_msgs.h"
#include "CANbus.h"
#include "CAN_FD.h"
#include <string.h>
#include "FaultHandlerTask.h"

void VCUSendStatusTask_Init(void);

void Task_VCUSendStatus();