#include "keymap.h"
#include "app_config.h"
/* Physical buttons 0..APP_BUTTON_COUNT-1, then virtual long-press keys. */
static const uint16_t pages[APP_KEY_COUNT] = APP_KEY_PAGES;
uint16_t keymap_page(unsigned button)
{
    return button < APP_KEY_COUNT ? pages[button] : 0;
}
static const uint16_t usages[APP_KEY_COUNT] = APP_KEY_USAGES;
uint16_t keymap_usage(unsigned button)
{
    return button < APP_KEY_COUNT ? usages[button] : 0;
}
