#pragma once

#include <stdint.h>

/* ================= CAN ID Macros ================= */

#define CAN_ID_LWS_STANDARD 0x2B0
#define CAN_ID_LWS_CONFIG 0x7C0

/* ================= CAN Length Macros ================= */

#define CAN_DLC_LWS_STANDARD 5
#define CAN_DLC_LWS_CONFIG 2


/* ================= Value Table Enums ================= */

typedef enum {
    LWS_STANDARD_LWS_FAULT_OK = 0,
    LWS_STANDARD_LWS_FAULT_INTERNAL_FAULT = 1,
} lws_standard_lws_fault_e;

typedef enum {
    LWS_STANDARD_LWS_CALIBRATIONSTAUS_SENSOR_NOT_CALIBRATED = 0,
    LWS_STANDARD_LWS_CALIBRATIONSTAUS_SENSOR_CALIBRATED = 1,
} lws_standard_lws_calibrationstaus_e;

typedef enum {
    LWS_STANDARD_LWS_TRIMMING_STATUS_FAULT_SENSOR_NOT_TRIMMED = 0,
    LWS_STANDARD_LWS_TRIMMING_STATUS_SENSOR_TRIMMED = 1,
} lws_standard_lws_trimming_status_e;

typedef enum {
    LWS_CONFIG_LWS_CCW_SETS_THE_SIGNAL_LWS_ANGLE_TO_0Ã_Â_ = 3,
    LWS_CONFIG_LWS_CCW_RESETS_CALIBRATION_STATUS = 5,
} lws_config_lws_ccw_e;

/* ================= Message Structs ================= */

typedef struct {
    int16_t LWS_Angle;
    uint8_t LWS_Speed;
    uint8_t LWS_Fault;
    uint8_t LWS_CalibrationStaus;
    uint8_t LWS_Trimming_Status;
    uint8_t LWS_SF1_5;
    uint8_t LWS_MSG_CNT;
    uint8_t LWS_CHK_SUM;
} lws_standard_t;

typedef struct {
    uint8_t LWS_CCW;
    uint16_t LWS_RES;
} lws_config_t;

