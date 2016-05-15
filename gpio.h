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

#pragma once

#include "pinout.h"
#include <p32xxxx.h>

#define on (1) // lowercase because of the conflicts
#define off (0)
#define LOW 0
#define HIGH 1
#define INPUT 1
#define OUTPUT 0

#define PIN(name) name ## _PIN
#define PORT(name) name ## _PORT
#define ACTIVE(name) name ## _ACTIVE

#define I_BITS_LHS_EVAL2(reg, port, pin) reg ## port ## bits.reg ## port ## pin
#define I_BITS_LHS_EVAL1(reg, port, pin) I_BITS_LHS_EVAL2(reg, port, pin)
#define I_BITS_LHS(reg, name)            I_BITS_LHS_EVAL1(reg, PORT(name), PIN(name)) // like LATXbits.LATXY

#define I_SETREG_EVAL(lhs, rhs)       lhs = rhs
#define ACTIVE_GET(value, activehigh) ((activehigh) == 1 ? (value) : !(value))

#define DIGPIN(name, value)           I_SETREG_EVAL(I_BITS_LHS(LAT, name), \
                                                    ACTIVE_GET(value, ACTIVE(name))) // like LATXbits.LATXY = Z

// This piece of mess is supposed to protect us against pins that are
// configured for ADC by default. Perhaps we should just turn off ADC on all
// pins before doing something? (TODO)
#define I_NO_AN_EVAL2(macro, pin) macro(pin)
#define I_NO_AN_EVAL(port, pin) I_NO_AN_EVAL2(I_NO_AN_ ## port, pin)
#define I_NO_AN(port, pin) I_NO_AN_EVAL(port, pin)
#define I_NO_AN_A(pin)
#define I_NO_AN_B(pin) AD1PCFGbits.PCFG ## pin = 1;
#define I_NO_AN_C(pin)
#define I_NO_AN_D(pin)
#define I_NO_AN_E(pin)
#define I_NO_AN_F(pin)
#define I_NO_AN_G(pin)
#define CONFIGURE_PIN(name, value) {            \
    /* like LATXbits.LATXY = Z */                \
    I_SETREG_EVAL(I_BITS_LHS(TRIS, name), value); \
    /* like AD1PCFGbits.PCFG3 = 0 */               \
    I_NO_AN(PORT(name), PIN(name))                  \
}

#define I_READBITS_LHS_EVAL2(port, pin)  PORT ## port ## bits.R ## port ## pin
#define I_READBITS_LHS_EVAL1(port, pin)  I_READBITS_LHS_EVAL2(port, pin)
#define I_READBITS_LHS(name)             I_READBITS_LHS_EVAL1(PORT(name), PIN(name)) // like PORTXbits.RXY
#define I_GETREG_EVAL(lhs, rhs)          lhs == rhs
#define READPIN(name)                    (I_GETREG_EVAL(I_READBITS_LHS(name), ACTIVE(name))) // like PORTXbits.RXY == Z

//#define mPORTBSetPinsDigitalIn(_inputs)     (TRISBSET = (unsigned int)(_inputs),  AD1PCFGSET = (unsigned int)(_inputs) )
//if (' ## PORT(name) ## ' == 'B') { AD1PCFGbits.PCFG ## PIN(name) = 0; }
