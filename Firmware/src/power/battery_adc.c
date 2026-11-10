#include "CH58x_common.h"
#include "battery.h"
/* Same offset-test sequence as ADC_DataCalib_Rough, with bounded waits.
 * A short IRQ critical section serializes with HAL_GetInterTempValue. */
static bool convert(uint16_t *value)
{
    unsigned limit=4096;
    R8_ADC_CONVERT=RB_ADC_START;
    while (R8_ADC_CONVERT & RB_ADC_START) if (!--limit) return false;
    *value=R16_ADC_DATA & RB_ADC_DATA;
    return true;
}
static bool burst(unsigned gain, uint16_t *raw)
{
    uint32_t zero=0, sum=0;
    uint16_t v;
    int32_t corrected;
    unsigned i;
    R8_ADC_CFG=RB_ADC_POWER_ON | RB_ADC_BUF_EN | (gain<<4);
    R8_ADC_CHANNEL=1;
    R8_ADC_CFG |= RB_ADC_OFS_TEST;
    if (!convert(&v)) return false;
    for(i=0;i<16;++i) {
        if (!convert(&v)) return false;
        zero+=(~v)&RB_ADC_DATA;
    }
    R8_ADC_CFG &= ~RB_ADC_OFS_TEST;
    R8_ADC_CHANNEL=CH_INTE_VBAT;
    if (!convert(&v)) return false; /* discard after channel change */
    for(i=0;i<8;++i) { if (!convert(&v)) return false; sum+=v; }
    corrected=(int32_t)((sum+4)/8)+2048-(int32_t)((zero+8)/16);
    if (corrected<0 || corrected>4095) return false;
    *raw=(uint16_t)corrected;
    return true;
}
bool battery_measure_mv(uint16_t *mv)
{
    unsigned long irq;
    uint16_t raw=0, value=0;
    bool ok;
    /* No other application ADC owner exists. Never interrupt an active transfer. */
    if (R8_ADC_CONVERT & RB_ADC_START) return false;
    SYS_DisableAllIrq(&irq);
    R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
    R8_TEM_SENSOR &= ~RB_TEM_SEN_PWR_ON;
    ADC_InterBATSampInit();
    ok=burst(0,&raw);
    if(ok) {
        value=battery_adc_to_mv(raw,0);
        /* Datasheet recommends -6dB below 2.9V, 0dB below 1.9V. */
        if(value<2900) {
            ok=burst(1,&raw);
            if(ok) value=battery_adc_to_mv(raw,1);
        }
        if(ok && value<1900) {
            ok=burst(2,&raw);
            if(ok) value=battery_adc_to_mv(raw,2);
        }
    }
    R8_ADC_CONVERT=0;
    R8_ADC_CFG=0; /* power, input buffer and offset test all off */
    R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
    R8_TEM_SENSOR &= ~RB_TEM_SEN_PWR_ON;
    SYS_RecoverIrq(irq);
    /* Gain thresholds above use the ADC's own scale (its input range);
     * the board correction applies to the reported voltage only. */
    if(ok) value=(uint16_t)(((uint32_t)value*APP_BATTERY_CAL_SCALE+5000UL)/10000UL);
    if(!ok || value<1700 || value>3400) return false;
    *mv=value;
    return true;
}
