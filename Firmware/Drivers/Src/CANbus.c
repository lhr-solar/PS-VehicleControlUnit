#include "CANbus.h"

#define FDCAN_NVIC_PRIO configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 5

FDCAN_HandleTypeDef *motorfdcan;

can_status_t Motor_CANBus_Init(void)
{

  motorfdcan = hfdcan1;
  motorfdcan->Instance = FDCAN1;

  motorfdcan->Init.ClockDivider = FDCAN_CLOCK_DIV1;
  motorfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  motorfdcan->Init.Mode = FDCAN_MODE_NORMAL;
  motorfdcan->Init.AutoRetransmission = ENABLE;
  motorfdcan->Init.TransmitPause = DISABLE;
  motorfdcan->Init.ProtocolException = DISABLE;
  motorfdcan->Init.NominalPrescaler = 20;
  motorfdcan->Init.NominalSyncJumpWidth = 1;
  motorfdcan->Init.NominalTimeSeg1 = 13;
  motorfdcan->Init.NominalTimeSeg2 = 2;
  motorfdcan->Init.DataPrescaler = 1;
  motorfdcan->Init.DataSyncJumpWidth = 1;
  motorfdcan->Init.DataTimeSeg1 = 1;
  motorfdcan->Init.DataTimeSeg2 = 1;
  motorfdcan->Init.StdFiltersNbr = 1;
  motorfdcan->Init.ExtFiltersNbr = 0;
  motorfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

  // accepts all CAN IDs from
  // FDCAN1 Filter Config
  FDCAN_FilterTypeDef sFilterConfig1;
  sFilterConfig1.IdType = FDCAN_STANDARD_ID;
  sFilterConfig1.FilterIndex = 0;
  sFilterConfig1.FilterType = FDCAN_FILTER_MASK;
  sFilterConfig1.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; // directs frames to FIFO0
  sFilterConfig1.FilterID1 = 0x000;
  sFilterConfig1.FilterID2 = 0x000;

  if (can_fd_init(motorfdcan, &sFilterConfig1) != CAN_OK)
  {
    return CAN_ERR;
  }

  if (can_fd_start(motorfdcan) != CAN_OK)
  {
    return CAN_ERR;
  }

  return CAN_OK;
}

FDCAN_HandleTypeDef *carfdcan;

can_status_t Car_CANBus_Init(void)
{

  carfdcan = hfdcan3;
  carfdcan->Instance = FDCAN3;

  carfdcan->Init.ClockDivider = FDCAN_CLOCK_DIV1;
  carfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  carfdcan->Init.Mode = FDCAN_MODE_NORMAL;
  carfdcan->Init.AutoRetransmission = ENABLE;
  carfdcan->Init.TransmitPause = DISABLE;
  carfdcan->Init.ProtocolException = DISABLE;
  carfdcan->Init.NominalPrescaler = 20;
  carfdcan->Init.NominalSyncJumpWidth = 1;
  carfdcan->Init.NominalTimeSeg1 = 13;
  carfdcan->Init.NominalTimeSeg2 = 2;
  carfdcan->Init.DataPrescaler = 1;
  carfdcan->Init.DataSyncJumpWidth = 1;
  carfdcan->Init.DataTimeSeg1 = 1;
  carfdcan->Init.DataTimeSeg2 = 1;
  carfdcan->Init.StdFiltersNbr = 1;
  carfdcan->Init.ExtFiltersNbr = 0;
  carfdcan->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

  // accepts all CAN IDs from
  // FDCAN1 Filter Config
  FDCAN_FilterTypeDef sFilterConfig1;
  sFilterConfig1.IdType = FDCAN_STANDARD_ID;
  sFilterConfig1.FilterIndex = 0;
  sFilterConfig1.FilterType = FDCAN_FILTER_MASK;
  sFilterConfig1.FilterConfig = FDCAN_FILTER_TO_RXFIFO0; // directs frames to FIFO0
  sFilterConfig1.FilterID1 = 0x000;
  sFilterConfig1.FilterID2 = 0x000;

  if (can_fd_init(carfdcan, &sFilterConfig1) != CAN_OK)
  {
    return CAN_ERR;
  }

  if (can_fd_start(carfdcan) != CAN_OK)
  {
    return CAN_ERR;
  }

  return CAN_OK;
}

can_status_t Motor_CANBus_Send(FDCAN_TxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks)
{
  return can_fd_send(motorfdcan, header, data, delay_ticks);
}

can_status_t Motor_CANBus_Recieve(uint16_t id, FDCAN_RxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks)
{
  return can_fd_recv(motorfdcan, id, header, data, delay_ticks);
}

can_status_t Car_CANBus_Send(FDCAN_TxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks)
{
  return can_fd_send(carfdcan, header, data, delay_ticks);
}

can_status_t Car_CANBus_Recieve(uint16_t id, FDCAN_RxHeaderTypeDef *header, uint8_t data[], TickType_t delay_ticks)
{
  return can_fd_recv(carfdcan, id, header, data, delay_ticks);
}

static uint32_t HAL_RCC_FDCAN_CLK_ENABLED = 0;

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *fdcanHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (fdcanHandle->Instance == FDCAN1)
  {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* FDCAN1 clock enable */
    HAL_RCC_FDCAN_CLK_ENABLED++;
    if (HAL_RCC_FDCAN_CLK_ENABLED == 1)
    {
      __HAL_RCC_FDCAN_CLK_ENABLE();
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**FDCAN1 GPIO Configuration
    PA11     ------> FDCAN1_RX
    PA12     ------> FDCAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
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

  else if (fdcanHandle->Instance == FDCAN3)
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
    if (HAL_RCC_FDCAN_CLK_ENABLED == 1)
    {
      __HAL_RCC_FDCAN_CLK_ENABLE();
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**FDCAN3 GPIO Configuration
    PA8     ------> FDCAN3_RX
    PA15     ------> FDCAN3_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_15;
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

void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef *fdcanHandle)
{
  if (fdcanHandle->Instance == FDCAN1)
  {
    /* Peripheral clock disable */
    HAL_RCC_FDCAN_CLK_ENABLED--;
    if (HAL_RCC_FDCAN_CLK_ENABLED == 0)
    {
      __HAL_RCC_FDCAN_CLK_DISABLE();
    }

    /**FDCAN1 GPIO Configuration
    PA11     ------> FDCAN1_RX
    PA12     ------> FDCAN1_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);

    /* FDCAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
    HAL_NVIC_DisableIRQ(FDCAN1_IT1_IRQn);
  }

  else if (fdcanHandle->Instance == FDCAN3)
  {
    /* Peripheral clock disable */
    HAL_RCC_FDCAN_CLK_ENABLED--;
    if (HAL_RCC_FDCAN_CLK_ENABLED == 0)
    {
      __HAL_RCC_FDCAN_CLK_DISABLE();
    }

    /**FDCAN3 GPIO Configuration
    PA8     ------> FDCAN3_RX
    PA15     ------> FDCAN3_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8 | GPIO_PIN_15);

    /* FDCAN3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(FDCAN3_IT0_IRQn);
    HAL_NVIC_DisableIRQ(FDCAN3_IT1_IRQn);
  }
}
