#include "CarCAN_can_msgs.h"
// CAN_RECV_ENTRY(ID, SIZE, CIRCULAR)
// carcan

#include "CarCAN_can_msgs.h"
#include "SteeringCAN_can_msgs.h"
#include "BPSCAN_can_msgs.h"

CAN_RECV_ENTRY(0x123, 8, true) // testing only, comment later
CAN_RECV_ENTRY(CAN_ID_BPS_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_PEDAL_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_LV_CARRIER_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_LWS_STANDARD, 8, true)
CAN_RECV_ENTRY(CAN_ID_DRIVER_INPUT_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_BPS_PRECHARGE_VOLTAGES, 8, true)
CAN_RECV_ENTRY(CAN_ID_CONTROLS_STATUS, 8, true)
