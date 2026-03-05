#include "inits.h"
#include "StatusLEDs.h"
#include "Contactors.h"

// Task configuration
#define TEST_TASK_STACK_SIZE 256
#define TEST_TASK_PRIORITY   ( tskIDLE_PRIORITY + 1 )
#define DELAY_2S             pdMS_TO_TICKS(2000)

// Static task buffers
static StaticTask_t xTestTaskBuffer;
static StackType_t xTestStack[TEST_TASK_STACK_SIZE];

void vContactorTestTask(void *pvParameters) {
    while (1) 
    {
        // Cycle through each contactor: Close then Open with 2s delay
        // Close Contactors (GPIO_PIN_SET assumes high = closed)
        LED_set(CAR_HB, ON);
        contactor_set(MOTOR_CONTACTOR, CLOSED, 100, NORMAL);
        vTaskDelay(DELAY_2S);

        LED_set(CAR_HB, OFF);
        contactor_set(MOTOR_PRE_CONTACTOR, CLOSED, 100, NORMAL);
        vTaskDelay(DELAY_2S);

        // Open Contactors
        LED_set(CAR_HB, OFF);
        contactor_set(MOTOR_CONTACTOR, OPEN, 100, NORMAL);
        vTaskDelay(DELAY_2S);

        LED_set(CAR_HB, OFF);
        contactor_set(MOTOR_PRE_CONTACTOR, OPEN, 100, NORMAL);
        vTaskDelay(DELAY_2S);
    }
}

/**
 * @brief Task that cycles through all contactors and then toggles the HV+
 */
int main() {
    // Initialize the contactor hardware and software abstractions

    HAL_Init();

    SystemClock_Config();

    LEDs_init();

    contactor_init();

    xTaskCreateStatic(
        vContactorTestTask,
        "ContactorTest",
        TEST_TASK_STACK_SIZE,
        NULL,
        TEST_TASK_PRIORITY,
        xTestStack,
        &xTestTaskBuffer
    );

    vTaskStartScheduler();

    return 0;
}