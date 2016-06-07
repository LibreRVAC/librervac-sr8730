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
#include "settings.h"
#include "motors.h"
#include "battery.h"
#include "cord_hw.h"
#include "cord_sound.h"
#include <stdbool.h>

// We are using both ΔT and ΔV to determine the charged state.
#define CHARGED_DELTA_T 0.5 // per minute

// 1A current is what original firmware does, so that's what we are going
// to do as well.
#define CHARGING_MAX_CURRENT 0.950 // allow some slop
// ↑ although it is a bit ugly, it makes sense when you consider that to
// reject noise we gather adc data only when we are not actively charging
// the battery, so the measured current is lower by a tiny bit.

#define CHARGING_SUB_PWM 20000
#define CHARGING_PWM        50
#define CHARGING_DUTY_MAX 1000
#define CHARGING_DUTY_INITIAL ( CHARGING_DUTY_MAX / 30 )
#define CHARGING_DUTY_ANTINOISE CHARGING_DUTY_INITIAL / 3

volatile unsigned int charging_duty_cycle = 0;

bool configured = false;
bool docked = false;
volatile bool noisy = false;

bool detect_charged(bool reset) {
    static uint_fast32_t then = 0;
    static float voltage_then = 0;
    static float temperature_then = 0;
    uint_fast32_t now = hw_get_time_ms();

    if (reset) {
        then = hw_get_time_ms();
        voltage_then = get_battery_voltage_after_shunt();
        temperature_then = get_battery_temperature();
        return false;
    }

    if (then + 60000 < now) { // check approximately every minute
        float voltage_now = get_battery_voltage_after_shunt(); // after shunt it is a bit less noisy
        if (voltage_now < voltage_then) {
            return true;
        }
        voltage_then = voltage_now;

        float temperature_now = get_battery_temperature();
        if (temperature_now > temperature_then + CHARGED_DELTA_T / 2) { // TODO / 2 because we do not approximate properly
            return true;
        }
        temperature_then = temperature_now;

        then = now;
    }
    return false;
}

// Things may be a bit flappy when we are actively charging the battery.

bool is_noisy() {
    return noisy;
}

bool is_charging() {
    return configured;
}

bool is_docked() {
    return docked;
}

void setup_charging() {
    // We will be acting similarly to the original firmware.
    // When adapter is connected, it starts charging (PWM
    // with 160 Hz ≈1.5% duty cycle, this results in about 250mA current).
    // It is unclear why they do that. May even be a bug. This happens for
    // around one minute.
    // Then they change PWM frequency to 20kHz, duty cycle ≈3.33% but they are
    // not doing that constantly. Instead, they have a 50Hz PWM on top of that.
    // That is, it sends 20kHz wave for a while, then does nothing, then repeats
    // this pattern with 50Hz frequency.
    // They are slowly increasing the duty cycle of this 50Hz wave, which
    // results in current going from almost zero to exactly 1A.
    // Once charged, it attempts to compensate for leaks. In order
    // to do that, every 2 minutes they charge the battery for 20 seconds with
    // ≈40mA current.
    // This trickle charge strategy is probably flawed. It seems like there is
    // about 15mA discharge current when the device is sitting on a dock.
    // 45mA charging for 20 seconds does not compensate 15mA discharge for
    // 100 seconds, and more so if you add self-discharge.
    // Interestingly, if you dock your robot while it is fully charged, then
    // once it is charged the device will start beeping annoyingly (“I am
    // charged, get me out of here”). So they are not interested in keeping
    // the battery charged, all they wanted to do is make sure that the battery
    // does not go dead if you leave your cleaner on a charger for a couple of
    // days.
    //
    // We will use the same 20kHz/50Hz strategy when charging, we will leave
    // weird 160 Hz thing as it does not seem to bring any profit.
    // We will handle self-discharge a bit more smartly (TODO)
    //
    // Timer3 will produce a nice 20kHz wave. In ISR of Timer5 we
    // disable/enable Timer3
    CloseTimer3();
    mT3IntEnable(0); // we will not be using an interrupt
    OpenTimer3(T3_ON | T3_PS_1_1 | T3_SOURCE_INT, FOSC / FPB_DIV / CHARGING_SUB_PWM);
    OpenOC3(OC_ON | OC_TIMER3_SRC | OC_PWM_FAULT_PIN_DISABLE, 0, 0);

    unsigned int ticks = FOSC / FPB_DIV / 8 / (CHARGING_PWM * CHARGING_DUTY_MAX);
    OpenTimer5(T5_ON | T5_PS_1_8 | T5_SOURCE_INT, ticks); // 50 * CHARGING_MAX_DUTY Hz
    mT5SetIntPriority(4);
    mT5ClearIntFlag();
}

void charging_start() {
    if (configured)
        return;

    configured = true;
    // it is important that this ↑ is before this ↓
    battery_calibrate_current();
    charging_duty_cycle = CHARGING_DUTY_INITIAL;
    setup_charging();
    detect_charged(true);
    mT5IntEnable(1);
}

void charging_stop() {
    if (!configured)
        return;
    mT5IntEnable(0);
    SetDCOC3PWM(0);
    CloseTimer3();
    configured = false;
    // We are using Timer3 for both motors and charging (we have to),
    // so here we are enabling the motors again.
    setup_motors();
}

void charging_step() {
    if (!configured)
        return;
    static uint_fast32_t then = 0;
    uint_fast32_t now = hw_get_time_ms();
    if (then + 30 < now) { // correct approximately every 200 ms
        if (get_battery_current() > CHARGING_MAX_CURRENT) {
            if (charging_duty_cycle > CHARGING_DUTY_INITIAL)
                charging_duty_cycle--;
        } else if (charging_duty_cycle <= CHARGING_DUTY_MAX)
            charging_duty_cycle++;
        // ↑ it's OK if it wiggles a little bit
        then = now;
    }
    if (detect_charged(false)) {
        cord_buzzer_queue_beep(1000, 0.1, 0.3);
        cord_buzzer_queue_beep(2000, 0.1, 0.3);
        charging_stop();
    }
}

void dock_connect() {
    // TODO right now we may die right on the dock
    if (!docked && get_battery_status() < 0.85) // TODO better value
        charging_start();
    if (!docked) {
        cord_buzzer_queue_beep(600, 0.1, 0.3);
        cord_buzzer_queue_beep(800, 0.1, 0.3);
    }
    docked = true;
}

void dock_disconnect() {
    if (!docked)
        return;
    charging_stop();
    docked = false;
    cord_buzzer_queue_beep(800, 0.1, 0.3);
    cord_buzzer_queue_beep(700, 0.1, 0.3);
    cord_buzzer_queue_beep(600, 0.1, 0.3);
}

void __ISR(_TIMER_5_VECTOR, IPL4SOFT) T5Interrupt(void) {
    static int counter = 0;
    if (++counter >= CHARGING_DUTY_MAX) {
        counter = 0;
        SetDCOC3PWM(0xFFFF / 30); // charging enable
    }
    if (counter >= charging_duty_cycle) {
        SetDCOC3PWM(0); // charging disable
    }
    // | \/\/\noise/\/\/..[....no noise....].. |
    noisy = !(charging_duty_cycle + CHARGING_DUTY_ANTINOISE < counter
            && counter < CHARGING_DUTY_MAX - CHARGING_DUTY_ANTINOISE);
    mT5ClearIntFlag();
}
