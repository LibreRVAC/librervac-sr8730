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

#include "mulpinout.h"
#include "gpio.h"

#define I_MULSEL(multiplexer, pin)                                     \
    {                                                                   \
    DIGPIN(PIN_ ## multiplexer ## _A, (((pin) & 0b001) > 0 ? on : off)); \
    DIGPIN(PIN_ ## multiplexer ## _B, (((pin) & 0b010) > 0 ? on : off));  \
    DIGPIN(PIN_ ## multiplexer ## _C, (((pin) & 0b100) > 0 ? on : off));   \
    }

#define I_MULPIN(multiplexer, pin, value)    \
    {                                         \
    I_MULSEL(multiplexer, pin);                \
    DIGPIN(PIN_ ## multiplexer ## _X, (value)); \
    }


#define MULSEL(mulpin)        I_MULSEL(mulpin) // mulpin is two args
#define MULPIN(mulpin, value) I_MULPIN(mulpin, (value)) // mulpin is two args
