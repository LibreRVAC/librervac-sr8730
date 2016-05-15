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
#include "cordlib/cord_connection.h"
#include "cordlib/cord_sound.h"
#include "charging.h"
#include "battery.h"


// ======================== Schematic ========================
//               VCC (3.3V)
//                ┃
//                │
//                │
//               ┏┻┓
//               ┃ ┃1.5kΩ
//               ┗┳┛
//                │
//   thermistor   ├──────━━ AN4
//    ↓           │
//    ┌───────────┼───────────┐
//    │           │           │
//   ┏┻┓         ┏┻┓         ━┻━
//   ┃/┃10kΩ     ┃ ┃10kΩ     ━┳━
//   ┗┳┛         ┗┳┛          │
//    │           │           │
//   ━┻━         ━┻━         ━┻━
//   ╺━╸         ╺━╸         ╺━╸
//    ━           ━           ━

//                                     ↓ shunt
//                                    0.3Ω
//   Vin ━━───┬─────┳━┓ ┏────────┬──━/\/\/\/━──┬─────────┐
//           ┏┻┓    ┃ ↑ ┃       ┏┻┓           ┏┻┓        │
//           ┃ ┃    ╞════       ┃ ┃10.1kΩ     ┃ ┃10.1kΩ  │
//           ┗┳┛    │           ┗┳┛           ┗┳┛        │
//            └─────┤      AN2━━─┤       AN3━━─┤        ━┻━
// Chg(PWM)━┓      ┏┻┓           │             │         ━
//          │      ┃ ┃         ┌─┴─┐       ┌───┤        ━━━
//         ┏┻┓     ┗┳┛         │   │       │   │         ━    ← 12 cells
//         ┃ ┃      │         ┏┻┓ ━┻━     ━┻━ ┏┻┓        ┋      in series
//         ┗┳┛   ┣━━┛      1kΩ┃ ┃ ━┳━     ━┳━ ┃ ┃1kΩ    ━━━
//          └────┫            ┗┳┛  │       │  ┗┳┛        ┳
//        NPN →  ┣━→┓          │   │       │   │         │
//                  │          │   │       │   │         │
//                 ━┻━        ━┻━ ━┻━     ━┻━ ━┻━       ━┻━
//                 ╺━╸        ╺━╸ ╺━╸     ╺━╸ ╺━╸       ╺━╸
//                  ━          ━   ━       ━   ━         ━

// Even at the end of the charging the battery should not reach 50°C.
// Perhaps it is OK to increase it to 60°C, but this is not going to help much.
#define ABSOLUTE_TEMPERATURE_CUTOFF 50

#define ADC_BUFFER_SIZE 75 // too high values may make things oscillate
// ADC values are really small, so it will definitely fit into *signed* ints.
// This way we will have no problems with things wrapping around.
volatile int_fast32_t adc_thermistor[ADC_BUFFER_SIZE];
volatile int_fast32_t adc_before_shunt[ADC_BUFFER_SIZE];
volatile int_fast32_t adc_after_shunt[ADC_BUFFER_SIZE];
size_t adc_buffer_needle = 0;
// ↑ Still, the values are a bit jumpy, and 10 bits is not enough to get
// precise values sometimes. So we will be getting the average of a bunch of
// latest samples. Note that ISR may be writing values into the array as
// we read it, but that's ok.

void setup_battery() {
    CloseADC10();
    SetChanADC10(
            ADC_CH0_NEG_SAMPLEA_NVREF
            | ADC_CH0_POS_SAMPLEA_AN2
            | ADC_CH0_POS_SAMPLEA_AN3
            | ADC_CH0_POS_SAMPLEA_AN4
            );
    // TODO we are getting unsigned values but we are treating them
    // as signed, is it ok? It seems to work but it is just weird.
    OpenADC10(ADC_MODULE_ON | ADC_FORMAT_INTG32 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON, //
            ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_ON | ADC_SAMPLES_PER_INT_3
            | ADC_ALT_BUF_ON | ADC_ALT_INPUT_OFF, //
            ADC_CONV_CLK_INTERNAL_RC | ADC_SAMPLE_TIME_31
            , // TODO internal rc? What about other options?
            ENABLE_AN2_ANA // battery voltage before shunt
            | ENABLE_AN3_ANA // battery voltage after shunt
            | ENABLE_AN4_ANA, // battery thermistor
            // TODO ↓ this is a little bit verbose, is there any easier way?
            SKIP_SCAN_AN0     | SKIP_SCAN_AN1/*| SKIP_SCAN_AN2  | SKIP_SCAN_AN3
            | SKIP_SCAN_AN4*/ | SKIP_SCAN_AN5  | SKIP_SCAN_AN6  | SKIP_SCAN_AN7
            | SKIP_SCAN_AN8   | SKIP_SCAN_AN9  | SKIP_SCAN_AN10 | SKIP_SCAN_AN11
            | SKIP_SCAN_AN12  | SKIP_SCAN_AN13 | SKIP_SCAN_AN14 | SKIP_SCAN_AN15
            ); // configure ADC using parameter define above
    ConfigIntADC10(ADC_INT_ON | ADC_INT_PRI_3 | ADC_INT_SUB_PRI_2);
    mAD1ClearIntFlag();
    EnableADC10();
}

