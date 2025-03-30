/*
 * gCore RTC Module
 *
 * Provides access to the gCore Real-Time clock and alarm.
 *
 * With from Michael Margolis' time.c file.
 *
 * Copyright 2021-2025 Michael Margolis and Dan Julio
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
#include "esp_system.h"
#include "esp_log.h"
#include "gcore.h"
#include "rtc.h"



//
// RTC variables
//
static const char* TAG = "RTC";


//
// RTC API
//

uint32_t rtc_get_time_secs()
{
    uint32_t t = 0;
    
    (void) gcore_get_time_secs(&t);
    
    return((uint32_t) t);
}


bool rtc_set_time_secs(uint32_t t)
{
    return (gcore_set_time_secs((uint32_t) t));
}


uint32_t rtc_get_alarm_secs()
{
	uint32_t t = 0;
    
    (void) gcore_get_alarm_secs(&t);
    
    return((uint32_t) t);
}


bool rtc_set_alarm_secs(uint32_t t)
{
	return (gcore_set_alarm_secs((uint32_t) t));
}


bool rtc_enable_alarm(bool en)
{
	
	return (gcore_set_wakeup_bit(GCORE_WK_ALARM_MASK, en) == true);
}


bool rtc_get_alarm_enable(bool* en)
{
	uint8_t t8;
	
	if (gcore_get_reg8(GCORE_REG_WK_CTRL, &t8)) {
		*en = ((t8 & GCORE_WK_ALARM_MASK) == GCORE_WK_ALARM_MASK);
		return true;
	} else {
		return false;
	}
}
