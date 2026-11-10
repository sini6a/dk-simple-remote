#include <stdint.h>
#define __INTERRUPT
#define __HIGH_CODE
#define GPIO_ModeIN_PU 1
#define GPIO_ITMode_FallEdge 2
#define GPIO_A_IRQn 1
#define GPIO_B_IRQn 2
#define ENABLE 1
#define RB_PIN_INTX 0x2000
extern uint16_t R16_PA_INT_EN,R16_PB_INT_EN,R16_PA_INT_IF,R16_PB_INT_IF;
void GPIOA_ModeCfg(uint32_t mask,unsigned mode);
void GPIOB_ModeCfg(uint32_t mask,unsigned mode);
void PFIC_EnableIRQ(unsigned irq);
uint32_t GPIOA_ReadPort(void);
uint32_t GPIOB_ReadPort(void);
void SYS_DisableAllIrq(unsigned long *s);
void SYS_RecoverIrq(unsigned long s);
void GPIOA_ITModeCfg(uint32_t mask,unsigned mode);
void GPIOB_ITModeCfg(uint32_t mask,unsigned mode);
void GPIOPinRemap(unsigned s,uint16_t perph);
