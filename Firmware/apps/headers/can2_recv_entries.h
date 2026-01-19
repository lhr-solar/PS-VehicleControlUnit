//Only need the size to be 1 for latest, but may want to increase this for logging purposes
//Makefile needs to point to this for FSM entries


//CAR CAN

CAN_RECV_ENTRY(CAN_ID_PEDALS, 1, true)
CAN_RECV_ENTRY(CAN_ID_GEARS, 1, true)
CAN_RECV_ENTRY(CAN_ID_REGEN_BUTTON, 1, true)
CAN_RECV_ENTRY(CAN_ID_REGEN_ENABLED, 1, true)
CAN_RECV_ENTRY(CAN_ID_CRUISE_CONTROL, 1, true)
CAN_RECV_ENTRY(CAN_ID_BPS_OK_TO_REGEN, 1, true)
CAN_RECV_ENTRY(CAN_ID_BPS_TRIP, 1, true)
CAN_RECV_ENTRY(CAN_ID_IGNITION_STATE, 1, true)
