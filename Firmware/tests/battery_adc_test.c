#include <assert.h>
#define CAL(mv) (((mv)*APP_BATTERY_CAL_SCALE+5000UL)/10000UL)
#include <stdio.h>
#include "CH58x_common.h"
#include "battery.h"
uint8_t R8_ADC_CFG,R8_ADC_CHANNEL,R8_TKEY_CFG,R8_TEM_SENSOR;
uint16_t R16_ADC_DATA;
static uint8_t control;
static unsigned target=3100,timeout,locked,conversions,gains;
uint8_t *adc_control(void)
{
    if(control && !timeout) {
        unsigned gain=(R8_ADC_CFG>>4)&3;
        assert(locked && (R8_ADC_CFG&RB_ADC_POWER_ON));
        ++conversions; gains|=1U<<gain;
        if(R8_ADC_CFG&RB_ADC_OFS_TEST) R16_ADC_DATA=2047;
        else {
            assert(R8_ADC_CHANNEL==CH_INTE_VBAT);
            R16_ADC_DATA=gain==0 ? 1536+target*512/1050 :
                gain==1 ? 1024+target*1024/1050 : target*2048/1050;
        }
        control=0;
    }
    return &control;
}
void SYS_DisableAllIrq(unsigned long *s) { assert(!locked); locked=1; *s=1; }
void SYS_RecoverIrq(unsigned long s) { assert(s==1 && locked); locked=0; }
void ADC_InterBATSampInit(void) { R8_ADC_CFG=3; R8_ADC_CHANNEL=14; }
int main(void)
{
    uint16_t mv=0;
    assert(battery_measure_mv(&mv) && mv>=CAL(3097) && mv<=CAL(3102) && gains==1);
    assert(!locked && !R8_ADC_CFG && !R8_TKEY_CFG && !R8_TEM_SENSOR);
    target=2500; gains=0; assert(battery_measure_mv(&mv));
    assert(mv>=CAL(2497) && mv<=CAL(2502) && gains==3);
    target=1800; gains=0; assert(battery_measure_mv(&mv));
    assert(mv>=CAL(1797) && mv<=CAL(1802) && gains==7);
    timeout=1; assert(!battery_measure_mv(&mv));
    assert(!locked && !R8_ADC_CFG && control==0 && conversions>0);
    puts("PASS: calibrated ADC bursts, gain fallback, serialized access, bounded timeout and analog shutdown");
    return 0;
}
