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

#include "motors.h"
#include "main.h"
#include "pinout.h"
#include "gpio.h"
#include "cord_motors.h"
#include "settings.h"
#include <math.h>
#include <stdint.h>

#define PWM_FREQUENCY 20000
#define TIMER_TICKS (FOSC / (PWM_FREQUENCY) / 1 * FPB_DIV)

// TODO we probably want to use “f” suffix here
#define DEADBAND_MAIN_BRUSH 0.65
#define DEADBAND_LEFT_WHEEL 0.65
#define DEADBAND_RIGHT_WHEEL 0.65

volatile int pwm_main_brush  = 0;
volatile int pwm_left_wheel  = 0;
volatile int pwm_right_wheel = 0;

void set_motor_throttle(cord_motor_type motor, float throttle) {
    // TODO PWM
    switch (motor) {
        case BRUSH_MAIN:
            DIGPIN(PIN_BRUSH_MAIN_PHASE, throttle >= 0.0 ? on : off); // on – forward
            if (throttle != 0)
                pwm_main_brush = (DEADBAND_MAIN_BRUSH + (1.0 - DEADBAND_MAIN_BRUSH)
                    * fabs(throttle)) * TIMER_TICKS;
            else
                pwm_main_brush = 0;
            break;
        case BRUSH_SIDES:
        case BRUSH_LEFT:
        case BRUSH_RIGHT:
            DIGPIN(PIN_BRUSH_SIDES, throttle > 0.5 ? on : off);
            break;
        case VACUUM:
            DIGPIN(PIN_VACUUM_ENABLE, throttle > 0.5 ? on : off);
            break;
        case WHEEL_LEFT:
            DIGPIN(PIN_WHEEL_LEFT_PHASE, throttle >= 0 ? on : off); // on – forward
            if (throttle != 0)
                pwm_left_wheel = (DEADBAND_LEFT_WHEEL + (1.0 - DEADBAND_LEFT_WHEEL)
                    * fabs(throttle)) * TIMER_TICKS;
            else
                pwm_left_wheel = 0;
            break;
        case WHEEL_RIGHT:
            DIGPIN(PIN_WHEEL_RIGHT_PHASE, throttle >= 0 ? on : off); // on – forward
            if (throttle != 0)
                pwm_right_wheel = (DEADBAND_RIGHT_WHEEL + (1.0 - DEADBAND_RIGHT_WHEEL)
                    * fabs(throttle)) * TIMER_TICKS;
            else
                pwm_right_wheel = 0;
            break;
        default:
            // TODO unknown motor?
            break;
    }
}

void setup_motors() {
    CloseTimer3();
    OpenTimer3(T3_ON | T3_PS_1_1 | T3_SOURCE_INT, TIMER_TICKS);
    OpenOC1(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE, 0, 0);
    OpenOC4(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE, 0, 0);
    OpenOC2(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE, 0, 0);
    SetDCOC1PWM(0); // wheel TODO which?
    SetDCOC4PWM(0); // wheel TODO which?
    SetDCOC2PWM(0); // main brush
    mT3SetIntPriority(3); // should be same as below
    mT3SetIntSubPriority(0);
    mT3ClearIntFlag();
    mT3IntEnable(1);
}

void __ISR(_TIMER_3_VECTOR, IPL3SOFT) T3Interrupt(void) {
    SetDCOC1PWM(pwm_left_wheel);
    SetDCOC4PWM(pwm_right_wheel);
    SetDCOC2PWM(pwm_main_brush);
    mT3ClearIntFlag();
}

/*
void __ISR(_INPUT_CAPTURE_1_VECTOR, ipl7) IC1Interrupt(void) {
    while (!mIC1CaptureReady()); // Wait for capture data to be ready
    mIC1ReadCapture(); // read capture
    mIC1ClearIntFlag(); // clear interrupt flag
}*/
