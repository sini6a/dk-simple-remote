#include <assert.h>
#include <stdio.h>
#include "HAL.h"
uint8_t R8_SLP_WAKE_CTRL, R8_RTC_MODE_CTRL;
volatile uint32_t RTCTigFlag;
static uint32_t clock_value, trigger;
static unsigned pending, sleeps, idles, restored, early_rtc;
void SYS_DisableAllIrq(unsigned long *s) { *s=1; }
void SYS_RecoverIrq(unsigned long s) { assert(s==1); }
uint32_t RTC_GetCycle32k(void) { return clock_value; }
void RTC_SetTignTime(uint32_t t) { trigger=t; RTCTigFlag=early_rtc; }
void LowPower_Sleep(uint32_t mask) { assert(mask==7); ++sleeps; }
void LowPower_Idle(void) { ++idles; clock_value=trigger; RTCTigFlag=1; }
void HSECFG_Current(unsigned c) { assert(c==0); ++restored; }
void sys_safe_access_enable(void) {}
void sys_safe_access_disable(void) {}
void PFIC_EnableIRQ(unsigned irq) { assert(irq==RTC_IRQn); }
void PWR_PeriphWakeUpCfg(unsigned e,unsigned s,unsigned d) { assert(e==ENABLE && s==RB_SLP_GPIO_WAKE && d==Long_Delay); }
uint8_t buttons_gpio_wake_pending(void) { return pending; }
uint32_t CH58X_LowPower(uint32_t time);
void HAL_SleepInit(void);
int main(void)
{
    HAL_SleepInit(); assert(R8_SLP_WAKE_CTRL && R8_RTC_MODE_CTRL);
    clock_value=100; assert(CH58X_LowPower(110)==2 && sleeps==0);
    pending=1; assert(CH58X_LowPower(500)==2 && sleeps==0); pending=0;
    early_rtc=1; assert(CH58X_LowPower(500)==3 && sleeps==0); early_rtc=0;
    /* A GPIO wake has no RTC flag: it must still wait for crystal settling. */
    clock_value=100; assert(CH58X_LowPower(500)==0);
    assert(sleeps==1 && idles==1 && restored==1 && trigger==146);
    clock_value=RTC_TIMER_MAX_VALUE-20;
    assert(CH58X_LowPower(500)==0 && trigger==26);
    assert(sleeps==2 && restored==2);
    puts("PASS: sleep admission, pending GPIO, expired deadline, GPIO crystal settling and RTC wrap");
    return 0;
}
