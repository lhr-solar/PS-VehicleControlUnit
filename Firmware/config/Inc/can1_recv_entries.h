#include "CarCAN_can_msgs.h"
// CAN_RECV_ENTRY(ID, SIZE, CIRCULAR)
// motorcan
#include "MotorCAN_can_msgs.h"

CAN_RECV_ENTRY(0x123, 8, true)  // testing only comment later
CAN_RECV_ENTRY(CAN_ID_MC_INFO, 2, true)
CAN_RECV_ENTRY(CAN_ID_MC_STATUS, 2, true)
CAN_RECV_ENTRY(CAN_ID_MC_BUSMEASUREMENT, 2, true)
CAN_RECV_ENTRY(CAN_ID_MC_VELOCITYMEASUREMENT, 2, true)
