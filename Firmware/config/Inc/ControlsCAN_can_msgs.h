#pragma once

#include <stdint.h>

/* ================= CAN ID Macros ================= */

#define CAN_ID_LIGHTING_COMMAND 0x660
#define CAN_ID_LIGHTING_BOARD0_STATUS 0x670
#define CAN_ID_LIGHTING_BOARD1_STATUS 0x671
#define CAN_ID_LIGHTING_BOARD2_STATUS 0x672
#define CAN_ID_LIGHTING_BOARD3_STATUS 0x673
#define CAN_ID_LIGHTING_BOARD4_STATUS 0x674
#define CAN_ID_LIGHTING_BOARD5_STATUS 0x675
#define CAN_ID_LIGHTING_BOARD6_STATUS 0x676

/* ================= CAN Length Macros ================= */

#define CAN_DLC_LIGHTING_COMMAND 1
#define CAN_DLC_LIGHTING_BOARD0_STATUS 8
#define CAN_DLC_LIGHTING_BOARD1_STATUS 8
#define CAN_DLC_LIGHTING_BOARD2_STATUS 8
#define CAN_DLC_LIGHTING_BOARD3_STATUS 8
#define CAN_DLC_LIGHTING_BOARD4_STATUS 8
#define CAN_DLC_LIGHTING_BOARD5_STATUS 8
#define CAN_DLC_LIGHTING_BOARD6_STATUS 8


/* ================= Value Table Enums ================= */

typedef enum {
    LIGHTING_BOARD0_STATUS_LIGHTING_BOARD_FAULTS_CONTROLS_LEADER_WATCHDOG = 7,
    LIGHTING_BOARD0_STATUS_LIGHTING_BOARD_FAULTS_LED1_OVERCURRENT = 6,
    LIGHTING_BOARD0_STATUS_LIGHTING_BOARD_FAULTS_LED0_OVERCURRENT = 5,
    LIGHTING_BOARD0_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_OVERCURRENT = 4,
    LIGHTING_BOARD0_STATUS_LIGHTING_BOARD_FAULTS_LED1_UNDERCURRENT = 3,
    LIGHTING_BOARD0_STATUS_LIGHTING_BOARD_FAULTS_LED0_UNDERCURRENT = 2,
    LIGHTING_BOARD0_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_UNDERCURRENT = 1,
    LIGHTING_BOARD0_STATUS_LIGHTING_BOARD_FAULTS_NONE = 0,
} lighting_board0_status_lighting_board_faults_e;

typedef enum {
    LIGHTING_BOARD1_STATUS_LIGHTING_BOARD_FAULTS_CONTROLS_LEADER_WATCHDOG = 7,
    LIGHTING_BOARD1_STATUS_LIGHTING_BOARD_FAULTS_LED1_OVERCURRENT = 6,
    LIGHTING_BOARD1_STATUS_LIGHTING_BOARD_FAULTS_LED0_OVERCURRENT = 5,
    LIGHTING_BOARD1_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_OVERCURRENT = 4,
    LIGHTING_BOARD1_STATUS_LIGHTING_BOARD_FAULTS_LED1_UNDERCURRENT = 3,
    LIGHTING_BOARD1_STATUS_LIGHTING_BOARD_FAULTS_LED0_UNDERCURRENT = 2,
    LIGHTING_BOARD1_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_UNDERCURRENT = 1,
    LIGHTING_BOARD1_STATUS_LIGHTING_BOARD_FAULTS_NONE = 0,
} lighting_board1_status_lighting_board_faults_e;

typedef enum {
    LIGHTING_BOARD2_STATUS_LIGHTING_BOARD_FAULTS_CONTROLS_LEADER_WATCHDOG = 7,
    LIGHTING_BOARD2_STATUS_LIGHTING_BOARD_FAULTS_LED1_OVERCURRENT = 6,
    LIGHTING_BOARD2_STATUS_LIGHTING_BOARD_FAULTS_LED0_OVERCURRENT = 5,
    LIGHTING_BOARD2_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_OVERCURRENT = 4,
    LIGHTING_BOARD2_STATUS_LIGHTING_BOARD_FAULTS_LED1_UNDERCURRENT = 3,
    LIGHTING_BOARD2_STATUS_LIGHTING_BOARD_FAULTS_LED0_UNDERCURRENT = 2,
    LIGHTING_BOARD2_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_UNDERCURRENT = 1,
    LIGHTING_BOARD2_STATUS_LIGHTING_BOARD_FAULTS_NONE = 0,
} lighting_board2_status_lighting_board_faults_e;

