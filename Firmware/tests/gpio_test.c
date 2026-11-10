#include <assert.h>
#include <stdio.h>
#include "CH58x_common.h"
#include "buttons.h"
#include "board.h"
uint16_t R16_PA_INT_EN,R16_PB_INT_EN,R16_PA_INT_IF,R16_PB_INT_IF;
static uint32_t port_a=0xffff,port_b=0xffffffff;
static uint16_t pin_alternate;
void GPIOA_ModeCfg(uint32_t m,unsigned mode) { assert(m==BOARD_BUTTON_A_MASK && mode==GPIO_ModeIN_PU); }
void GPIOB_ModeCfg(uint32_t m,unsigned mode) { assert(m==BOARD_BUTTON_B_MASK && mode==GPIO_ModeIN_PU); }
void PFIC_EnableIRQ(unsigned irq) { assert(irq==GPIO_A_IRQn || irq==GPIO_B_IRQn); }
uint32_t GPIOA_ReadPort(void) { return port_a; }
uint32_t GPIOB_ReadPort(void) { return port_b; }
void SYS_DisableAllIrq(unsigned long *s) { *s=1; }
void SYS_RecoverIrq(unsigned long s) { assert(s==1); }
void GPIOA_ITModeCfg(uint32_t m,unsigned mode) { assert(mode==GPIO_ITMode_FallEdge); R16_PA_INT_EN|=m; }
/* Like the SDK: PB22/PB23 map to interrupt bits 8/9. */
void GPIOB_ITModeCfg(uint32_t m,unsigned mode) { assert(mode==GPIO_ITMode_FallEdge); R16_PB_INT_EN|=(uint16_t)(m|((m&(3UL<<22))>>14)); }
void GPIOPinRemap(unsigned s,uint16_t perph) { assert(s==ENABLE); pin_alternate|=perph; }
void GPIOA_IRQHandler(void);
void GPIOB_IRQHandler(void);
int main(void)
{
    unsigned i;
    const unsigned pins[]={BOARD_SW2_PB_PIN,4,2,1,3};
    buttons_gpio_init(); assert(buttons_gpio_read()==0);
    /* rev2 routes the PB22 interrupt; rev1 must leave PB8/PB9 untouched. */
    assert(!!(pin_alternate & RB_PIN_INTX) == BOARD_SW2_INT_REMAP);
    port_a &= ~BOARD_BUTTON_A_MASK; assert(buttons_gpio_read()==1); port_a=0xffff;
    for(i=0;i<5;++i) { port_b=0xffffffff & ~(1UL<<pins[i]); assert(buttons_gpio_read()==(1U<<(i+1))); }
    port_b=0xffffffff;
    R16_PA_INT_EN=R16_PB_INT_EN=0x8000;
    buttons_gpio_arm(); assert(!buttons_gpio_take_wake());
    R16_PA_INT_IF=BOARD_BUTTON_A_MASK; GPIOA_IRQHandler();
    assert(buttons_gpio_take_wake()==1 && !buttons_gpio_take_wake());
    assert(R16_PA_INT_EN==0x8000 && R16_PB_INT_EN==0x8000);
    /* A held-low pin at arm time is caught without needing another edge. */
    port_b &= ~(1U<<2); buttons_gpio_arm(); assert(buttons_gpio_take_wake());
    port_b=0xffffffff; R16_PB_INT_EN=0x8000; buttons_gpio_arm();
    assert(R16_PB_INT_EN==(0x8000|BOARD_BUTTON_B_INT_MASK)); /* SW2 armed */
    R16_PB_INT_IF=(uint16_t)(BOARD_SW2_INT_REMAP ? 1U<<8 : 1U<<BOARD_SW2_PB_PIN);
    GPIOB_IRQHandler(); assert(buttons_gpio_take_wake());
    assert(R16_PB_INT_EN==0x8000);
    printf("PASS board rev%d: ", BOARD_REV);
    puts("six switch mappings, GPIO wake latch, interrupt masking and held-low rearm");
    return 0;
}
