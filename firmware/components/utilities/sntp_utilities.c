/*
 * NTP access utilities
 *
 * Copyright 2024-2025 Dan Julio
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
 */
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include "rtc.h"
#include "sntp_utilities.h"
#include "time_utilities.h"
#include <time.h>



//
// Variables
//
static const char* TAG = "sntp_utilities";




//
// Internal functions
//
static void _time_sync_notification_cb(struct timeval *tv);



//
// API
//
void sntp_start_service()
{
	esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_POOL_SERVER);
	
	config.sync_cb = _time_sync_notification_cb;
	
	esp_netif_sntp_init(&config);
	
	ESP_LOGI(TAG, "Starting SNTP service");
	esp_netif_sntp_start();
}


void sntp_stop_service()
{
	ESP_LOGI(TAG, "Stopping SNTP service");
	
	esp_sntp_stop();
	esp_netif_sntp_deinit();
}



//
// Internal functions
//
static void _time_sync_notification_cb(struct timeval *tv)
{
	char buf[32];
	time_t now;
	tmElements_t te;
	
	now = tv->tv_sec;
	if (tv->tv_usec > 500000) now += 1;
	localtime_r(&now, &te);   // Get the unix formatted timeinfo
	mktime(&te);              // Fill in the DOW and DOY fields
	
	time_get_disp_string(&te, buf);
	ESP_LOGI(TAG, "SNTP Set %s", buf);
	
	if (!rtc_set_time_secs((uint32_t) now) != 0) {
		ESP_LOGE(TAG, "Update RTC failed");
	}
}