#define VCC 3.3
#define ADC_MAX 1024
#define Rd 10.1

float get_battery_voltage_before_shunt() {
    int_fast32_t sum = 0;
    for (size_t i = 0; i < ADC_BUFFER_SIZE; i++)
        sum += adc_before_shunt[i];
    return (float) sum / ADC_BUFFER_SIZE * VCC / ADC_MAX * Rd;
}

float get_battery_voltage_after_shunt() {
    int_fast32_t sum = 0;
    for (size_t i = 0; i < ADC_BUFFER_SIZE; i++)
        sum += adc_after_shunt[i];
    return (float) sum / ADC_BUFFER_SIZE * VCC / ADC_MAX * Rd;
}

// TODO NiMH discharge is COMPLETELY non-linear!

float get_battery_status() {
    float voltage = get_battery_voltage_after_shunt();
    // TODO get more precise values
    float result;
    if (is_charging()) {
        // ≈15V -- ≈17.7
        result = (voltage - 15.5) / (17.7 - 15);
    } else {
        // ≈14V -- ≈17 // TODO ??
        result = (voltage - 14.8) / (17 - 14);
    }
    if (result < 0)
        return 0;
    if (result > 1)
        return 1;
    return result;
}

#define Rshunt 0.3

volatile float current_offset = 0;

float get_battery_current() {
    if (!is_charging())
        return 0.0; // we don't know
    int_fast32_t sum = 0;
    for (size_t i = 0; i < ADC_BUFFER_SIZE; i++)
        sum += adc_before_shunt[i] - adc_after_shunt[i];
    return (float) sum / ADC_BUFFER_SIZE * VCC / ADC_MAX * Rd / Rshunt
            + current_offset;
}

void battery_calibrate_current() {
    // We can only measure charging current. This means that if we are not
    // charging, the current has to be 0. In reality, there is an
    // offset (possibly due imperfect resistors).
    // It is unclear if this error becomes higher with higher currents, but
    // in practice if we treat this as a plain offset it works very well.
    current_offset = 0;
    current_offset = -get_battery_current();
}

float get_battery_temperature() {
#define A -0.966249433923487
#define B 42.4294209665111
#define C 0.129333893760934
#define D 0.307073187406113
#define R1 1.5
#define R2 10
    int_fast32_t sum = 0;
    for (size_t i = 0; i < ADC_BUFFER_SIZE; i++)
        sum += adc_thermistor[i];
    float voltage = (float) sum / ADC_BUFFER_SIZE * VCC / ADC_MAX;
    // Rt = R1 / ( VCC / adc - 1 - R1 / R2 )
    float Rt = R1 / ( VCC / ((float) voltage) - 1 - R1 / R2 );
    return A + (B / ( C * (Rt) + D));
}

void __ISR(_ADC_VECTOR, IPL3SOFT) ADCHandler(void) {
    // TODO hardcode some protection here
    // TODO there is probably no need to run adc so often

    if (!is_noisy()) {
        int bufshift = ReadActiveBufferADC10() == 1 ? 0 : 8;
        adc_before_shunt[adc_buffer_needle] = ReadADC10(bufshift + 0);
        adc_after_shunt[adc_buffer_needle]  = ReadADC10(bufshift + 1);
        adc_thermistor[adc_buffer_needle]   = ReadADC10(bufshift + 2);
        if (++adc_buffer_needle >= ADC_BUFFER_SIZE)
            adc_buffer_needle = 0;
    }
    mAD1ClearIntFlag();
}
