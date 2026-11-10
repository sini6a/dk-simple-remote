#include "CONFIG.h"
#include "app_config.h"
#include "power_manager.h"
static uint8_t boot_dcdc;
static uint8_t dcdc_flags(void)
{
    return ((R16_POWER_PLAN & RB_PWR_DCDC_EN) ? 1 : 0) |
           ((R16_POWER_PLAN & RB_PWR_DCDC_PRE) ? 2 : 0) |
           ((R16_AUX_POWER_ADJ & RB_DCDC_CHARGE) ? 4 : 0);
}
void power_manager_read_status(uint8_t out[8])
{
    uint16_t gates=(uint16_t)R32_SLEEP_CONTROL;
    out[0]=1;
    out[1]=(uint8_t)(int8_t)APP_TX_POWER_DBM;
    out[2]=BLE_TX_POWER;
    out[3]=dcdc_flags();
    out[4]=boot_dcdc;
    out[5]=(uint8_t)gates; out[6]=(uint8_t)(gates>>8);
    out[7]=((R8_ADC_CFG & RB_ADC_POWER_ON) ? 1 : 0) |
           ((R8_ADC_CFG & RB_ADC_BUF_EN) ? 2 : 0) |
           ((R8_TEM_SENSOR & RB_TEM_SEN_PWR_ON) ? 4 : 0) |
           ((R8_TKEY_CFG & RB_TKEY_PWR_ON) ? 8 : 0);
}
void power_manager_init(void)
{
    /* Same order as HID_Consumer/APP/hidconsumer_main.c. 22uH fitted. */
    PWR_DCDCCfg(ENABLE);
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    /* Unused application peripherals only. BLE, RTC, RAM and timers remain
     * available to the closed-source stack and calibration machinery. */
    PWR_PeriphClkCfg(DISABLE, BIT_SLP_CLK_UART0 | BIT_SLP_CLK_UART1 |
        BIT_SLP_CLK_UART2 | BIT_SLP_CLK_UART3 | BIT_SLP_CLK_SPI0 |
        BIT_SLP_CLK_PWMX | BIT_SLP_CLK_USB | (RB_SLP_CLK_I2C << 8) |
        (RB_SLP_CLK_SPI1 << 8) | (RB_SLP_CLK_USB2 << 8));
    R8_ADC_CFG=0;
    R8_TEM_SENSOR &= ~RB_TEM_SEN_PWR_ON;
    R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
    boot_dcdc=dcdc_flags();
}
