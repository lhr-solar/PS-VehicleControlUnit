#include "FaultHandlerTask.h"
#include "InitTask.h"   // for hprecharge_task handle
#include "Watchdogs.h"

#define FAULT_LOOP_PRINTF_DELAY_MS 10000
#define FAULT_PRINTF_COUNTER       (FAULT_LOOP_PRINTF_DELAY_MS / FAULT_LOOP_PERIOD_MS)


static void FHT_kill_precharge_task(void) {
    if (precharge_task_handle != NULL) {
        vTaskDelete(precharge_task_handle);
        precharge_task_handle = NULL;
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

            fsm_disable();
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
                vTaskDelay(pdMS_TO_TICKS(FAULT_LOOP_PERIOD_MS));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}