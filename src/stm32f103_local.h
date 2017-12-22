#include "mcu/stm32.h"
#include "mcu/stm32f103.h"

/* why is this not in the stm32f103.h ? */
struct GPIO {
  volatile uint32_t CRL;
  volatile uint32_t CRH;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
  volatile uint32_t BSRR;
  volatile uint32_t BRR;
  volatile uint32_t LCKR;
};

#define GPIOA_BASE	(APB2PERIPH_BASE + 0x0800)
#define GPIOA		((struct GPIO *) GPIOA_BASE)
#define GPIOB_BASE	(APB2PERIPH_BASE + 0x0C00)
#define GPIOB		((struct GPIO *) GPIOB_BASE)
#define GPIOC_BASE	(APB2PERIPH_BASE + 0x1000)
#define GPIOC		((struct GPIO *) GPIOC_BASE)
#define GPIOD_BASE	(APB2PERIPH_BASE + 0x1400)
#define GPIOD		((struct GPIO *) GPIOD_BASE)
#define GPIOE_BASE	(APB2PERIPH_BASE + 0x1800)
#define GPIOE		((struct GPIO *) GPIOE_BASE)

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

#define EXTI3_IRQ    9
#define EXTI4_IRQ    10
#define EXTI15_10_IRQ 40

