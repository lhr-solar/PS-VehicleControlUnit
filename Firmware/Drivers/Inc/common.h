#pragma once
#include <stdint.h>

typedef enum State_e {OFF = 0, ON} State;

typedef struct {
    uint32_t port;
    uint16_t pin;
} GPIO_Pin_t;

// Define a struct type for configs
struct VCUConfig {
    uint32_t mask;
    uint32_t can_id;
};

void gpio_clock_enable(uint32_t port);