typedef enum {
    LIGHTING_BOARD3_STATUS_LIGHTING_BOARD_FAULTS_CONTROLS_LEADER_WATCHDOG = 7,
    LIGHTING_BOARD3_STATUS_LIGHTING_BOARD_FAULTS_LED1_OVERCURRENT = 6,
    LIGHTING_BOARD3_STATUS_LIGHTING_BOARD_FAULTS_LED0_OVERCURRENT = 5,
    LIGHTING_BOARD3_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_OVERCURRENT = 4,
    LIGHTING_BOARD3_STATUS_LIGHTING_BOARD_FAULTS_LED1_UNDERCURRENT = 3,
    LIGHTING_BOARD3_STATUS_LIGHTING_BOARD_FAULTS_LED0_UNDERCURRENT = 2,
    LIGHTING_BOARD3_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_UNDERCURRENT = 1,
    LIGHTING_BOARD3_STATUS_LIGHTING_BOARD_FAULTS_NONE = 0,
} lighting_board3_status_lighting_board_faults_e;

typedef enum {
    LIGHTING_BOARD4_STATUS_LIGHTING_BOARD_FAULTS_CONTROLS_LEADER_WATCHDOG = 7,
    LIGHTING_BOARD4_STATUS_LIGHTING_BOARD_FAULTS_LED1_OVERCURRENT = 6,
    LIGHTING_BOARD4_STATUS_LIGHTING_BOARD_FAULTS_LED0_OVERCURRENT = 5,
    LIGHTING_BOARD4_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_OVERCURRENT = 4,
    LIGHTING_BOARD4_STATUS_LIGHTING_BOARD_FAULTS_LED1_UNDERCURRENT = 3,
    LIGHTING_BOARD4_STATUS_LIGHTING_BOARD_FAULTS_LED0_UNDERCURRENT = 2,
    LIGHTING_BOARD4_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_UNDERCURRENT = 1,
    LIGHTING_BOARD4_STATUS_LIGHTING_BOARD_FAULTS_NONE = 0,
} lighting_board4_status_lighting_board_faults_e;

typedef enum {
    LIGHTING_BOARD5_STATUS_LIGHTING_BOARD_FAULTS_CONTROLS_LEADER_WATCHDOG = 7,
    LIGHTING_BOARD5_STATUS_LIGHTING_BOARD_FAULTS_LED1_OVERCURRENT = 6,
    LIGHTING_BOARD5_STATUS_LIGHTING_BOARD_FAULTS_LED0_OVERCURRENT = 5,
    LIGHTING_BOARD5_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_OVERCURRENT = 4,
    LIGHTING_BOARD5_STATUS_LIGHTING_BOARD_FAULTS_LED1_UNDERCURRENT = 3,
    LIGHTING_BOARD5_STATUS_LIGHTING_BOARD_FAULTS_LED0_UNDERCURRENT = 2,
    LIGHTING_BOARD5_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_UNDERCURRENT = 1,
    LIGHTING_BOARD5_STATUS_LIGHTING_BOARD_FAULTS_NONE = 0,
} lighting_board5_status_lighting_board_faults_e;

typedef enum {
    LIGHTING_BOARD6_STATUS_LIGHTING_BOARD_FAULTS_CONTROLS_LEADER_WATCHDOG = 7,
    LIGHTING_BOARD6_STATUS_LIGHTING_BOARD_FAULTS_LED1_OVERCURRENT = 6,
    LIGHTING_BOARD6_STATUS_LIGHTING_BOARD_FAULTS_LED0_OVERCURRENT = 5,
    LIGHTING_BOARD6_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_OVERCURRENT = 4,
    LIGHTING_BOARD6_STATUS_LIGHTING_BOARD_FAULTS_LED1_UNDERCURRENT = 3,
    LIGHTING_BOARD6_STATUS_LIGHTING_BOARD_FAULTS_LED0_UNDERCURRENT = 2,
    LIGHTING_BOARD6_STATUS_LIGHTING_BOARD_FAULTS_ADDR_LED_UNDERCURRENT = 1,
    LIGHTING_BOARD6_STATUS_LIGHTING_BOARD_FAULTS_NONE = 0,
} lighting_board6_status_lighting_board_faults_e;

/* ================= Message Structs ================= */

