#include "CONFIG.h"
#include "ble_manager.h"
#include "buttons.h"
#include "power_manager.h"
#include "remote.h"
__HIGH_CODE __attribute__((noinline))
static void run(void)
{
    while (1) { remote_process_wake(); TMOS_SystemProcess(); }
}
int main(void)
{
    power_manager_init();
    buttons_gpio_init();
    ble_manager_init();
    remote_init();
    run();
}
