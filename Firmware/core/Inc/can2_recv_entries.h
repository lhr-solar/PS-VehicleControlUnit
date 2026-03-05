#include "CarCAN_can_msgs.h"
#include <stdbool.h>

//Only need the size to be 1 for latest, but may want to increase this for logging purposes
//Makefile needs to point to this for FSM entries


//CAR CAN

CAN_RECV_ENTRY(CAN_ID_VCU_STATUS,                    1, true);
CAN_RECV_ENTRY(CAN_ID_CONTROLS_STATUS,               1, true);
CAN_RECV_ENTRY(CAN_ID_VCU_PRECHARGE_VOLTAGES,        1, true);
CAN_RECV_ENTRY(CAN_ID_ACCEL_BRAKE_POSITION,          1, true);
CAN_RECV_ENTRY(CAN_ID_ACCEL_BRAKE_POSITION_VOLTAGE,  1, true);
CAN_RECV_ENTRY(CAN_ID_DRIVER_INPUT_STATUS,           1, true);
CAN_RECV_ENTRY(CAN_ID_LWS_STANDARD,                  1, true);
CAN_RECV_ENTRY(CAN_ID_LV_CARRIER_STATUS,             1, true);
CAN_RECV_ENTRY(CAN_ID_BRAKE_PRESSURE,                1, true);
CAN_RECV_ENTRY(CAN_ID_LWS_CONFIG,                    1, true);
