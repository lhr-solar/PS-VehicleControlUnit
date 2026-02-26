//Only need the size to be 1 for latest, but may want to increase this for logging purposes
//Makefile needs to point to this for FSM entries


//CAR CAN

#define BPS_STATUS_CAN_ID 0x1
#define VCU_STATUS_CAN_ID 0x10
#define ACCEL_BRAKE_POSITION_CAN_ID 0x50
#define DRIVER_INPUT_STATUS_CAN_ID 0x60

FSM_RECV_ENTRY(BPS_STATUS,          BPS_STATUS_CAN_ID)
FSM_RECV_ENTRY(VCU_STATUS,          VCU_STATUS_CAN_ID)
FSM_RECV_ENTRY(ACCEL_BRAKE_POS,     ACCEL_BRAKE_POSITION_CAN_ID)
FSM_RECV_ENTRY(DRIVER_INPUT_STATUS, DRIVER_INPUT_STATUS_CAN_ID)