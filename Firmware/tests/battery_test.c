#include <assert.h>
#include <stdio.h>
#include "battery.h"
int main(void)
{
    battery_estimate_t b={0};
    unsigned mv,previous=0;
    assert(battery_adc_to_mv(3072,0)==3150);
    assert(battery_adc_to_mv(3072,1)==2100);
    assert(battery_adc_to_mv(2048,2)==1050);
    assert(battery_adc_to_mv(1000,0)==0);
    assert(battery_adc_to_mv(4096,0)==0);
    for(mv=1700;mv<=3400;++mv) {
        unsigned p=battery_percent_from_mv(mv);
        assert(p<=100 && p>=previous && p%5==0); previous=p;
    }
    assert(!battery_estimate_update(&b,0) && !b.valid);
    assert(battery_estimate_update(&b,3000) && b.valid && b.percent==100);
    assert(!battery_estimate_update(&b,2995) && b.percent==100);
    /* A single dip is ignored and a normal reading clears it. */
    assert(!battery_estimate_update(&b,2600) && b.percent==100 && b.has_pending);
    assert(!battery_estimate_update(&b,3000) && b.percent==100 && !b.has_pending);
    /* Two lows in a row confirm; it moves only to the less extreme one. */
    assert(battery_percent_from_mv(2800)==10 && battery_percent_from_mv(2600)==5);
    assert(!battery_estimate_update(&b,2800) && b.percent==100);
    assert(battery_estimate_update(&b,2600) && b.percent==10 && b.millivolts==2600);
    /* Rises need BATTERY_RISE_POINTS, confirmed twice (recovers a bad value). */
    assert(battery_percent_from_mv(2950)==90 && battery_percent_from_mv(2840)==10);
    assert(!battery_estimate_update(&b,2840) && b.percent==10 && !b.has_pending);
    assert(!battery_estimate_update(&b,2950) && b.percent==10);
    assert(battery_estimate_update(&b,2950) && b.percent==90);
    assert(battery_percent_from_mv(2951)==95); /* +5: below the rise threshold. */
    assert(!battery_estimate_update(&b,2951) && !battery_estimate_update(&b,2951) && b.percent==90);
    /* A pending drop followed by a spike does not confirm either direction. */
    assert(!battery_estimate_update(&b,2600) && !battery_estimate_update(&b,3300) && b.percent==90);
    assert(!battery_estimate_update(&b,2600) && b.percent==90);
    assert(battery_estimate_update(&b,2600) && b.percent==5);
    assert(!battery_estimate_update(&b,4000) && b.percent==5);
    puts("PASS: ADC PGA offset scaling, monotonic bounded estimate, invalid readings, two-sample confirmation of drops and rises");
    return 0;
}
