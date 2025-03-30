/*
 * Time related utilities
 *
 * Contains functions to interface the RTC to the system timekeeping
 * capabilities and provide application access to the system time.
 *
 * Copyright 2020-2025 Dan Julio
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */
#ifndef TIME_UTILITIES_H
#define TIME_UTILITIES_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>



//
// Time structures
//
typedef struct tm tmElements_t;



//
// Time Utilities API
//
void time_init(const char *timezone);
void time_set(tmElements_t* te);
void time_get(tmElements_t* te);
void time_timezone_set(const char *timezone);
bool time_changed(tmElements_t* te, time_t* prev_time);
void time_get_disp_string(tmElements_t* te, char* buf);

#endif /* TIME_UTILITIES_H */
