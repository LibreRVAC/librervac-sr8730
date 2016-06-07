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

// PIC32MX360F256L Configuration Bit Settings

// 'C' source line config statements

#include <xc.h>

// DEVCFG3
// USERID = No Setting

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (4x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (15x Multiplier)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)

// DEVCFG1
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = HS             // Primary Oscillator Configuration (HS osc mode)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_1           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)


#include "main.h"
#include "cord_init.h"
#include "cord_hw.h"
#include "cord_connection.h"
#include "bumper.h"

#include "gpio.h"
#include "mulpinout.h"
#include "multiplexer.h"
#include <math.h>

#include "setup.h"
#include "timing.h"
#include "sound.h"
#include "motors.h"
#include "uart.h"
#include "battery.h"
#include "charging.h"

#include <stdint.h>
#include "cord_sound.h"
#include "leds.h"

#define FIRMWARE_VERSION "0.0.1"

void step() {
    bumper_step();
    uart_process_edges();
    charging_step();
    leds_step();

    static uint_fast32_t dock_detect_debounce = 0;
    if (READPIN(PIN_ADAPTER_DETECT)
            && dock_detect_debounce + 300 < hw_get_time_ms()) {
        dock_connect();
        dock_detect_debounce = hw_get_time_ms();
    } else if (!READPIN(PIN_ADAPTER_DETECT)
            && dock_detect_debounce + 300 < hw_get_time_ms()) {
        dock_disconnect(); // TODO we're overcalling it a bit. It's OK but could be better
        dock_detect_debounce = hw_get_time_ms();
    }

    /*
    static uint_fast32_t lastms = 0;
    if (hw_get_time_ms() % 250 == 0 && lastms < hw_get_time_ms()) {
        lastms = hw_get_time_ms();
        char buf[140];
        sprintf(buf, "{\"c\":\"log\", \"str\":\"ms=%d Bat T=%fC; V1=%fV; V2=%fV; d=%fV; curr=%fA\"}",
                hw_get_time_ms(),
                (double) get_battery_temperature(),
                (double) get_battery_voltage_before_shunt(),
                (double) get_battery_voltage_after_shunt(),
                (double) (get_battery_voltage_before_shunt() - get_battery_voltage_after_shunt()),
                (double) get_battery_current()
                );
        for (int i = 0; buf[i] != 0 && i < 140; i++)
            cord_connection_output_put(buf[i]);
        cord_connection_output_put(0);
    }*/

    static uint_fast32_t lastms2 = 0;
    if (hw_get_time_ms() % 5000 == 0 && lastms2 < hw_get_time_ms()) {
        //cord_buzzer_queue_beep(300, 0.05, 0.1);
        lastms2 = hw_get_time_ms();
    }
    //DIGPIN(PIN_BRUSH_SIDES, on);
    //DIGPIN(WHEEL_LEFT_ENABLE, READPIN(WHEEL_LIFT_LEFT) ? off : on);
    //DIGPIN(WHEEL_RIGHT_ENABLE, READPIN(WHEEL_LIFT_RIGHT) ? off : on);
    /*if (z % 10000 == 0) {
        DIGPIN(BEEPER, on);
    }
    if (z % 20000 == 0) {*/
    //if (milliseconds % 2 == 0) {
    //    MULPIN(PIN_SPEAKER, on);
    //} else {
    //    MULPIN(PIN_SPEAKER, off);
    //}
    /*
    if (READPIN(BUMPER_SWITCH_RIGHT) == on) {
        DIGPIN(BEEPER, on);
    } else {
        DIGPIN(BEEPER, off);
    }*/


    /*
    DIGPIN(WHEEL_LEFT_PHASE, READPIN(BUMPER_SWITCH_LEFT) ? on : off);
    DIGPIN(WHEEL_RIGHT_PHASE, READPIN(BUMPER_SWITCH_LEFT) ? on : off);

    DIGPIN(BRUSH_MAIN_ENABLE, READPIN(BUMPER_SWITCH_RIGHT) ? on : off);
    DIGPIN(BRUSH_SIDES, READPIN(BUMPER_SWITCH_RIGHT) ? on : off);
    DIGPIN(VACUUM_ENABLE, READPIN(BUMPER_SWITCH_RIGHT) ? on : off);
     */
}

const char* get_version() {
    return FIRMWARE_VERSION;
}

int main() {
    set_callback_get_version(&get_version);
    set_callback_setup(&setup);
    set_callback_step(&step);
    set_callback_get_time(&get_time);
    set_callback_get_time_ms(&get_time_ms);
    set_callback_process_beeps(&process_beeps);
    set_callback_set_motor_throttle(&set_motor_throttle);
    set_callback_get_battery_status(&get_battery_status);
    set_callback_get_battery_voltage(&get_battery_voltage_after_shunt);
    set_callback_get_battery_current(&get_battery_current);
    set_callback_get_battery_temperature(&get_battery_temperature);
    cord_event_init();
    return 0;
}

void toggle_interrupts(bool enable) {
    static int int_status = 0; // TODO is it a good default?
    if (enable)
        int_status = INTDisableInterrupts();
    else
        INTRestoreInterrupts(int_status);
}
