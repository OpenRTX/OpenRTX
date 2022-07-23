#include <hwconfig.h>
#include "interfaces/delays.h"
#ifdef LED_PRESENT
#include "interfaces/gpio.h"
#endif
#include <stdint.h>
#include <state.h>
#include <stdbool.h>

uint16_t standby_toggle_tick = 0;
uint16_t standby_swap_tick = 0;
bool * standby_active = false;
bool led_on = false;
bool red_on = false;

// Runs every 5ms
void led_tick() {
    if (*standby_active && state.settings.standby_led) {
        standby_toggle_tick++;
        if (standby_toggle_tick % 40 == 0) {
            // 200ms passed, switch active LED
            red_on = !red_on;
        }
        standby_swap_tick++;
        if (standby_swap_tick % 1000 == 0 && !led_on)
        {
            // 5 seconds has passed, toggle LED on
            led_on = true;
        } else if (standby_swap_tick % 200 == 0)
        {
            // 1 second has passed, toggle LED off
            led_on = false;
        }

        #ifdef LED_PRESENT
        if(led_on) {
            if (red_on) {
                gpio_setPin(RED_LED);
                gpio_clearPin(GREEN_LED);
            } else {
                gpio_setPin(GREEN_LED);
                gpio_clearPin(RED_LED);
            }
        } else {
            gpio_clearPin(GREEN_LED);
            gpio_clearPin(RED_LED);
        }
        #endif
    }
}

void led_link_standby_state(bool * standby) {
    standby_active = standby;
}
