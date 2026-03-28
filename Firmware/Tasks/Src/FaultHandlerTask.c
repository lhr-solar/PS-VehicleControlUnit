// #include "FaultHandlerTask.h"
// #include "PrechargeTask.h" // for hprecharge_task handle

// #define FAULT_LOOP_PRINTF_DELAY_MS 10000

// #define FAULT_PRINTF_COUNTER       (FAULT_LOOP_PRINTF_DELAY_MS / FAULT_LOOP_PERIOD_MS)

// EventBits_t fault_bits = 0;

// void Init_FaultHandlerTask() {
//     if (!faults_init()) {
//         // Fault bit initialization failed
//         Error_Handler();
//     }
// }

// void Kill_Precharge_Task() {
//     if (hprecharge_task != NULL) {
//         vTaskDelete(hprecharge_task);
//     }
// }

// static void print_fault() {
//     switch (fault_bits) // compare against individual bitmasks
//     {
//         case FAULT_BIT(FAULT_ID_MOTOR_GT_BATTERY):
//             printf("Fault: Motor Voltage Greater Than Battery Voltage\r\n");
//             break;
//         case FAULT_BIT(FAULT_ID_BATTERY_OVERVOLTAGE):
//             printf("Fault: Battery Overvoltage\r\n");
//             break;
//         case FAULT_BIT(FAULT_ID_BATTERY_UNDERVOLTAGE):
//             printf("Fault: Battery Undervoltage\r\n");
//             break;
//         case FAULT_BIT(FAULT_ID_MOTOR_SENSE_TIMEOUT):
//             printf("Fault: Motor Sense Timeout\r\n");
//             break;
//         case FAULT_BIT(FAULT_ID_PRECHARGE_SENSE_TIMEOUT):
//             printf("Fault: Precharge Sense Timeout\r\n");
//             break;
//         case FAULT_BIT(FAULT_ID_PRECHARGE_TIMEOUT):
//             printf("Fault: Precharge Sequence Timeout\r\n");
//             break;
//         case FAULT_BIT(FAULT_ID_CONTACTOR_CALLBACK):
//             printf("Fault: Contactor Sense Fault\r\n");
//             break;
//         case FAULT_BIT(FAULT_ID_MOTOR_SENSE_MISMATCH):
//             printf("Fault: Motor Sense Mismatch\r\n");
//             break;
//         case FAULT_BIT(FAULT_ID_PRECHARGE_SENSE_MISMATCH):
//             printf("Fault: Precharge Sense Mismatch\r\n");
//             break;
//         default:
//             printf("Fault: Unknown\r\n");
//             break;
//     }
// }

// void Fault_Loop() {
//     uint32_t fault_printf_debug_counter = 0;
//     while (1) {
//         fault_printf_debug_counter++;

//         if (fault_printf_debug_counter >= FAULT_PRINTF_COUNTER) {
//             print_fault();
//             fault_printf_debug_counter = 0;
//         }

//         LED_toggle(HB);
//         vTaskDelay(FAULT_LOOP_PERIOD_MS);
//     }
// }

// void Set_Fault_LED() {
//     switch (fault_bits) // compare against individual bitmasks
//     {
//         case FAULT_BIT(FAULT_ID_MOTOR_GT_BATTERY):
//             LED_set(CAR_BPSFAULT, LED_ON);
//             break;
//         case FAULT_BIT(FAULT_ID_BATTERY_OVERVOLTAGE):
//             LED_set(CAR_BPSFAULT, LED_ON);
//             break;
//         case FAULT_BIT(FAULT_ID_BATTERY_UNDERVOLTAGE):
//             LED_set(CAR_BPSFAULT, LED_ON);
//             break;
//         case FAULT_BIT(FAULT_ID_MOTOR_SENSE_TIMEOUT):
//             LED_set(MOTOR_SENSE_TIMEOUT, LED_ON);
//             break;
//         case FAULT_BIT(FAULT_ID_PRECHARGE_SENSE_TIMEOUT):
//             LED_set(PRECHARGE_SENSE_TIMEOUT, LED_ON);
//             break;
//         case FAULT_BIT(FAULT_ID_PRECHARGE_TIMEOUT):
//             LED_set(PRECHARGE_TIMEOUT, LED_ON);
//             break;
//         case FAULT_BIT(FAULT_ID_CONTACTOR_CALLBACK):
//             LED_set(CAR_BPSFAULT, LED_ON);
//             break;
//         case FAULT_BIT(FAULT_ID_MOTOR_SENSE_MISMATCH):
//             LED_set(CAR_BPSFAULT, LED_ON);
//             break;
//         case FAULT_BIT(FAULT_ID_PRECHARGE_SENSE_MISMATCH):
//             LED_set(CAR_BPSFAULT, LED_ON);
//             break;
//         default:
//             break;
//     }
// }

// void Task_FaultHandler() {
//     Init_FaultHandlerTask();

//     while (true) {
//         fault_bits = faults_wait(FAULT_ID_COUNT, portMAX_DELAY);

