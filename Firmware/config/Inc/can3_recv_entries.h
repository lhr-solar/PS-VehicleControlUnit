#include "CarCAN_can_msgs.h"
// CAN_RECV_ENTRY(ID, SIZE, CIRCULAR)
// carcan

#include "CarCAN_can_msgs.h"
#include "SteeringCAN_can_msgs.h"
#include "BPSCAN_can_msgs.h"
#include "DebugConfig.h"

// CAN_RECV_ENTRY(0x123, 8, true) // testing only, comment later
CAN_RECV_ENTRY(CAN_ID_BPS_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_PEDAL_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_LV_CARRIER_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_LWS_STANDARD, 8, true)
CAN_RECV_ENTRY(CAN_ID_DRIVER_INPUT_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_BPS_PRECHARGE_VOLTAGES, 8, true)
CAN_RECV_ENTRY(CAN_ID_CONTROLS_STATUS, 8, true)
CAN_RECV_ENTRY(CAN_ID_BRAKE_PRESSURE_1, 8, true)
CAN_RECV_ENTRY(CAN_ID_BRAKE_PRESSURE_2, 8, true)

#if FSM_DEBUG_BUILD
// Bench-test: route motor controller telemetry/commands over the CarCAN
// bus instead of the (possibly unwired) dedicated MotorCAN bus.
CAN_RECV_ENTRY(CAN_ID_MC_INFO, 2, true)
CAN_RECV_ENTRY(CAN_ID_MC_STATUS, 2, true)
CAN_RECV_ENTRY(CAN_ID_MC_BUSMEASUREMENT, 2, true)
CAN_RECV_ENTRY(CAN_ID_MC_VELOCITYMEASUREMENT, 2, true)
CAN_RECV_ENTRY(CAN_ID_SET_MOTOR_CMD_SRC, 2, true)
#endif