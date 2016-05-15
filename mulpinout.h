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

// IC1 and IC2
#define PIN_IR_SENSOR_RIGHT_RIGHT  IC_SENSORS, 0
#define PIN_IR_SENSOR_MIDDLE_RIGHT IC_SENSORS, 1
#define PIN_IR_SENSOR_FRONT_RIGHT  IC_SENSORS, 2
#define PIN_IR_SENSOR_FRONT        IC_SENSORS, 3
#define PIN_IR_SENSOR_FRONT_LEFT   IC_SENSORS, 4
#define PIN_IR_SENSOR_MIDDLE_LEFT  IC_SENSORS, 5
#define PIN_IR_SENSOR_LEFT_LEFT    IC_SENSORS, 6
// 7 is grounded


// other IC16 are not known yet (grounded)
#define PIN_SPEAKER                IC16,       7
