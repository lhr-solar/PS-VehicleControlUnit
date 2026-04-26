#include "bootloader_bringup.h"

#if defined(VCU_BOOTLOADER_BRINGUP)

#include "inits.h"
#include "StatusLEDs.h"
#include "uart_bootloader.h"

#if !defined(VCU_BOOTLOADER_BRINGUP_LEVEL)
#define VCU_BOOTLOADER_BRINGUP_LEVEL 0
#endif

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 4)
#include "UART.h"
#endif

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 5)
#include "CANbus.h"
#endif

#define BRINGUP_BLINK_PERIOD_MS (250U)
#define BRINGUP_STATUS_LED_PERIOD_MS (125U)
#define BRINGUP_CAN_PERIOD_MS (500U)
#define BRINGUP_CAN_ID (0x321U)

#if defined(STM32G473xx)
#define BRINGUP_LED_PORT GPIOC
#define BRINGUP_LED_PIN GPIO_PIN_3
#else
#define BRINGUP_LED_PORT HB_LED_PORT
#define BRINGUP_LED_PIN HB_LED_PIN
#endif

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 5)
static bool s_can_ready = false;
#endif

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 4)
static bool s_uart_ready = false;
#endif

void VCU_BootloaderBringup_EarlyIndicator(void)
{
#if defined(STM32G473xx)
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
  __DSB();

  GPIOC->MODER &= ~(GPIO_MODER_MODE3_Msk);
  GPIOC->MODER |= (1UL << GPIO_MODER_MODE3_Pos);
  GPIOC->OTYPER &= ~(GPIO_OTYPER_OT3);
  GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD3_Msk);

  for (uint32_t pulse = 0U; pulse < 8U; pulse++)
  {
    GPIOC->ODR ^= GPIO_ODR_OD3;
    for (volatile uint32_t delay = 0U; delay < 400000U; delay++)
    {
      __NOP();
    }
  }

  GPIOC->BSRR = GPIO_PIN_3;
#endif
}

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 2)
static void BringupLed_Init(void)
{
  GPIO_InitTypeDef led_config = {
      .Mode = GPIO_MODE_OUTPUT_PP,
      .Pull = GPIO_NOPULL,
      .Pin = BRINGUP_LED_PIN,
  };

  if (BRINGUP_LED_PORT == GPIOA)
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
  }
  else if (BRINGUP_LED_PORT == GPIOB)
  {
    __HAL_RCC_GPIOB_CLK_ENABLE();
  }
  else if (BRINGUP_LED_PORT == GPIOC)
  {
    __HAL_RCC_GPIOC_CLK_ENABLE();
  }

  HAL_GPIO_Init(BRINGUP_LED_PORT, &led_config);
  HAL_GPIO_WritePin(BRINGUP_LED_PORT, BRINGUP_LED_PIN, GPIO_PIN_SET);
}
#endif

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 2)
static void BringupStatusLeds_Step(void)
{
  static Status_Mapping_t led = PRECHARGE_COMPLETE;

  Toggle_LED(led);

  led++;
  if (led >= num_LEDs)
  {
    led = PRECHARGE_COMPLETE;
  }
}
#endif

static void BringupRawLed_Init(void)
{
#if defined(STM32G473xx)
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
  __DSB();

  GPIOC->MODER &= ~(GPIO_MODER_MODE3_Msk);
  GPIOC->MODER |= (1UL << GPIO_MODER_MODE3_Pos);
  GPIOC->OTYPER &= ~(GPIO_OTYPER_OT3);
  GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPD3_Msk);
  GPIOC->BSRR = GPIO_PIN_3;
#endif
}

static void BringupRawLed_Toggle(void)
{
#if defined(STM32G473xx)
  GPIOC->ODR ^= GPIO_ODR_OD3;
#endif
}

static void BringupRawDelay(void)
{
  for (volatile uint32_t delay = 0U; delay < 1200000U; delay++)
  {
    __NOP();
  }
}

