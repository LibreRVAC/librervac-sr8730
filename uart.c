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
#include "gpio.h"
#include "cord_hw.h"
#include "cord_connection.h"
#include "cordlib/cord_buffer.h"
#include "cordlib/cord_sound.h"

// 8 bits, no parity, 1 stop bit

// TODO overrun protection

typedef volatile struct {
    uint_fast32_t ticks;
    bool value; // final value after the edge
} edge;

// TODO the whole thing has to be volatile
RBUFFER(edges, edge, 1000);

volatile bool initialized;
volatile uint_fast16_t ticks_per_bit = UINT_FAST16_MAX;

volatile bool sent = true;
volatile uint8_t output_byte = 0;

volatile uint_fast32_t ticks = 0;

static void start_timer(uint_fast16_t period) {
    OpenTimer4(T4_ON | T4_PS_1_8 | T4_SOURCE_INT, period);
    mT4SetIntPriority(7); // should be same as below
    mT4ClearIntFlag();
    mT4IntEnable(1);
}

void __ISR(_TIMER_4_VECTOR, IPL7SOFT) T4Interrupt(void) {
    static uint_fast32_t output_current_bit = 0;
    ticks += ticks_per_bit; // XXX

    if (initialized && !sent) {
        output_current_bit++;
        if (output_current_bit == 1) {
            DIGPIN(PIN_CONNECTION_TX, off); // start bit
        } else if (output_current_bit < 10) {
            DIGPIN(PIN_CONNECTION_TX, output_byte & 0b1); // data bits
            output_byte >>= 1;
        } else {
            sent = true;
            output_current_bit = 0;
            DIGPIN(PIN_CONNECTION_TX, on); // stop bit
        }
    }
    mT4ClearIntFlag();
}

void setup_uart() {
    // TODO move it out of here
    INTDisableInterrupts();

    mCNOpen(CN_ON | CN_IDLE_CON, CN1_ENABLE, CN1_PULLUP_ENABLE);
    mPORTCRead();
    mCNSetIntPriority(6); // same as below
    mCNClearIntFlag();
    mCNIntEnable(1);

    start_timer(0xFFFF);
    INTEnableInterrupts();
}

void __ISR(_CHANGE_NOTICE_VECTOR, IPL6SOFT) ChangeNotice_Handler(void) {
    mPORTCRead();
    mCNClearIntFlag();
    unsigned x = READPIN(PIN_CONNECTION_RX);

    //T3CONCLR = 0x8000;

    mT4IntEnable(0);
    uint_fast32_t time = ReadTimer4() + ticks;
    mT4IntEnable(1);

    //T3CONSET = 0x8000;

    RBUFFER_WRITE(edges, ((edge) {
        .value = x, // READPIN(PIN_CONNECTION_RX),
        .ticks = time,
    }));
    asm volatile ("nop");
}

// Call this function periodically

void uart_process_edges() {
    static uint_fast32_t input_current_bit = 0;
    static uint_fast32_t input_last_ticks = 0;
    static uint8_t input_byte = 0;

    //static bool fail = false;
    //static char debug_strs[10][50];
    static uint_fast32_t last_debug = 0;


    uint_fast32_t curms = hw_get_milliseconds();
    if (curms % 5000 == 0 && last_debug < curms) {
        ticks = 0; // XXX SHITTY WORKAROUND
        last_debug = curms;
        /*char str[40];
        sprintf(str, "ticks=%u\r\n", ticks);
        for (int i = 0; i < 40; i++) {
            if (str[i] == 0)
                break;
            cord_connection_output_put(str[i]);
        }*/
    }

    if (sent && cord_connection_output_size() > 0) {
        output_byte = cord_connection_output_pull();
        sent = false;
    }

    if (!RBUFFER_HAS_DATA(edges))
        return;
    if (RBUFFER_ELEMS(edges) < 2)
        return;
    edge edge = RBUFFER_READ(edges);

    if (hw_get_milliseconds() < 100)
        return; // allow some initial noise, just in case

    if (!initialized) { // TODO test if start bit is low
        ticks_per_bit = min(edge.ticks - input_last_ticks, ticks_per_bit);
        input_last_ticks = edge.ticks;
        if (++input_current_bit == 1 + 8 + 1) { // start + bits + stop
            initialized = true;
            input_current_bit = 0;
            CloseTimer4();
            start_timer(ticks_per_bit);
        }
        return;
    }

    if (edge.ticks > input_last_ticks + ticks_per_bit * 20)
        input_current_bit = 0; // autoheal

    if (input_current_bit == 0) {
        if (!edge.value) {
            //sprintf(debug_strs[0], "start: %u %d\r\n", edge.ticks, edge.value);
            // Start bit
            input_current_bit = 1;
            input_last_ticks = edge.ticks;
            input_byte = 0; // just in case
        }
        return;
    }
    if (input_current_bit <= 9) {
        while (input_last_ticks + ticks_per_bit * 1.5 < edge.ticks) {
            //char str[40];
            /*sprintf(str, "input_last_ticks=%u\r\n", input_last_ticks);
            for (int i = 0; i < 40; i++) {
                if (str[i] == 0)
                    break;
                cord_connection_output_put(str[i]);
            }
            sprintf(str, "edge.ticks=%u\r\n", edge.ticks);
            for (int i = 0; i < 40; i++) {
                if (str[i] == 0)
                    break;
                cord_connection_output_put(str[i]);
            }*/
            /*
            sprintf(str, "delay=%u\r\n", edge.ticks - input_last_ticks);
            for (int i = 0; i < 40; i++) {
                if (str[i] == 0)
                    break;
                cord_connection_output_put(str[i]);
            }
            sprintf(str, "ticks_per_bit=%u\r\n", ticks_per_bit);
            for (int i = 0; i < 40; i++) {
                if (str[i] == 0)
                    break;
                cord_connection_output_put(str[i]);
            }*/
            //fail = true;
            //DIGPIN(PIN_BRUSH_SIDES, on);
            //cord_buzzer_queue_beep(100, 0.1, 0.3);
            //sprintf(debug_strs[input_current_bit], "same: ~%u %d\r\n", input_last_ticks + ticks_per_bit, !edge.value);
            input_byte += (!edge.value << (input_current_bit++ - 1)); // inverse of what we have now
            input_last_ticks += ticks_per_bit;
        }
        if (input_current_bit < 9) {
            //sprintf(debug_strs[input_current_bit], "bit %u: %u %d\r\n", input_current_bit, edge.ticks, edge.value);

            // Data bits
            input_byte += (edge.value << (input_current_bit - 1));
            input_current_bit++;
            input_last_ticks = edge.ticks;
        } else {
            //sprintf(debug_strs[9], "stop: %u %d\r\n", edge.ticks, edge.value);
            /*if (fail)
                for (int x = 0; x < 10; x++)
                    for (int i = 0; i < 40; i++) {
                        if (debug_strs[x][i] == 0)
                            break;
                        cord_connection_output_put(debug_strs[x][i]);
                    }
             */
            //fail = false;
            // Stop bit
            input_current_bit = 0;
            //next_byte = input_byte;
            cord_connection_input_put(input_byte);
            // if (input_byte != 'U')
            //DIGPIN(PIN_BRUSH_SIDES, on);
        }
        return;
    }
    // something weird happened. Reset everything.
    input_current_bit = 0;
    input_byte = 0;
    // initialized = false; // TODO maybe?
}
