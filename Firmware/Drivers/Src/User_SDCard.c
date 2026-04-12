#include "User_SDCard.h"

sd_handle_t sd;
SPI_HandleTypeDef hspi_user;   

#define SDCARD_SPI_NVIC_PRIO (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+5)

static BaseType_t SDCard_SPI_Init(){

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // enable SPI1 clock
    __HAL_RCC_SPI1_CLK_ENABLE();

    /* Configure GPIO (SCK, MISO, MOSI, CS) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    GPIO_InitStruct.Pin = SDCARD_SCK_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1; 
    HAL_GPIO_Init(SDCARD_SCK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SDCARD_MISO_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(SDCARD_MISO_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SDCARD_MOSI_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(SDCARD_MOSI_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SDCARD_CS_PORT, SDCARD_CS_PIN, GPIO_PIN_SET); 
    GPIO_InitStruct.Pin = SDCARD_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(SDCARD_CS_PORT, &GPIO_InitStruct);

    /* Configure SPI Peripheral */
    hspi_user.Instance = SPI1;
    hspi_user.Init.Mode = SPI_MODE_MASTER;
    hspi_user.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_user.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_user.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi_user.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi_user.Init.NSS = SPI_NSS_SOFT;
    hspi_user.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2; 


    hspi_user.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi_user.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi_user.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    hspi_user.Init.CRCPolynomial = 7;
    hspi_user.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi_user.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;

    if (HAL_SPI_Init(&hspi_user) != HAL_OK) { 
        return pdFAIL;
    }
    __HAL_SPI_ENABLE(&hspi_user);


    HAL_NVIC_SetPriority(SPI1_IRQn, SDCARD_SPI_NVIC_PRIO, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);

    return pdPASS;
}

sd_status_t SDCard_Init(void){


    sd.hspi = &hspi_user;
    sd.cs_port = SDCARD_CS_PORT; 
    sd.cs_pin  = SDCARD_CS_PIN;

    if(SDCard_SPI_Init() != pdPASS){
        return SD_ERROR;
    }

    // initialize the sd card worker task.
    return USER_SD_Card_Init(&sd, SDCARD_WORKER_THREAD_PRIO);
}

sd_status_t SDCard_Write(const char *filename, const char *data, TickType_t delay_ticks){
    return USER_SD_Card_Write_Async(&sd, filename, data, delay_ticks);
}


// recieve and transmit
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    sdcard_SPI_TxRxCpltCallback(hspi, &xHigherPriorityTaskWoken);
    
    // Context switch if a higher priority task was woken up
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) { 
    HAL_SPI_TxRxCpltCallback(hspi); 
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) { 
    HAL_SPI_TxRxCpltCallback(hspi); 
}

// IRQ HANDLER
void SPI1_IRQHandler(void) { 
    HAL_SPI_IRQHandler(&hspi_user); 
}