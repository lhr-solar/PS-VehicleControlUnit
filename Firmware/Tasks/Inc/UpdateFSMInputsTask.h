#pragma once

#include "inits.h"
#include "CANbus.h"
#include "FSM.h"

#define BRAKE_THRESH      42.0f  // percent
#define BRAKE_THRESH_HYST 30.0f  // percent

void Task_UpdateFSMInputs(void *args __attribute__((unused)));