typedef struct {
    uint8_t Headlights_On;
    uint8_t Left_Indicator_On;
    uint8_t Right_Indicator_On;
    uint8_t Blinker_Sync;
    uint8_t Brakelights_On;
    uint8_t BPS_Strobe_On;
    uint8_t Custom_Mode0_On;
    uint8_t Custom_Mode1_On;
} lighting_command_t;

typedef struct {
    uint8_t Lighting_Board_Faults;
    uint8_t Light_Headlight_Active;
    uint8_t Light_LTurn_Active;
    uint8_t Light_RTurn_Active;
    uint8_t Light_BPS_Strobe_Active;
    uint8_t Light_Brakelight_Active;
    uint8_t Light_Custom0_Active;
    uint8_t Light_Custom1_Active;
    uint8_t Light_Custom2_Active;
    uint16_t Addr_LED_Current;
    uint16_t LED0_Current;
    uint16_t LED1_Current;
} lighting_board0_status_t;

typedef struct {
    uint8_t Lighting_Board_Faults;
    uint8_t Light_Headlight_Active;
    uint8_t Light_LTurn_Active;
    uint8_t Light_RTurn_Active;
    uint8_t Light_BPS_Strobe_Active;
    uint8_t Light_Brakelight_Active;
    uint8_t Light_Custom0_Active;
    uint8_t Light_Custom1_Active;
    uint8_t Light_Custom2_Active;
    uint16_t Addr_LED_Current;
    uint16_t LED0_Current;
    uint16_t LED1_Current;
} lighting_board1_status_t;

typedef struct {
    uint8_t Lighting_Board_Faults;
    uint8_t Light_Headlight_Active;
    uint8_t Light_LTurn_Active;
    uint8_t Light_RTurn_Active;
    uint8_t Light_BPS_Strobe_Active;
    uint8_t Light_Brakelight_Active;
    uint8_t Light_Custom0_Active;
    uint8_t Light_Custom1_Active;
    uint8_t Light_Custom2_Active;
    uint16_t Addr_LED_Current;
    uint16_t LED0_Current;
    uint16_t LED1_Current;
} lighting_board2_status_t;

typedef struct {
    uint8_t Lighting_Board_Faults;
    uint8_t Light_Headlight_Active;
    uint8_t Light_LTurn_Active;
    uint8_t Light_RTurn_Active;
    uint8_t Light_BPS_Strobe_Active;
    uint8_t Light_Brakelight_Active;
    uint8_t Light_Custom0_Active;
    uint8_t Light_Custom1_Active;
    uint8_t Light_Custom2_Active;
    uint16_t Addr_LED_Current;
    uint16_t LED0_Current;
    uint16_t LED1_Current;
} lighting_board3_status_t;

typedef struct {
    uint8_t Lighting_Board_Faults;
    uint8_t Light_Headlight_Active;
    uint8_t Light_LTurn_Active;
    uint8_t Light_RTurn_Active;
    uint8_t Light_BPS_Strobe_Active;
    uint8_t Light_Brakelight_Active;
    uint8_t Light_Custom0_Active;
    uint8_t Light_Custom1_Active;
    uint8_t Light_Custom2_Active;
    uint16_t Addr_LED_Current;
    uint16_t LED0_Current;
    uint16_t LED1_Current;
} lighting_board4_status_t;

typedef struct {
    uint8_t Lighting_Board_Faults;
    uint8_t Light_Headlight_Active;
    uint8_t Light_LTurn_Active;
    uint8_t Light_RTurn_Active;
    uint8_t Light_BPS_Strobe_Active;
    uint8_t Light_Brakelight_Active;
    uint8_t Light_Custom0_Active;
    uint8_t Light_Custom1_Active;
    uint8_t Light_Custom2_Active;
    uint16_t Addr_LED_Current;
    uint16_t LED0_Current;
    uint16_t LED1_Current;
} lighting_board5_status_t;

typedef struct {
    uint8_t Lighting_Board_Faults;
    uint8_t Light_Headlight_Active;
    uint8_t Light_LTurn_Active;
    uint8_t Light_RTurn_Active;
    uint8_t Light_BPS_Strobe_Active;
    uint8_t Light_Brakelight_Active;
    uint8_t Light_Custom0_Active;
    uint8_t Light_Custom1_Active;
    uint8_t Light_Custom2_Active;
    uint16_t Addr_LED_Current;
    uint16_t LED0_Current;
    uint16_t LED1_Current;
} lighting_board6_status_t;

