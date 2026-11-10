#ifndef REMOTE_BATTERY_H
#define REMOTE_BATTERY_H
#include <stdint.h>
#include <stdbool.h>
/* percent changes only after two consecutive samples agree on the direction:
 * any drop, or a rise of BATTERY_RISE_POINTS (recovers from a bad first value).
 * pending holds the first, still unconfirmed, sample's percentage. */
typedef struct {
    uint16_t millivolts;
    uint8_t percent, pending;
    bool valid, has_pending;
} battery_estimate_t;
#define BATTERY_RISE_POINTS 10U
uint16_t battery_adc_to_mv(uint16_t raw, unsigned gain);
uint8_t battery_percent_from_mv(uint16_t mv);
bool battery_estimate_update(battery_estimate_t *state, uint16_t mv);
/* Short bounded burst; ADC/buffer/temperature/TouchKey are off on return. */
bool battery_measure_mv(uint16_t *mv);
#endif
