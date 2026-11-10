#include <stdint.h>
#define RTC_TIMER_MAX_VALUE 0xa8c00000UL
#define SLEEP_RTC_MIN_TIME 32U
#define SLEEP_RTC_MAX_TIME 100000U
#define WAKE_UP_RTC_MAX_TIME 46U
#define RB_PWR_RAM2K 1
#define RB_PWR_RAM30K 2
#define RB_PWR_EXTEND 4
#define HSE_RCur_100 0
#define RB_SLP_RTC_WAKE 1
#define RB_RTC_TRIG_EN 1
#define RTC_IRQn 1
#define ENABLE 1
#define RB_SLP_GPIO_WAKE 2
#define Long_Delay 0
extern uint8_t R8_SLP_WAKE_CTRL, R8_RTC_MODE_CTRL;
extern volatile uint32_t RTCTigFlag;
void SYS_DisableAllIrq(unsigned long *s);
void SYS_RecoverIrq(unsigned long s);
uint32_t RTC_GetCycle32k(void);
void RTC_SetTignTime(uint32_t time);
void LowPower_Sleep(uint32_t mask);
void LowPower_Idle(void);
void HSECFG_Current(unsigned current);
void sys_safe_access_enable(void);
void sys_safe_access_disable(void);
void PFIC_EnableIRQ(unsigned irq);
void PWR_PeriphWakeUpCfg(unsigned enable,unsigned source,unsigned delay);
