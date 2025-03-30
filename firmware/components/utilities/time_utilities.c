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
#include "esp_system.h"
#include "time_utilities.h"
#include "esp_log.h"
#include "rtc.h"
#include <stdlib.h>
#include <sys/time.h>


//
// Constants
//
// Minimum epoch time (12:00:00 AM Jan 1 2000)
#define MIN_EPOCH_TIME 946684800


//
// Time Utilities date related strings (related to tmElements)
//
static const char* day_strings[] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

static const char* mon_strings[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};



//
// Time Utilities Variables
//
static const char* TAG = "time_utilities";


//
// Time Utilities API
//

/**
 * Initialize system time from the RTC
 */
void time_init(const char *timezone)
{
	char buf[32];
	struct timeval tv;
	tmElements_t te;
	uint32_t secs;
	
	// Set the timezone first
	setenv("TZ", timezone, 1);
	tzset();
	ESP_LOGI(TAG, "Set timezone: %s", timezone);
	
	// Set the system time from the RTC
	secs = rtc_get_time_secs();
	if (secs == 0) {
		secs = MIN_EPOCH_TIME;
	}
	
	// Set ESP32 system clock
	tv.tv_sec = secs;
	tv.tv_usec = 0;
	settimeofday((const struct timeval *) &tv, NULL);
	
	// Diagnostic display of time
	time_get(&te);
	time_get_disp_string(&te, buf);
	ESP_LOGI(TAG, "Set time: %s", buf);
}


/**
 * Set the system time and update the RTC
 */
void time_set(tmElements_t* te)
{
	char buf[32];
	struct timeval tv;
	time_t secs;
	
	// Set the system time
	secs = mktime(te);
	tv.tv_sec = secs;
	tv.tv_usec = 0;
	settimeofday((const struct timeval *) &tv, NULL);
	
	// Then attempt to set the RTC
	if (rtc_set_time_secs(secs)) {
		time_get_disp_string(te, buf);
		ESP_LOGI(TAG, "Update RTC time: %s", buf);
	} else {
		ESP_LOGE(TAG, "Update RTC failed");
	}
}



/**
 * Get the system time
 */
void time_get(tmElements_t* te)
{
	time_t now;
	
	time(&now);
    localtime_r(&now, te);  // Get the unix formatted timeinfo
    mktime(te);             // Fill in the DOW and DOY fields
}


/**
 * Change the timezone
 */
void time_timezone_set(const char *timezone)
{
	char buf[32];
	struct timeval tv;
	tmElements_t te;
	uint32_t secs;
	
	ESP_LOGI(TAG, "New timezone: %s", timezone);
	
	// Set the new timezone
	setenv("TZ", timezone, 1);
	tzset();
	
	// Update the RTC
	gettimeofday(&tv, NULL);
	secs = (uint32_t) tv.tv_sec;
	if (tv.tv_usec >= 500000) {
		secs += 1;
	}
	
	if (rtc_set_time_secs(secs)) {
		time_get(&te);
		time_get_disp_string(&te, buf);
		ESP_LOGI(TAG, "Set RTC time for timezone change to: %s", buf);
	} else {
		ESP_LOGE(TAG, "Update RTC for timezone change failed");
	}
}


/**
 * Return true if the system time (in seconds) has changed from the last time
 * this function returned true. Each calling task must maintain its own prev_time
 * variable (it can initialize it to 0).  Set te to NULL if you don't need the time.
 */
bool time_changed(tmElements_t* te, time_t* prev_time)
{
	time_t now;
	struct tm timeinfo;
	
	// Get the time and check if it is different
	time(&now);
	if (now != *prev_time) {
		*prev_time = now;
		
		if (te != NULL) {
			// convert time into our simplified tmElements format
    		localtime_r(&now, &timeinfo);  // Get the unix formatted timeinfo
    		mktime(&timeinfo);             // Fill in the DOW and DOY fields
    	}
    	
    	return true;
    } else {
    	return false;
    }
}


/**
 * Load buf with a time & date string for display.
 *
 *   "DOW MON DAY, YEAR HH:MM:SS"
 *
 * buf must be at least 26 bytes long (to include null termination).
 */
void time_get_disp_string(tmElements_t* te, char* buf)
{
	// Validate te to prevent illegal accesses to the constant string buffers
	if (te->tm_wday > 6) te->tm_wday = 0;
	if (te->tm_mon > 11) te->tm_mon = 0;
	
	// Build up the string
	sprintf(buf,"%s %s %2d, %4d %2d:%02d:%02d", 
		day_strings[te->tm_wday],
		mon_strings[te->tm_mon],
		te->tm_mday,
		te->tm_year + 1900,
		te->tm_hour,
		te->tm_min,
		te->tm_sec);
}
