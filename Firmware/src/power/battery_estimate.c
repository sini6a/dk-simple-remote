#include "battery.h"
/* CH583 datasheet table 15-2. Negative PGA modes include a DC offset.
 * Vref is nominal 1050mV, not a calibrated absolute reference. */
uint16_t battery_adc_to_mv(uint16_t raw, unsigned gain)
{
    int32_t value;
    if (raw > 4095 || gain > 2) return 0;
    if (gain == 0) value = ((int32_t)raw - 1536) * 1050 / 512;
    else if (gain == 1) value = ((int32_t)raw - 1024) * 1050 / 1024;
    else value = (int32_t)raw * 1050 / 2048;
    return value > 0 ? (uint16_t)value : 0;
}
uint8_t battery_percent_from_mv(uint16_t mv)
{
    /* Energizer UltimateCR2032EU0920 blue background curve, 21C.
     * Digitized provenance and monotone-envelope policy: docs/data/.
     * Capacity is normalized to a conservative 2.2V cutoff, not rated mAh.
     * The long voltage plateau makes this an uncertain estimate. */
    static const uint16_t volts[] = {2200,2528,2775,2855,2864,2885,2893,2905,2918,2927,2933,2936,2937,2951,2980};
    static const uint8_t levels[] = {0,5,10,15,20,25,30,35,40,45,50,55,60,95,100};
    unsigned i;
    if (mv <= volts[0]) return 0;
    for (i=1;i<sizeof(levels);++i) if (mv < volts[i]) {
        unsigned p=levels[i-1]+(uint32_t)(mv-volts[i-1])*(levels[i]-levels[i-1])/(volts[i]-volts[i-1]);
        return (uint8_t)((p/5)*5);
    }
    return 100;
}
bool battery_estimate_update(battery_estimate_t *s, uint16_t mv)
{
    uint8_t next;
    bool down, up;
    if (mv<1700 || mv>3400) return false;
    s->millivolts=mv; /* Callers already pass the best of a quiet burst. */
    next=battery_percent_from_mv(mv);
    if (!s->valid) {
        s->percent=next; s->valid=true; s->has_pending=false;
        return true;
    }
    down=next<s->percent;
    up=next>=s->percent+BATTERY_RISE_POINTS;
    if (!down && !up) { s->has_pending=false; return false; }
    if (s->has_pending && (down ? s->pending<s->percent :
                           s->pending>=s->percent+BATTERY_RISE_POINTS)) {
        /* Confirmed: move only as far as the less extreme of the two. */
        s->percent=down ? (next>s->pending ? next : s->pending) :
                          (next<s->pending ? next : s->pending);
        s->has_pending=false;
        return true;
    }
    s->pending=next; s->has_pending=true; /* One dip or spike is ignored. */
    return false;
}
