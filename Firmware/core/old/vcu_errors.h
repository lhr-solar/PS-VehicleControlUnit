#ifndef VCU_ERRORS_H
#define VCU_ERRORS_H

typedef enum {
    ERR_NONE                        = 0,
    // Motor controller faults 
    ERR_MOT_HARDWARE_OC             = (1 << 0),
    ERR_MOT_SOFTWARE_OC             = (1 << 1),
    ERR_MOT_DC_BUS_OV               = (1 << 2),
    ERR_MOT_HALL_SENSOR             = (1 << 3),
    ERR_MOT_WDOG_LAST_RESET         = (1 << 4),
    ERR_MOT_CONFIG_READ             = (1 << 5),    
    ERR_MOT_UNDERVOLT_LOCKOUT       = (1 << 6),
    ERR_MOT_DESAT_FAULT             = (1 << 7),
    ERR_MOT_MOTOR_OVERSPEED         = (1 << 8),
    ERR_PEDALS_WD_TRIP              = (1 << 9),
    ERR_PEDALS_FAULT                = (1 << 10),
    ERR_CTRLS_LDR_WD_TRIP           = (1 << 11),
    ERR_CTRLS_LDR_FAULT             = (1 << 12),
    ERR_BPS_WD_TRIP                 = (1 << 13),
    ERR_BPS_FAULT                   = (1 << 14),
    ERR_

    FAULT_MOTOR_ERROR               = (1 << 1),
    FAULT_PEDALS_MISSING            = (1 << 2),
    FAULT_DASHBOARD_MISSING         = (1 << 3),
    FAULT_STEERING_MISSING          = (1 << 4),
    FAULT_BPS_FAULT                 = (1 << 5),
    FAULT_MOTOR_CONTROLLER_FAULT    = (1 << 6),
    FAULT_SW_OVERCURRENT            = (1 << 7),
    FAULT_BPS_WATCHDOG_EXP          = (1 << 8),
    FAULT_CTL_LDR_WATCHDOG_EXP      = (1 << 9),
    FAULT__WATCHDOG_EXP      = (1 << 9),
} fault_bits_t;

typedef struct {
    FaultBits active_faults;
    FaultBits recoverable_faults;
    bool scheduler_locked;
    TaskHandle_t task_handle;
} ErrorManager;

#endif /* vcu_errors.h */