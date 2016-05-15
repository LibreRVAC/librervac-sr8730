/*
 * This file is part of the LibreRVAC project
 *
 * Copyright © 2015-2016
 *     Aleks-Daniel Jakimenko-Aleksejev <alex.jakimenko@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cordlib/cord_leds.h"
#include "leds.h"
#include "gpio.h"
#include "cord_hw.h"
#include "battery.h"
#include "charging.h"
#include <stddef.h>

/// This function has to be ran 2 × 38_000 times per second.
/// The reason is that we have to share the same timer with infrared sensors,
/// one of which has to shine with 38kHz frequency to make docking
/// station happy.
#define LEDS_MAIN_FREQ ( 38000 * 2 )
/// There is no practical reason to run too often
#define LEDS_DIVIDER 1000
#define LEDS_TICKS_PER_SECOND ( LEDS_MAIN_FREQ / LEDS_DIVIDER )

// TODO Perhaps we should wrap around cordlib struct?
Led leds[QUASI_LED_LAST];

void leds_step() {
    static uint_fast32_t last_step = 0;
    leds[LED_HOME].brighteness = READPIN(PIN_ADAPTER_DETECT) ? 255 : 0;
    //leds[LED_HOME].brighteness = is_docked() ? 255 : 0;
    leds[LED_REPEAT].brighteness = is_charging() ? 255 : 0;

    if (hw_get_milliseconds() % 50 == 0 && hw_get_milliseconds() != last_step) { // TODO this is getting repetetive. Write a macro
        last_step = hw_get_milliseconds();
        float charge = get_battery_status() * 3;
        uint_fast8_t values[3] = {0, 0, 0, };
        for (uint_fast8_t i = 0; i < 3 && charge > 0; i++) {
            values[i] = charge >= 1 ? 255 : charge * 255;
            charge -= 1;
        }
        leds[LED_BATTERY_0].brighteness = values[0];
        leds[LED_BATTERY_1].brighteness = values[1];
        leds[LED_BATTERY_2].brighteness = values[2];
    }
}

void leds_tick() {
    static uint_fast32_t ticks = 0;
    /*if (ticks % LEDS_DIVIDER != 0) {
        ticks++;
        return;
    }*/

    static uint_fast8_t current_brighteness[QUASI_LED_LAST] = {255, 0, 5, 20, 255, 0, 0, 0};
    //leds[0].blink_frequency = 5;
    //static uint_fast8_t direction[QUASI_LED_LAST];

    // TODO blinking
    if (ticks % 1000)
        for (size_t i = 0; i < QUASI_LED_LAST; i++) {
            current_brighteness[i] = leds[i].brighteness;
            //uint_fast32_t ticks_per_blink = LEDS_MAIN_FREQ / leds[i].blink_frequency;
            //current_brighteness[i] = (ticks % ticks_per_blink) * 255 / ticks_per_blink;
        }

#define LEDS_GROUPS 2 // TODO support 7-segment displays
    // ↓ Too high values introduce visible flickering, too low values cause
    // other groups to shine slightly.
#define LEDS_SWITCH_DIVIDER 2
    // TODO consider flipping many pins at once
    uint_fast8_t current_duty = (ticks / LEDS_SWITCH_DIVIDER) % 255;
    switch (((ticks / LEDS_SWITCH_DIVIDER) % (LEDS_GROUPS * 255)) / 255) {
        case 0:
            DIGPIN(PIN_LEDS_POWER_ALL, off);
            DIGPIN(PIN_LEDS_POWER_BATTERY, off);
            // ↓ This is not cool, but perhaps this is the easiest way right now? (TODO)
            DIGPIN(PIN_LED_0, current_brighteness[0] > current_duty); // LED_DONE
            DIGPIN(PIN_LED_1, current_brighteness[1] > current_duty); // LED_AUTO
            DIGPIN(PIN_LED_2, current_brighteness[2] > current_duty); // LED_SPOT
            DIGPIN(PIN_LED_3, current_brighteness[3] > current_duty); // LED_REPEAT
            DIGPIN(PIN_LED_4, current_brighteness[4] > current_duty); // LED_HOME
            DIGPIN(PIN_LEDS_POWER_ALL, on);
            break;
        case 1:
            DIGPIN(PIN_LEDS_POWER_ALL, off);
            DIGPIN(PIN_LEDS_POWER_BATTERY, off);
            DIGPIN(PIN_LED_0, off); // LED_DONE
            DIGPIN(PIN_LED_1, off); // LED_AUTO
            DIGPIN(PIN_LED_2, current_brighteness[5] > current_duty); // LED_BATTERY_0
            DIGPIN(PIN_LED_3, current_brighteness[6] > current_duty); // LED_BATTERY_1
            DIGPIN(PIN_LED_4, current_brighteness[7] > current_duty); // LED_BATTERY_2
            DIGPIN(PIN_LEDS_POWER_BATTERY, on);
            break;
    }
    ticks++;
}
