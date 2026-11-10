#include "CONFIG.h"
#include "app_config.h"
#include <assert.h>
#include <stdio.h>
static uint16_t plan,adj;
static uint32_t fake_clock_gates;
static uint8_t adc,sensor,touch;
#undef R16_POWER_PLAN
#undef R16_AUX_POWER_ADJ
#undef R32_SLEEP_CONTROL
#undef R8_ADC_CFG
#undef R8_TEM_SENSOR
#undef R8_TKEY_CFG
#define R16_POWER_PLAN plan
#define R16_AUX_POWER_ADJ adj
#define R32_SLEEP_CONTROL fake_clock_gates
#define R8_ADC_CFG adc
#define R8_TEM_SENSOR sensor
#define R8_TKEY_CFG touch
#include "../src/power/power_manager.c"
static unsigned order;
void PWR_DCDCCfg(FunctionalState state)
{
    assert(state==ENABLE && order++==0);
    plan|=RB_PWR_DCDC_EN|RB_PWR_DCDC_PRE; adj|=RB_DCDC_CHARGE;
}
void SetSysClock(SYS_CLKTypeDef clock)
{ assert(clock==CLK_SOURCE_PLL_60MHz && order++==1 && (plan&RB_PWR_DCDC_EN)); }
void PWR_PeriphClkCfg(FunctionalState state,uint16_t mask)
{
    assert(state==DISABLE && order++==2);
    assert(!(mask&(BIT_SLP_CLK_BLE|BIT_SLP_CLK_TMR0|BIT_SLP_CLK_TMR1|BIT_SLP_CLK_TMR2|BIT_SLP_CLK_TMR3)));
    assert((mask&BIT_SLP_CLK_UART1) && (mask&BIT_SLP_CLK_USB)); fake_clock_gates|=mask;
}
int main(void)
{
    uint8_t out[8];
    fake_clock_gates=0x80000000; adc=sensor=touch=255;
    power_manager_init(); assert(order==3 && (fake_clock_gates&0x80000000));
    power_manager_read_status(out);
    assert(out[0]==1 && (int8_t)out[1]==-5 && out[2]==LL_TX_POWEER_MINUS_5_DBM);
    assert(out[3]==7 && out[4]==7 && out[7]==0);
    assert(BUILD_UINT16(out[5],out[6])==(uint16_t)fake_clock_gates);
    plan &= ~RB_PWR_DCDC_EN;
    power_manager_read_status(out); assert(out[3]==6 && out[4]==7);
    adc=RB_ADC_POWER_ON|RB_ADC_BUF_EN; sensor=RB_TEM_SEN_PWR_ON; touch=RB_TKEY_PWR_ON;
    power_manager_read_status(out); assert(out[7]==15);
    puts("PASS: DC/DC before clock, live versus boot readback, preserved BLE/timer clocks, analog shutdown and encoded transmit level");
    return 0;
}
