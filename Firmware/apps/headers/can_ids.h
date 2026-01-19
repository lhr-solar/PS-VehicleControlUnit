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

#define CAN_ID_MOTOR_DRIVE 0x221
#define CAN_ID_MOTOR_POWER 0x222
#define CAN_ID_MOTOR_RESET 0x223
#define CAN_ID_MOTOR_IDENTIFICATION 0x240
#define CAN_ID_MOTOR_STATUS 0x241

#define MOTOR_SEND_IDS 5



//Add more defines for other tasks as needed, this is so it works with preprocessing

#endif