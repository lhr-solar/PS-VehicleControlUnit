//Only need the size to be 1 for latest, but may want to increase this for logging purposes
//Makefile needs to point to this for FSM entries

//Motor CAN Receive

//Will have to hard code the values for now....
#define MOCO_BASE 0x221
#define MOTOR_IDENTITY        0x221
#define MOTOR_ERROR_STATUS    0x222
#define BUS_STATUS            0x223
#define MOTOR_VELOCITY        0x224
#define MOTOR_PHASE_CURRENT   0x225
#define MOTOR_VOLTAGE_VECTOR  0x226
#define MOTOR_CURRENT_VECTOR  0x227
#define MOTOR_BACK_EMF        0x228
#define RAIL_15V_STATUS       0x229
#define RAIL_3V3_1V9_STATUS   0x22A
#define HEATSINK_MOTOR_TEMP   0x22C
#define DSP_BOARD_TEMP        0x22D
#define ODOMETER_BUS_AH       0x22F
#define SLIP_SPEED            0x238

CAN_RECV_ENTRY(MOTOR_IDENTITY, 1, true)
CAN_RECV_ENTRY(MOTOR_ERROR_STATUS, 1, true)
CAN_RECV_ENTRY(BUS_STATUS, 1, true)
CAN_RECV_ENTRY(MOTOR_VELOCITY, 1, true)
CAN_RECV_ENTRY(MOTOR_PHASE_CURRENT, 1, true)
CAN_RECV_ENTRY(MOTOR_VOLTAGE_VECTOR, 1, true)
CAN_RECV_ENTRY(MOTOR_CURRENT_VECTOR, 1, true)
CAN_RECV_ENTRY(MOTOR_BACK_EMF, 1, true)
CAN_RECV_ENTRY(RAIL_15V_STATUS, 1, true)
CAN_RECV_ENTRY(RAIL_3V3_1V9_STATUS, 1, true)
CAN_RECV_ENTRY(HEATSINK_MOTOR_TEMP, 1, true)
CAN_RECV_ENTRY(DSP_BOARD_TEMP, 1, true)
CAN_RECV_ENTRY(ODOMETER_BUS_AH, 1, true)
CAN_RECV_ENTRY(SLIP_SPEED, 1, true)
