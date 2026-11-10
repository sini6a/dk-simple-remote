#include "CH58x_common.h"
#include "board.h"
#include "buttons.h"
void buttons_gpio_init(void)
{
    GPIOA_ModeCfg(BOARD_BUTTON_A_MASK, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(BOARD_BUTTON_B_MASK, GPIO_ModeIN_PU);
#if BOARD_SW2_INT_REMAP
    GPIOPinRemap(ENABLE, RB_PIN_INTX); /* PB22 (SW2) interrupt instead of PB8 */
#endif
    PFIC_EnableIRQ(GPIO_A_IRQn);
    PFIC_EnableIRQ(GPIO_B_IRQn);
}
uint8_t buttons_gpio_read(void)
{
    uint32_t a = GPIOA_ReadPort();
    uint32_t b = GPIOB_ReadPort();
    return (uint8_t)((!(a & (1UL << 13)) << 0) |
                     (!(b & (1UL << BOARD_SW2_PB_PIN)) << 1) |
                     (!(b & (1UL << 4)) << 2) |
                     (!(b & (1UL << 2)) << 3) |
                     (!(b & (1UL << 1)) << 4) |
                     (!(b & (1UL << 3)) << 5));
}

/* ISR owns only the six switch flags. TMOS and debounce run in main context. */
static volatile uint8_t wake_pending;
uint8_t buttons_gpio_wake_pending(void) { return wake_pending; }
uint8_t buttons_gpio_take_wake(void)
{
    unsigned long state;
    uint8_t pending;
    SYS_DisableAllIrq(&state);
    pending = wake_pending; wake_pending = 0;
    SYS_RecoverIrq(state);
    return pending;
}
void buttons_gpio_arm(void)
{
    unsigned long state;
    SYS_DisableAllIrq(&state);
    GPIOA_ITModeCfg(BOARD_BUTTON_A_MASK, GPIO_ITMode_FallEdge);
    GPIOB_ITModeCfg(BOARD_BUTTON_B_MASK, GPIO_ITMode_FallEdge);
    /* A press during rearming must be sampled even if its edge was cleared. */
    if (buttons_gpio_read()) wake_pending = 1;
    SYS_RecoverIrq(state);
}
__HIGH_CODE void buttons_gpio_disarm(void)
{
    R16_PA_INT_EN &= ~BOARD_BUTTON_A_MASK;
    R16_PB_INT_EN &= ~BOARD_BUTTON_B_INT_MASK;
}
__INTERRUPT __HIGH_CODE void GPIOA_IRQHandler(void)
{
    if (R16_PA_INT_IF & BOARD_BUTTON_A_MASK) {
        R16_PA_INT_IF = BOARD_BUTTON_A_MASK;
        buttons_gpio_disarm(); wake_pending = 1;
    }
}
__INTERRUPT __HIGH_CODE void GPIOB_IRQHandler(void)
{
    if (R16_PB_INT_IF & BOARD_BUTTON_B_INT_MASK) {
        R16_PB_INT_IF = BOARD_BUTTON_B_INT_MASK;
        buttons_gpio_disarm(); wake_pending = 1;
    }
}
