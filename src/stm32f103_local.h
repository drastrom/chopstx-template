#include "mcu/stm32f103.h"

#define DMA1_ChannelN_BASE(n)    (AHBPERIPH_BASE + 0x0008 + 20*(n-1))
#define Define_DMA1_ChannelN(n) \
	static struct DMA_Channel *const DMA1_Channel##n = \
	(struct DMA_Channel *)DMA1_ChannelN_BASE(n)

Define_DMA1_ChannelN(2);
Define_DMA1_ChannelN(3);
Define_DMA1_ChannelN(4);
Define_DMA1_ChannelN(5);
Define_DMA1_ChannelN(6);
Define_DMA1_ChannelN(7);

#undef Define_DMA1_ChannelN

#define DMA1_CHANNEL1_IRQ 11
#define DMA1_CHANNEL2_IRQ 12
#define DMA1_CHANNEL3_IRQ 13
#define DMA1_CHANNEL4_IRQ 14
#define DMA1_CHANNEL5_IRQ 15
#define DMA1_CHANNEL6_IRQ 16
#define DMA1_CHANNEL7_IRQ 17

#define TIM1_BASE (APB2PERIPH_BASE + 0x2C00)
static struct TIM *const TIM1 = (struct TIM *)TIM1_BASE;
#define TIM1_BRK_IRQ 24
#define TIM1_UP_IRQ 25
#define TIM1_TRG_COM_IRQ 26
#define TIM1_CC_IRQ 27

#define EXTI15_10_IRQ 40

struct USART {
  volatile uint32_t SR;
  volatile uint32_t DR;
  volatile uint32_t BRR;
  volatile uint32_t CR1;
  volatile uint32_t CR2;
  volatile uint32_t CR3;
  volatile uint32_t GTPR;
};

#define USART1_BASE           (APB2PERIPH_BASE + 0x3800)
#define USART2_BASE           (APB1PERIPH_BASE + 0x4400)
#define USART3_BASE           (APB1PERIPH_BASE + 0x4800)
static struct USART *const USART1 = (struct USART *)USART1_BASE;
static struct USART *const USART2 = (struct USART *)USART2_BASE;
static struct USART *const USART3 = (struct USART *)USART3_BASE;

#define USART_SR_CTS	(1 << 9)
#define USART_SR_LBD	(1 << 8)
#define USART_SR_TXE	(1 << 7)
#define USART_SR_TC	(1 << 6)
#define USART_SR_RXNE	(1 << 5)
#define USART_SR_IDLE	(1 << 4)
#define USART_SR_ORE	(1 << 3)
#define USART_SR_NE	(1 << 2)
#define USART_SR_FE	(1 << 1)
#define USART_SR_PE	(1 << 0)

#define USART_CR1_UE		(1 << 13)
#define USART_CR1_M		(1 << 12)
#define USART_CR1_WAKE		(1 << 11)
#define USART_CR1_PCE		(1 << 10)
#define USART_CR1_PS		(1 <<  9)
#define USART_CR1_PEIE		(1 <<  8)
#define USART_CR1_TXEIE		(1 <<  7)
#define USART_CR1_TCIE		(1 <<  6)
#define USART_CR1_RXNEIE	(1 <<  5)
#define USART_CR1_IDLEIE	(1 <<  4)
#define USART_CR1_TE		(1 <<  3)
#define USART_CR1_RE		(1 <<  2)
#define USART_CR1_RWU		(1 <<  1)
#define USART_CR1_SBK		(1 <<  0)

#define USART2_IRQ 38
#define USART3_IRQ 39

#define RCC_APB2RSTR_USART1RST	(1 << 14)
#define RCC_APB2ENR_USART1EN	(1 << 14)

#define RCC_APB1RSTR_USART2RST	(1 << 17)
#define RCC_APB1RSTR_USART3RST	(1 << 18)

#define RCC_APB1ENR_USART2EN	(1 << 17)
#define RCC_APB1ENR_USART3EN	(1 << 18)
