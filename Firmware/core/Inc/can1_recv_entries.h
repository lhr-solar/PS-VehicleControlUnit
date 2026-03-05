#include "MotorCAN_can_msgs.h"
#include <stdbool.h>


//Only need the size to be 1 for latest, but may want to increase this for logging purposes
//Makefile needs to point to this for FSM entries

//Motor CAN Receive
CAN_RECV_ENTRY(CAN_ID_MC_DRIVECOMMAND,                  1, true);
CAN_RECV_ENTRY(CAN_ID_MC_POWERCOMMAND,                  1, true);
CAN_RECV_ENTRY(CAN_ID_MC_RESETCOMMAND,                  1, true);
CAN_RECV_ENTRY(CAN_ID_MC_INFO,                          1, true);
CAN_RECV_ENTRY(CAN_ID_MC_STATUS,                        1, true);
CAN_RECV_ENTRY(CAN_ID_MC_BUSMEASUREMENT,                1, true);
CAN_RECV_ENTRY(CAN_ID_MC_VELOCITYMEASUREMENT,           1, true);
CAN_RECV_ENTRY(CAN_ID_MC_PHASECURRENTMEASUREMENT,       1, true);
CAN_RECV_ENTRY(CAN_ID_MC_MOTORVOLTAGEVECTORMEASUREMENT, 1, true);
CAN_RECV_ENTRY(CAN_ID_MC_MOTORCURRENTVECTORMEASUREMENT, 1, true);
CAN_RECV_ENTRY(CAN_ID_MC_BACKEMFMEASUREMENTPREDICTION,  1, true);
CAN_RECV_ENTRY(CAN_ID_MC_15VRAILMEASUREMENT,            1, true);
CAN_RECV_ENTRY(CAN_ID_MC_3V319VRAILMEASUREMENT,         1, true);
CAN_RECV_ENTRY(CAN_ID_MC_MOTOR_TEMPMEASUREMENT,         1, true);
CAN_RECV_ENTRY(CAN_ID_MC_DSPBOARDTEMPMEASUREMENT,       1, true);
CAN_RECV_ENTRY(CAN_ID_MC_ODOMETERBUSAHMEASUREMENT,      1, true);
CAN_RECV_ENTRY(CAN_ID_MC_SLIPSPEEDMEASUREMENT,          1, true);