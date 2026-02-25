#include "Contactors.h"

static SemaphoreHandle_t contactorsMutex = NULL;
static StaticSemaphore_t contactorsMutexBuffer;

static const char* CONTACTOR_NAMES[NUM_CONTACTORS] = {
    "Motor Contactor",
    "Motor Pre Contactor"
};

// array to hold the contactor structs
static contactor_t contactors[NUM_CONTACTORS];

// get
bool contactor_get(contactor_num_t contactor_num    ) {
    
    // check that contactor exists
    if ((contactor_num < 0) || (contactor_num >= NUM_CONTACTORS)) {
        Error_Handler();
    }

    contactor_t* contactor = &contactors[contactor_num];
    return HAL_GPIO_ReadPin(contactor->sense_pin.port, contactor->sense_pin.pin);
}

static void vContactorCallback( TimerHandle_t senseTimer ) {

    contactor_num_t contactor_num = (contactor_num_t)pvTimerGetTimerID(senseTimer);
    contactor_t* contactor = &contactors[contactor_num];

    if (contactor->state != contactor_get(contactor_num)) {
        Fault_Handler();
    }
}

/* sets contactor, updates state value, then starts timer to check expected state matches actual state. 
An error means semaphore was busy, or that I set a contactor that didn't exist. */
ErrorStatus contactor_set(contactor_num_t contactor_num, contactor_state_t state, uint32_t wait_ms, fault_state_t emergency) {
    
    // check that contactor exists
    if ((contactor_num < 0) || (contactor_num >= NUM_CONTACTORS)) {
        Error_Handler(); 
    }

    contactor_t* contactor = &contactors[contactor_num];

    // if its emergency, dont bother with semaphore
    if (!emergency && xSemaphoreTake(contactorsMutex, wait_ms) == pdFALSE) {
        return ERROR;
    };

    // critical section:
    HAL_GPIO_WritePin(contactor->control_pin.port, contactor->control_pin.pin, state);
    contactor->state = state;

    /* start timer to check if the state of the contactor makes expected state, the exit critical section. Timer resets
    when the contactor is set to another value, so no possible error with expected value changing from when timer is called*/
    if (!emergency) { 
        xTimerStart(contactor->senseTimer, 0); 
        // TODO: implement callback to set fault bits if not set properly
        xSemaphoreGive(contactorsMutex);
    }

    return SUCCESS;
}

void contactor_init() {

    // Enable clock for GPIO A/B/C 
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // INITIALIZE MUTEX
    contactorsMutex = xSemaphoreCreateMutexStatic(&contactorsMutexBuffer);


    const GpioPin_t Motor_Contactor_Enable = {MOTOR_CONTACTOR_ENABLE_PORT, MOTOR_CONTACTOR_ENABLE_PIN};
    const GpioPin_t Motor_Contactor_Sense = {MOTOR_CONTACTOR_SENSE_PORT, MOTOR_CONTACTOR_SENSE_PIN};
    const GpioPin_t Precharge_Contactor_Enable = {PRECHARGE_PRE_ENABLE_PORT, PRECHARGE_PRE_ENABLE_PIN};
    const GpioPin_t Precharge_Contactor_Sense = {PRECHARGE_PRE_SENSE_PORT, PRECHARGE_PRE_SENSE_PIN};

    contactors[MOTOR_CONTACTOR].state= OPEN;
    contactors[MOTOR_CONTACTOR].sense_pin= Motor_Contactor_Sense;
    contactors[MOTOR_CONTACTOR].control_pin = Motor_Contactor_Enable;
    contactors[MOTOR_PRE_CONTACTOR].state = OPEN;
    contactors[MOTOR_PRE_CONTACTOR].sense_pin = Precharge_Contactor_Sense;
    contactors[MOTOR_PRE_CONTACTOR].control_pin = Precharge_Contactor_Enable;
    
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    // loop to intialize contactor GPIO and timers
    for (uint32_t contactor_num = 0; contactor_num < NUM_CONTACTORS; contactor_num++) {

        contactor_t* contactor = &contactors[contactor_num];

        // init contactor control pins
        GPIO_InitStruct.Pin = contactor->control_pin.pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(contactor->control_pin.port, &GPIO_InitStruct);

        // init contactor sense pins 
        GPIO_InitStruct.Pin = contactor->sense_pin.pin;
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(contactor->sense_pin.port, &GPIO_InitStruct);

        contactor->state = contactor_get(contactor_num);

        // making timers and putting them into contactor structs
        contactor->senseTimer = xTimerCreateStatic(
            CONTACTOR_NAMES[contactor_num],                     /* Name of the timer */
            CONTACTOR_SENSE_DELAY,                              /* Timer period in ticks */
            pdFALSE,                                            /* Don't auto-reload */
            (void*)contactor_num,                               /* Timer ID */
            vContactorCallback,                                 /* Callback function */
            &(contactor->senseTimerBuffer)       /* Buffer to hold timer data */
        );
    }
}