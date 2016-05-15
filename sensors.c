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

#include <stdint.h>
#include "main.h"

#include "multiplexer.h"
#include "settings.h"
#include "charging.h"
#include "leds.h"

// 38kHz PWM to make infrared stuff happy
// 2000 bits per second
// 5 32-bit packets per second
#define CHARGE_ME_SIGNAL 0b111111000110000110000110011000011L
#define CHARGE_ME_BITS   33
// ↑ Yes, that's a 33-bit value! That's what you *have* to send! WTF? Ask Samsung!
// ↓ It is mostly insensetive to changes of these values. Almost any will do.
#define CHARGE_ME_FREQ   5
#define IR_PWM_FREQ      38000   // Commonly used pwm frequency for IR stuff
#define TICKS_PER_BIT    ( (IR_PWM_FREQ) * 2 / 2000 )
#define TICKS_PER_PACKET ( (IR_PWM_FREQ) * 2 / (CHARGE_ME_FREQ) ) // including wait

void setup_sensors() {
    CloseTimer1();
    // TODO is Timer1 clocked from FPB?
    OpenTimer1(T1_ON | T1_PS_1_1 | T1_SOURCE_INT, FOSC / FPB_DIV / (IR_PWM_FREQ * 2));
    mT1SetIntPriority(3); // should be same as below
    mT1SetIntSubPriority(0);
    mT1ClearIntFlag();
    mT1IntEnable(1);
    MULSEL(PIN_IR_SENSOR_FRONT); // TODO
}

void __ISR(_TIMER_1_VECTOR, IPL3SOFT) T1Interrupt(void) {
    //static uint_fast8_t current_sensor = 0;
    //if (++current_sensor >= 7) // 7 sensors in total
    //    current_sensor = 0;
    // MULSEL(PIN_IC_SENSORS_X, current_sensor); // TODO
    if (is_docked()) {
        static uint_fast32_t ticks = 0;
        static uint_fast32_t current_bit = 0;

        current_bit = ticks / TICKS_PER_BIT;

        if (current_bit < CHARGE_ME_BITS &&
                CHARGE_ME_SIGNAL & (1 << (CHARGE_ME_BITS - current_bit - 1)))
            DIGPIN(PIN_IC_SENSORS_X_OUT, ticks % 2 == 0 ? on : off);
        else
            DIGPIN(PIN_IC_SENSORS_X_OUT, off);

        if (++ticks >= TICKS_PER_PACKET)
            ticks = 0;
    }
    leds_tick(); // TODO any better place to call it?
    mT1ClearIntFlag();
}