//         if (fault_bits != 0) {
//             Kill_Precharge_Task();
//             contactor_emergency_open_all();

//             // prevents the motor from running
//             clear_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);
//             clear_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);

//             printf("Fault Handler triggered with bitmask: 0x%02lX\r\n", fault_bits);

//             Set_Fault_LED();
//             Fault_Loop();
//         }

//         vTaskDelay(1000);
//     }
// }

#include "FaultHandlerTask.h"
#include "PrechargeTask.h"   // for hprecharge_task handle
#include "Watchdogs.h"

#define FAULT_LOOP_PRINTF_DELAY_MS 10000
#define FAULT_PRINTF_COUNTER       (FAULT_LOOP_PRINTF_DELAY_MS / FAULT_LOOP_PERIOD_MS)


static void FHT_kill_precharge_task(void) {
    if (hprecharge_task != NULL) {
        vTaskDelete(hprecharge_task);
        hprecharge_task = NULL;
    }
}

// Sets fault LEDs based on which faults are active. Multiple faults may be 
// active at once, so multiple LEDs may be set.
static void FHT_set_fault_leds(EventBits_t bits) {
    // TODO add a generic fault LED
    //LED_set(FAULT_LED, LED_ON);
    for (FaultID_e i = 0; i < FAULT_ID_COUNT; i++) {
        if (bits & FAULT_BIT(i)) {
            switch (i) {
                /* ============= MOTOR CONTROLLER ============ */
                case FAULT_ID_MOTOR_HARDWARE_OVERCURRENT:
                case FAULT_ID_MOTOR_DC_BUS_OVERVOLTAGE:
                case FAULT_ID_MOTOR_WD_RESET:
                case FAULT_ID_MOTOR_CONFIG_READ:
                case FAULT_ID_MOTOR_15V_UNDERVOLTAGE:
                case FAULT_ID_MOTOR_DESATURATION:
                case FAULT_ID_MOTOR_OVERSPEED:
                    LED_set(MOTOR_FAULT, LED_ON);
                    break;
                case FAULT_ID_MOTOR_SOFTWARE_OVERCURRENT:
                    LED_set(SWOC, LED_ON);
                    break;
                case FAULT_ID_MOTOR_BAD_HALL_SEQUENCE:
                    LED_set(HALL_EFFECT, LED_ON);
                    break;

                /* ================ PRECHARGE ================ */
                case FAULT_ID_PRECHARGE_TIMEOUT:
                case FAULT_ID_PRECHARGE_SENSE_TIMEOUT:
                case FAULT_ID_PRECHARGE_SENSE_MISMATCH:
                case FAULT_ID_MOTOR_SENSE_TIMEOUT:
                case FAULT_ID_MOTOR_SENSE_MISMATCH:
                case FAULT_ID_BATTERY_OVERVOLTAGE:
                case FAULT_ID_BATTERY_UNDERVOLTAGE:
                case FAULT_ID_MOTOR_GT_BATTERY:
                case FAULT_ID_CONTACTOR_CALLBACK:
                // TODO add a precharge fault led
                    break;

                /* ============== OTHER BOARDS =============== */
                case FAULT_ID_STEERING_SENSOR_FAULT:
                case FAULT_ID_PEDAL_BOARD_FAULT:
                case FAULT_ID_CONTROLS_FAULT:
                    // TODO add other leds 
                    break;
                case FAULT_ID_BPS_FAULT:
                    LED_set(CAR_BPSFAULT, LED_ON);
                    break;
                case FAULT_ID_GENERIC_WATCHDOG_FAULT:
                    LED_set(WATCHDOG, LED_ON);
                    break;

                default:
                    // TODO unknown fault, turn on generic fault LED
                    //LED_set(FAULT_LED, LED_ON);
                    break;
            }
        }
    }
}

void Task_FaultHandler(void *args __attribute__((unused))) {
    if (!faults_init()) {
        // Fault bit initialization failed, cannot proceed safely
        Error_Handler();
    }

    while (true) {
        EventBits_t bits = faults_wait(FAULT_ID_COUNT, portMAX_DELAY);

        if (bits != 0) {
            FHT_kill_precharge_task();
            contactor_emergency_open_all();
            clear_MotorSafeBit(MOTOR_CONTACTOR_ENABLED);
            clear_MotorSafeBit(MOTOR_PRECHARGE_CONTACTOR_ENABLED);

            // fsm_disable();
            watchdog_stop_all();

            printf("Fault Handler triggered: 0x%02lX\r\n", bits);

            // Loop to display/handle fault until system reset
            uint32_t print_counter = 0;
            while (true) {
                print_counter++;
                if (print_counter >= FAULT_PRINTF_COUNTER) {
                    for (int i = 0; i < FAULT_ID_COUNT; i++) {
                        if (bits & FAULT_BIT(i)) {
                            printf("Fault: %s\r\n", fault_names[i]);
                        }
                    }
                    print_counter = 0;
                }

                FHT_set_fault_leds(bits);
                LED_toggle(HB);
                vTaskDelay(pdMS_TO_TICKS(FAULT_LOOP_PERIOD_MS));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