static void BringupRawMarker(uint32_t pulses)
{
  BringupRawLed_Init();
  for (uint32_t pulse = 0U; pulse < pulses; pulse++)
  {
    BringupRawLed_Toggle();
    BringupRawDelay();
  }
#if defined(STM32G473xx)
  GPIOC->BSRR = GPIO_PIN_3;
#endif
}

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 4)
static bool BringupBootloaderUart_Init(void)
{
#if defined(USART3)
  husart3->Instance = USART3;
  husart3->Init.BaudRate = 115200;
  husart3->Init.WordLength = UART_WORDLENGTH_8B;
  husart3->Init.StopBits = UART_STOPBITS_1;
  husart3->Init.Parity = UART_PARITY_NONE;
  husart3->Init.Mode = UART_MODE_TX_RX;
  husart3->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  husart3->Init.OverSampling = UART_OVERSAMPLING_16;
  husart3->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  husart3->Init.ClockPrescaler = UART_PRESCALER_DIV1;
  husart3->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(husart3) != HAL_OK)
  {
    return false;
  }

  /* Bring-up polls the UART directly, so keep the FreeRTOS UART IRQ path off. */
  HAL_NVIC_DisableIRQ(USART3_IRQn);
  HAL_NVIC_ClearPendingIRQ(USART3_IRQn);
  return true;
#else
  return false;
#endif
}

static void BringupBootloaderCommand_Poll(void)
{
  uint8_t byte = 0U;

  if (!s_uart_ready)
  {
    return;
  }

  while (__HAL_UART_GET_FLAG(husart3, UART_FLAG_RXNE))
  {
    if (HAL_UART_Receive(husart3, &byte, 1U, 0U) != HAL_OK)
    {
      __HAL_UART_CLEAR_OREFLAG(husart3);
      return;
    }

    if (uart_bootloader_feed_command_byte(byte) && uart_bootloader_is_entry_allowed())
    {
      uart_bootloader_request_reset();
    }
  }
}
#endif

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 5)
static void BringupCanHeartbeat_Send(void)
{
  FDCAN_TxHeaderTypeDef header = {
      .Identifier = BRINGUP_CAN_ID,
      .IdType = FDCAN_STANDARD_ID,
      .TxFrameType = FDCAN_DATA_FRAME,
      .DataLength = FDCAN_DLC_BYTES_2,
      .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
      .BitRateSwitch = FDCAN_BRS_OFF,
      .FDFormat = FDCAN_CLASSIC_CAN,
      .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
      .MessageMarker = 0,
  };
  static uint8_t data[2] = {0xBC, 0x00U};

  if (s_can_ready)
  {
    data[1]++;
    (void)Motor_CANBus_Send(&header, data, 0);
    (void)Car_CANBus_Send(&header, data, 0);
  }
}
#endif

void VCU_BootloaderBringup_Run(void)
{
#if (VCU_BOOTLOADER_BRINGUP_LEVEL == 0)
  BringupRawLed_Init();
  while (1)
  {
    BringupRawLed_Toggle();
    BringupRawDelay();
  }
#else
  BringupRawMarker(8U);
  HAL_Init();

#if (VCU_BOOTLOADER_BRINGUP_LEVEL == 1)
  BringupRawMarker(4U);
  while (1)
  {
    BringupRawLed_Toggle();
    BringupRawDelay();
  }
#else

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 3)
  SystemClock_Config();
#endif

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
  __enable_irq();

  LEDs_init();
  BringupLed_Init();

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 4)
  s_uart_ready = BringupBootloaderUart_Init();
  uart_bootloader_set_entry_allowed(true);
#endif

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 5)
  s_can_ready = (CAN_Init() == CAN_OK);
#endif

  uint32_t blink_ms = 0U;
  uint32_t status_led_ms = 0U;
#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 5)
  uint32_t can_ms = 0U;
#endif

  while (1)
  {
    HAL_Delay(1U);
    blink_ms++;
    status_led_ms++;
#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 5)
    can_ms++;
#endif

    if (blink_ms >= BRINGUP_BLINK_PERIOD_MS)
    {
      blink_ms = 0U;
      HAL_GPIO_TogglePin(BRINGUP_LED_PORT, BRINGUP_LED_PIN);
    }

    if (status_led_ms >= BRINGUP_STATUS_LED_PERIOD_MS)
    {
      status_led_ms = 0U;
      BringupStatusLeds_Step();
    }

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 4)
    BringupBootloaderCommand_Poll();
#endif

#if (VCU_BOOTLOADER_BRINGUP_LEVEL >= 5)
    if (can_ms >= BRINGUP_CAN_PERIOD_MS)
    {
      can_ms = 0U;
      BringupCanHeartbeat_Send();
    }
#endif
  }
#endif
#endif
}

#endif /* VCU_BOOTLOADER_BRINGUP */
