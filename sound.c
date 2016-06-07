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

#include "main.h"
#include "sound.h"
#include "cordlib/cord_sound.h"
#include "timing.h"
#include "settings.h"
#include "pinout.h"

#include "gpio.h" //
#include "multiplexer.h"
#include "cord_hw.h" //

volatile uint_fast16_t duty_cycle;
volatile uint_fast32_t stop_time;

void setup_sound() {
    MULPIN(PIN_SPEAKER, off); // required to set A B C correctly
}

void handle_beep() {
    Beep* beep = cord_buzzer_get_beep();
    if (!beep) { // OK, done for now
        SetDCOC5PWM(0);
        mT2IntEnable(0);
        DIGPIN(PIN_BEEPER, off);
        return;
    }
    if (beep->frequency != 0)
        DIGPIN(PIN_BEEPER, on);
    OpenOC5(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE, 0, 0);
    stop_time = beep->duration * 1000 + hw_get_time_ms();
    uint_fast16_t ticks = FOSC / beep->frequency / 8 * FPB_DIV; // / prescaler * FPBDIV
    OpenTimer2(T2_ON | T2_PS_1_8 | T2_SOURCE_INT, ticks);
    duty_cycle = beep->volume * ticks / 2;
    SetDCOC5PWM(duty_cycle);
}

void process_beeps() {
    if (mT2GetIntEnable()) // TODO what if it is changed right after that?
        return; // already PWM-ing
    handle_beep();
    mT2SetIntPriority(3); // should be same as below
    mT2SetIntSubPriority(1);
    mT2ClearIntFlag();
    mT2IntEnable(1);
}

void __ISR(_TIMER_2_VECTOR, IPL3SOFT) T2Interrupt(void) {
    if (hw_get_time_ms() >= stop_time) {
        handle_beep(); // time for a new value
    } else {
        SetDCOC5PWM(duty_cycle); // just continue
    }
    mT2ClearIntFlag();
}
