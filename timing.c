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

#include "timing.h"
#include "main.h"
#include "settings.h"
//#include "gpio.h"

// TODO what are we going to do after 49 days?
static volatile uint_fast32_t milliseconds = 0;

void setup_timing() {
    OpenCoreTimer(FOSC / 1000);
    mConfigIntCoreTimer(CT_INT_ON | CT_INT_PRIOR_1 | CT_INT_SUB_PRIOR_3);
}

void __ISR(_CORE_TIMER_VECTOR, IPL1SOFT) _CoreTimerHandler(void) {
    mCTClearIntFlag();
    UpdateCoreTimer(FOSC / 1000 / 2); // TODO why / 2?
    milliseconds++;
}

float get_time() {
    return milliseconds * 0.001;
}

uint_fast32_t get_milliseconds() {
    return milliseconds;
}
