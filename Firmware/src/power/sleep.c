/********************************** (C) COPYRIGHT *******************************
 * File Name          : SLEEP.c
 * Author             : WCH
 * Version            : V1.2
 * Date               : 2022/01/18
 * Description        : BLE sleep configuration
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 * SPDX-License-Identifier: Apache-2.0
 * Modified from the WCH original; see vendor/README.md.
 *******************************************************************************/


/* Application adaptation of WCH HAL SLEEP.c: GPIO wake is permitted only
 * with a full RTC-timed oscillator settling interval before returning to BLE.
 * No crystal load-capacitance changes are made here. */
#include "HAL.h"
#include "buttons.h"
uint32_t CH58X_LowPower(uint32_t time)
{
    uint32_t now, duration, settle;
    unsigned long state;
    SYS_DisableAllIrq(&state);
    now = RTC_GetCycle32k();
    duration = time >= now ? time - now : time + (RTC_TIMER_MAX_VALUE - now);
    if (buttons_gpio_wake_pending() || duration < SLEEP_RTC_MIN_TIME ||
        duration > SLEEP_RTC_MAX_TIME) {
        SYS_RecoverIrq(state); return 2;
    }
    RTC_SetTignTime(time);
    SYS_RecoverIrq(state);
    if (RTCTigFlag || buttons_gpio_wake_pending()) return 3;
    LowPower_Sleep(RB_PWR_RAM2K | RB_PWR_RAM30K | RB_PWR_EXTEND);
    /* Also guard GPIO wakes: the vendor example guards only RTC wakes.
     * Count from actual wake, so repeated GPIO edges cannot shorten settling.
     * BLE owns the next deadline and will recalculate it on callback return. */
    settle = RTC_GetCycle32k() + WAKE_UP_RTC_MAX_TIME;
    if (settle >= RTC_TIMER_MAX_VALUE) settle -= RTC_TIMER_MAX_VALUE;
    RTC_SetTignTime(settle);
    while (!RTCTigFlag) LowPower_Idle();
    HSECFG_Current(HSE_RCur_100);
    return 0;
}
void HAL_SleepInit(void)
{
    sys_safe_access_enable();
    R8_SLP_WAKE_CTRL |= RB_SLP_RTC_WAKE;
    sys_safe_access_disable();
    sys_safe_access_enable();
    R8_RTC_MODE_CTRL |= RB_RTC_TRIG_EN;
    sys_safe_access_disable();
    PFIC_EnableIRQ(RTC_IRQn);
    PWR_PeriphWakeUpCfg(ENABLE, RB_SLP_GPIO_WAKE, Long_Delay);
}
