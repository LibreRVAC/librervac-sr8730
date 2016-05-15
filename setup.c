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

#include "setup.h"
#include "settings.h"
#include "cord_hw.h"
#include "main.h"
#include "gpio.h"
#include "sound.h"
#include "uart.h"
#include "motors.h"
#include "battery.h"
#include "timing.h"
#include "sensors.h"

void setup_pins() {
    DDPCONbits.JTAGEN = 0; // disable jtag
    PMCONbits.ON = 0; // disable the PMP interface
    AD1CON1bits.ON = 1;
    //AD1PCFG = 0xFFFF;
    SYSTEMConfigPerformance(FOSC);
    CONFIGURE_PIN(PIN_ACCELGYRO_POWER, OUTPUT);

    /*
    CONFIGURE_PIN(PIN_CONNECTION_SCK, INPUT);
    CONFIGURE_PIN(PIN_CONNECTION_MISO, OUTPUT);
    CONFIGURE_PIN(PIN_CONNECTION_MOSI, INPUT);
     */
    CONFIGURE_PIN(PIN_CONNECTION_RX, INPUT);
    CONFIGURE_PIN(PIN_CONNECTION_TX, OUTPUT);

    CONFIGURE_PIN(PIN_BUTTONS, INPUT);
    CONFIGURE_PIN(PIN_USERBUTTON_INTERRUPT, INPUT);
    CONFIGURE_PIN(PIN_SENSOR_BOTTOM_MIDDLE, INPUT);

    CONFIGURE_PIN(PIN_BRUSH_SIDES, OUTPUT);
    CONFIGURE_PIN(PIN_VACUUM_ENABLE, OUTPUT);

    CONFIGURE_PIN(PIN_BUMPER_SWITCH_LEFT, INPUT);
    CONFIGURE_PIN(PIN_BUMPER_SWITCH_RIGHT, INPUT);

    CONFIGURE_PIN(PIN_WHEEL_LIFT_LEFT, INPUT);
    CONFIGURE_PIN(PIN_WHEEL_LIFT_RIGHT, INPUT);

    // CONFIGURE_PIN(PIN_WHEEL_LEFT_ENABLE, OUTPUT); // // commented out because configured as OC
    CONFIGURE_PIN(PIN_WHEEL_LEFT_MODE, OUTPUT);
    CONFIGURE_PIN(PIN_WHEEL_LEFT_PHASE, OUTPUT);

    // CONFIGURE_PIN(PIN_WHEEL_RIGHT_ENABLE, OUTPUT); // commented out because configured as OC
    CONFIGURE_PIN(PIN_WHEEL_RIGHT_MODE, OUTPUT);
    CONFIGURE_PIN(PIN_WHEEL_RIGHT_PHASE, OUTPUT);

    // CONFIGURE_PIN(PIN_BRUSH_MAIN_ENABLE, OUTPUT); // commented out because configured as OC
    CONFIGURE_PIN(PIN_BRUSH_MAIN_DECAY, OUTPUT);
    CONFIGURE_PIN(PIN_BRUSH_MAIN_PHASE, OUTPUT);

    CONFIGURE_PIN(PIN_BEEPER, OUTPUT);

    CONFIGURE_PIN(PIN_IC16_A, OUTPUT);
    CONFIGURE_PIN(PIN_IC16_B, OUTPUT);
    CONFIGURE_PIN(PIN_IC16_C, OUTPUT);
    // CONFIGURE_PIN(PIN_IC16_X, OUTPUT); // commented out because configured as OC

    CONFIGURE_PIN(PIN_IC_SENSORS_A, OUTPUT);
    CONFIGURE_PIN(PIN_IC_SENSORS_B, OUTPUT);
    CONFIGURE_PIN(PIN_IC_SENSORS_C, OUTPUT);
    CONFIGURE_PIN(PIN_IC_SENSORS_X_OUT, OUTPUT);
    CONFIGURE_PIN(PIN_IC_SENSORS_X_IN, OUTPUT);

    CONFIGURE_PIN(PIN_ADAPTER_DETECT, INPUT);

    CONFIGURE_PIN(PIN_LEDS_POWER_ALL, OUTPUT);
    CONFIGURE_PIN(PIN_LEDS_POWER_BATTERY, OUTPUT);

    CONFIGURE_PIN(PIN_LED_0, OUTPUT);
    CONFIGURE_PIN(PIN_LED_1, OUTPUT);
    CONFIGURE_PIN(PIN_LED_2, OUTPUT);

    CONFIGURE_PIN(PIN_LED_3, OUTPUT);
    CONFIGURE_PIN(PIN_LED_4, OUTPUT);

    //    DIGPIN(PIN_LEDS_POWER_BATTERY, on);
    /*
        DIGPIN(PIN_LED_0, on);
        DIGPIN(PIN_LED_1, on);
        DIGPIN(PIN_LED_2, on);
        DIGPIN(PIN_LED_3, on);
        DIGPIN(PIN_LED_4, on);
     */

    DIGPIN(PIN_ACCELGYRO_POWER, on);

    DIGPIN(PIN_BRUSH_SIDES, off);
    DIGPIN(PIN_VACUUM_ENABLE, off);

    DIGPIN(PIN_WHEEL_LEFT_PHASE, on); // on – forward
    DIGPIN(PIN_WHEEL_LEFT_MODE, off); // on – fast stop
    DIGPIN(PIN_WHEEL_LEFT_ENABLE, off);

    DIGPIN(PIN_WHEEL_RIGHT_PHASE, on); // on – forward
    DIGPIN(PIN_WHEEL_RIGHT_MODE, off); // on – fast stop
    DIGPIN(PIN_WHEEL_RIGHT_ENABLE, off);

    DIGPIN(PIN_BRUSH_MAIN_PHASE, on); // on – forward
    DIGPIN(PIN_BRUSH_MAIN_DECAY, off); // on – fast stop
    // DIGPIN(PIN_BRUSH_MAIN_ENABLE, off); // commented out because configured as OC

    //DIGPIN(PIN_BEEPER, off);

    //OpenCapture1(IC_EVERY_RISE_EDGE | IC_INT_1CAPTURE | IC_CAP_32BIT | IC_FEDGE_RISE | IC_ON);
}

void setup() {
    INTEnableSystemMultiVectoredInt();
    setup_timing();
    setup_pins();
    setup_sound();
    //setup_spi();
    setup_uart();
    setup_motors();
    setup_battery();
    setup_sensors();
}
