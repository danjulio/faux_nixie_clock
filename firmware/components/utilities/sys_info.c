/*
 * Get sys_info string for display
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
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys_info.h"
#include "time_utilities.h"
#include "power_utilities.h"
#include "ps_utilities.h"
#include "wifi_utilities.h"
#include <string.h>



//
// Variables
//

// Camera information string
static char info_buf[SYS_INFO_MAX_LEN+1];

static net_config_t wifi_info;

static const char* copyright_info = "\nFauxNixieClock copyright (c) 2024-2025\n" \
                                    "by Dan Julio.  All rights reserved.\n";



//
// Forward declarations for internal functions
//
static void _update_info();
static int _add_fw_version(int n);
static int _add_sdk_version(int n);
static int _add_battery_info(int n);
static int _add_time(int n);
static int _add_mem_info(int n);
static int _add_copyright_info(int n);
static int _add_wifi_mode(int n);
static int _add_ip_address(int n);
static int _add_mac_address(int n);




//
// API
//
char* sys_info_get_string()
{
	_update_info();
	
	return info_buf;
}



//
// Internal functions
//
static void _update_info()
{
	int n = 0;
	
	(void) ps_get_config(PS_CONFIG_TYPE_NET, &wifi_info);
	
	n = _add_fw_version(n);
	n = _add_sdk_version(n);
	n = _add_battery_info(n);
	n = _add_wifi_mode(n);
	n = _add_ip_address(n);
	n = _add_mac_address(n);
	n = _add_time(n);
	n = _add_mem_info(n);
	n = _add_copyright_info(n);
}


static int _add_fw_version(int n)
{
	const esp_app_desc_t* app_desc;
	
	app_desc = esp_app_get_description();
	sprintf(&info_buf[n], "FW Version: %s\n", app_desc->version);
	
	return (strlen(info_buf));
}


static int _add_sdk_version(int n)
{
	sprintf(&info_buf[n], "SDK Version: %s\n", esp_get_idf_version());
	
	return (strlen(info_buf));
}


static int _add_battery_info(int n)
{
	batt_status_t bs;
	
	power_get_batt(&bs);

	sprintf(&info_buf[n], "Battery: %1.2f V, Charge ", bs.batt_voltage);
	n = strlen(info_buf);
	
	switch (bs.charge_state) {
		case CHARGE_OFF:
			sprintf(&info_buf[n], "off\n");
			break;
		case CHARGE_ON:
			sprintf(&info_buf[n], "on\n");
			break;
		case CHARGE_DONE:
			sprintf(&info_buf[n], "done\n");
			break;
		case CHARGE_FAULT:
			sprintf(&info_buf[n], "fault\n");
			break;
	}

	return (strlen(info_buf));
}


static int _add_time(int n)
{
	char buf[28];
	tmElements_t te;
	
	time_get(&te);
	time_get_disp_string(&te, buf);
	sprintf(&info_buf[n], "Time: %s\n", buf);
	
	return (strlen(info_buf));
}


static int _add_mem_info(int n)
{
	sprintf(&info_buf[n], "Heap Free: Int %d (min %d)\n            PSRAM %d (min %d)\n",
		heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
		heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
		heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
		heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
		
	return (strlen(info_buf));
}


static int _add_wifi_mode(int n)
{
	if (!wifi_info.sta_mode) {
		sprintf(&info_buf[n], "Wifi Mode: AP\n");
	} else if (wifi_info.sta_static_ip) {
		sprintf(&info_buf[n], "Wifi Mode: STA with static IP address\n");
	} else {
		sprintf(&info_buf[n], "Wifi Mode: STA\n");
	}
	
	return (strlen(info_buf));
}


static int _add_ip_address(int n)
{
	char buf[16];   // "XXX.XXX.XXX.XXX" + null
		
	if ((!wifi_info.sta_mode && wifi_is_enabled()) ||
	    ( wifi_info.sta_mode && wifi_is_connected())) {
	    
	    wifi_get_ipv4_addr(buf);
		sprintf(&info_buf[n], "IP Address: %s\n", buf);
	} else {
		sprintf(&info_buf[n], "IP Address: - \n");
	}
	
	return (strlen(info_buf));
}


static int _add_mac_address(int n)
{
	uint8_t sys_mac_addr[6];
	
	esp_efuse_mac_get_default(sys_mac_addr);
	
	// Add 1 for soft AP mode (see "Miscellaneous System APIs" in the ESP-IDF documentation)
	if (!wifi_info.sta_mode) sys_mac_addr[5] += 1;
	
	sprintf(&info_buf[n], "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
		sys_mac_addr[0], sys_mac_addr[1], sys_mac_addr[2],
		sys_mac_addr[3], sys_mac_addr[4], sys_mac_addr[5]);
	
	return (strlen(info_buf));
}


static int _add_copyright_info(int n)
{
	sprintf(&info_buf[n], copyright_info);
	
	return(strlen(info_buf));
}
