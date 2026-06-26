/**
 * @file HeartbeatTask.c
 * @brief Liveness heartbeat (see header).
 * @copyright Copyright (c) 2026 UT Longhorn Racing Solar
 */

#include "HeartbeatTask.h"
#include "StatusLEDs.h"
#include "FreeRTOS.h"
#include "task.h"

#define HEARTBEAT_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define HEARTBEAT_TOGGLE_PERIOD_MS 500U /* 500 ms toggle => ~1 Hz blink */

static StaticTask_t s_heartbeat_task_buf;
static StackType_t s_heartbeat_task_stack[HEARTBEAT_TASK_STACK_SIZE];

static void Task_Heartbeat(void *pvParameters) {
    (void)pvParameters;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        LED_toggle(HB);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(HEARTBEAT_TOGGLE_PERIOD_MS));
    }
}

void HeartbeatTask_Init(void) {
    (void)xTaskCreateStatic(
        Task_Heartbeat,
        "Heartbeat",
        HEARTBEAT_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1U,
        s_heartbeat_task_stack,
        &s_heartbeat_task_buf);
}
