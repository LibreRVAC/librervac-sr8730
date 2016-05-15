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

#include "spi.h"
#include "cord_sound.h"
#include "gpio.h"
#include "cord_connection.h"
#include "settings.h"
#include "main.h"
#include "timing.h"
#include "cord_hw.h"

// This is probably the most reasonable way to communicate with a linux board.
// The code below is for SPI slave bit-bang (Mode 3)
// Most linux boards cannot act as slaves, so we have to be a slave.
// None of the SPI/UART/I²C interfaces are available for us on that board, so
// we have to improvise.

#define BUFFER_SIZE 500

uint_fast8_t bits_left = 8; // TODO not 32
uint8_t byte = 0; ///< shift register

// TODO refactoring

volatile uint_fast8_t buffer[BUFFER_SIZE]; ///< Typical ring buffer
volatile uint_fast16_t buffer_head;  ///< pulling from here
volatile uint_fast16_t buffer_empty; ///< pushing here
volatile uint_fast16_t buffer_size;  // makes our life slightly easier
volatile uint_fast16_t buffer_read_head;

static inline uint_fast8_t spi_buffer_pull() {
    uint_fast8_t byte;
    if (buffer_size == 0) {
        byte = 0; // no more data
    } else {
        byte = buffer[buffer_head];
        buffer_size--;
    }
    if (buffer_head++ >= CORD_INPUT_BUFFER_SIZE)
        buffer_head = 0;
    return byte;
}

// For external use ↓

bool spi_buffer_put(uint_fast8_t byte) {
    //if (buffer_size >= BUFFER_SIZE) // whoops, no space left!
    //    return false; // TODO
    buffer[buffer_empty] = byte;
    buffer_size++;
    if (++buffer_empty >= BUFFER_SIZE)
        buffer_empty = 0;
    return true;
}

bool spi_buffer_data_available() {
    // TODO disable interrupts here?
    return buffer_read_head != buffer_head;
}

uint_fast8_t spi_buffer_read() {
    uint_fast8_t byte = buffer[buffer_read_head++];
    if (buffer_read_head >= BUFFER_SIZE)
        buffer_read_head = 0;
    return byte;
}
// For external use ↑

static inline void begin() {
    //DIGPIN(PIN_BRUSH_SIDES, on);
    //byte = 69;
    //byte = cord_connection_output_pull(); // 0 if the buffer is empty, that's fine
    spi_buffer_pull();
    byte = 0b10110010;
    bits_left = 8;
}

static inline void end() {
    //DIGPIN(PIN_BRUSH_SIDES, off);
    //if (byte > 0)
    //    DIGPIN(PIN_BRUSH_SIDES, on);
    spi_buffer_put(byte);
    //cord_connection_input_put(byte);
    // ↑ TODO is & 0xFF required here? Should we switch to a more appropriate type?
}

void setup_spi() {
    // TODO move it out of here
    INTDisableInterrupts();
    mCNOpen(CN_ON | CN_IDLE_CON, CN1_ENABLE, CN1_PULLUP_ENABLE);
    mPORTCRead();
    mCNSetIntPriority(6); // same as below
    mCNClearIntFlag();
    mCNIntEnable(1);
    INTEnableInterrupts();
}

/*
void __ISR(_CHANGE_NOTICE_VECTOR, IPL6AUTO) ChangeNotice_Handler(void) {
    // This handler is really-really hot. Since some poor dude might
    // be compiling this with a crippled version of gcc provided by
    // Microchip (crippled by Microchip too), we will be doing
    // some ugly stuff here.
    // This handler must finish in ≈0.03 ms
    //mPORTCRead();
    if (hw_get_milliseconds() < 100) { // allow some initial noise
        mCNClearIntFlag();
        return;
    }

    if (!READPIN(PIN_CONNECTION_SCK)) { // let's write
        //if (bits_left == 0)
        //    begin();
        //DIGPIN(PIN_CONNECTION_MISO, (0b10000000 & byte) ? on : off);
        //byte <<= 1;
        //bits_left--;
        DIGPIN(PIN_CONNECTION_MISO, off);
    } else { // let's read
        //byte += READPIN(PIN_CONNECTION_MOSI);
        //if (bits_left == 0)
        //    end();
        DIGPIN(PIN_CONNECTION_MISO, on);
    }
    mCNClearIntFlag();
}
 */
