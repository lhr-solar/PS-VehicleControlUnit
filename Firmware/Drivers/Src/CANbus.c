#include "CANbus.h"

#define FDCAN_NVIC_PRIO configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 5

static can_status_t Motor_CANBus_Init(void){
    hfdcan3->Instance = FDCAN3;
    hfdcan3->Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan3->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan3->Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan3->Init.AutoRetransmission = ENABLE;
    hfdcan3->Init.TransmitPause = DISABLE;
    hfdcan3->Init.ProtocolException = DISABLE;
    hfdcan3->Init.NominalPrescaler = 20;
    hfdcan3->Init.NominalSyncJumpWidth = 1;
    hfdcan3->Init.NominalTimeSeg1 = 13;
    hfdcan3->Init.NominalTimeSeg2 = 2;
    hfdcan3->Init.DataPrescaler = 1;
    hfdcan3->Init.DataSyncJumpWidth = 1;
    hfdcan3->Init.DataTimeSeg1 = 1;
    hfdcan3->Init.DataTimeSeg2 = 1;
    hfdcan3->Init.StdFiltersNbr = 1;
    hfdcan3->Init.ExtFiltersNbr = 0;
    hfdcan3->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

    // accepts all CAN IDs from 
    // FDCAN3 Filter Config
    FDCAN_FilterTypeDef sFilterConfig3;
    sFilterConfig3.IdType = FDCAN_STANDARD_ID;
    sFilterConfig3.FilterIndex = 0;
    sFilterConfig3.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig3.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; // directs frames to FIFO0
    sFilterConfig3.FilterID1 = 0x000;
    sFilterConfig3.FilterID2 = 0x000;

    if(can_fd_init(hfdcan3, &sFilterConfig3) != CAN_OK){
       return CAN_ERR;
    }

    if(can_fd_start(hfdcan3) != CAN_OK){
       return CAN_ERR;
    }

    return CAN_OK;

}

can_status_t CANBus_Init(void){

    if(Motor_CANBus_Init() != CAN_OK){
        return CAN_ERR;
    }
    return CAN_OK;
}


static uint32_t HAL_RCC_FDCAN_CLK_ENABLED=0;

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* fdcanHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if(fdcanHandle->Instance==FDCAN1)
  {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* FDCAN1 clock enable */
    HAL_RCC_FDCAN_CLK_ENABLED++;
    if(HAL_RCC_FDCAN_CLK_ENABLED==1){
      __HAL_RCC_FDCAN_CLK_ENABLE();
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**FDCAN1 GPIO Configuration
    PA11     ------> FDCAN1_RX
    PA12     ------> FDCAN1_TX
    */ 
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* FDCAN1 interrupt Init */
    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, FDCAN_NVIC_PRIO, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    HAL_NVIC_SetPriority(FDCAN1_IT1_IRQn, FDCAN_NVIC_PRIO, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT1_IRQn);
  }

  else if(fdcanHandle->Instance==FDCAN3)
  {
  /** Initializes the peripherals clocks
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* FDCAN3 clock enable */
    HAL_RCC_FDCAN_CLK_ENABLED++;
    if(HAL_RCC_FDCAN_CLK_ENABLED==1){
      __HAL_RCC_FDCAN_CLK_ENABLE();
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**FDCAN3 GPIO Configuration
    PA8     ------> FDCAN3_RX
    PA15     ------> FDCAN3_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF11_FDCAN3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* FDCAN3 interrupt Init */
    HAL_NVIC_SetPriority(FDCAN3_IT0_IRQn, FDCAN_NVIC_PRIO, 0);
    HAL_NVIC_EnableIRQ(FDCAN3_IT0_IRQn);
    HAL_NVIC_SetPriority(FDCAN3_IT1_IRQn, FDCAN_NVIC_PRIO, 0);
    HAL_NVIC_EnableIRQ(FDCAN3_IT1_IRQn);

  }
}

void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef* fdcanHandle)
{
    if(fdcanHandle->Instance==FDCAN1)
    {
        /* Peripheral clock disable */
        HAL_RCC_FDCAN_CLK_ENABLED--;
        if(HAL_RCC_FDCAN_CLK_ENABLED==0){
        __HAL_RCC_FDCAN_CLK_DISABLE();
        }

        /**FDCAN1 GPIO Configuration
        PA11     ------> FDCAN1_RX
        PA12     ------> FDCAN1_TX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

        /* FDCAN1 interrupt Deinit */
        HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
        HAL_NVIC_DisableIRQ(FDCAN1_IT1_IRQn);
    }

    else if(fdcanHandle->Instance==FDCAN3)
    {
        /* Peripheral clock disable */
        HAL_RCC_FDCAN_CLK_ENABLED--;
        if(HAL_RCC_FDCAN_CLK_ENABLED==0){
        __HAL_RCC_FDCAN_CLK_DISABLE();
        }

        /**FDCAN3 GPIO Configuration
        PA8     ------> FDCAN3_RX
        PA15     ------> FDCAN3_TX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8|GPIO_PIN_15);

        /* FDCAN3 interrupt Deinit */
        HAL_NVIC_DisableIRQ(FDCAN3_IT0_IRQn);
        HAL_NVIC_DisableIRQ(FDCAN3_IT1_IRQn);
    }
}
