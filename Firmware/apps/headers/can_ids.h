// CAN IDS I care about
#ifndef __CAN_IDS_H
#define __CAN_IDS_H

#define CAN1
#define CAN2

//This is for defining CAN IDs to be received/sent
#define CAN_ID_PEDALS               0x67
#define CAN_ID_GEARS                0x67
#define CAN_ID_REGEN_BUTTON         0x67
#define CAN_ID_REGEN_ENABLED        0x67
#define CAN_ID_CRUISE_CONTROL       0x67
#define CAN_ID_BPS_OK_TO_REGEN      0x67
#define CAN_ID_BPS_TRIP             0x67
#define CAN_ID_IGNITION_STATE       0x67

#define SEND_TRITIUM_IDS 8

//Add more defines for other tasks as needed, this is so it works with preprocessing

#endif