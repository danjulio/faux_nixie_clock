/*
 * Command handlers updating or retrieving application state
 *
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
#include <arpa/inet.h>
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cmd_handlers.h"
#include "cmd_utilities.h"
#include "ps_utilities.h"
#include "sys_info.h"
#include "sys_utilities.h"
#include "time_utilities.h"
#include <string.h>
#include "ctrl_task.h"
#include "web_task.h"
#include "gui_task.h"



//
// Constants
//

// These must match code below and in gui response handler and sender
#define CMD_TIME_LEN            36
#define CMD_WIFI_INFO_LEN       (3 + 2*(PS_SSID_MAX_LEN+1) + 2*(PS_PW_MAX_LEN+1) + 3*4)



//
// Variables
//
static const char* TAG = "cmd_handlers";

// Statically allocated big data structures used by functions below to save stack space
static uint8_t send_buf[CMD_WIFI_INFO_LEN];     // Sized for the largest packet type we send
static net_config_t orig_net_config;
static net_config_t new_net_config;
static struct tm te;


//
// Forward declarations for internal functions
//
static bool net_config_structs_eq(net_config_t* s1, net_config_t* s2);



//
// API
//
void cmd_handler_get_backlight(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	gui_config_t gui_config;
	
	ps_get_config(PS_CONFIG_TYPE_GUI, &gui_config);
	if (!cmd_send_int32(CMD_RSP, CMD_BACKLIGHT, (int32_t) gui_config.lcd_brightness)) {
		ESP_LOGE(TAG, "Couldn't send lcd_brightness");
	}
}


void cmd_handler_get_mode(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	gui_config_t gui_config;
	
	ps_get_config(PS_CONFIG_TYPE_GUI, &gui_config);
	if (!cmd_send_int32(CMD_RSP, CMD_MODE, (int32_t) gui_config.hour_mode_24)) {
		ESP_LOGE(TAG, "Couldn't send hour_mode_24");
	}
}


void cmd_handler_get_sys_info(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	if (!cmd_send_string(CMD_RSP, CMD_SYS_INFO, sys_info_get_string())) {
		ESP_LOGE(TAG, "Couldn't send sys_info");
	}
}


void cmd_handler_get_time(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	// Get the current time
	time_get(&te);
	
	// Pack the byte array - the response handler must unpack in the same order
	*(uint32_t*)&send_buf[0] = htonl((uint32_t) te.tm_sec);
	*(uint32_t*)&send_buf[4] = htonl((uint32_t) te.tm_min);
	*(uint32_t*)&send_buf[8] = htonl((uint32_t) te.tm_hour);
	*(uint32_t*)&send_buf[12] = htonl((uint32_t) te.tm_mday);
	*(uint32_t*)&send_buf[16] = htonl((uint32_t) te.tm_mon);
	*(uint32_t*)&send_buf[20] = htonl((uint32_t) te.tm_year);
	*(uint32_t*)&send_buf[24] = htonl((uint32_t) te.tm_wday);
	*(uint32_t*)&send_buf[28] = htonl((uint32_t) te.tm_yday);
	*(uint32_t*)&send_buf[32] = htonl((uint32_t) te.tm_isdst);
	
	if (!cmd_send_binary(CMD_RSP, CMD_TIME, CMD_TIME_LEN, send_buf)) {
		ESP_LOGE(TAG, "Couldn't send time");
	}
}


void cmd_handler_get_timezone(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	tz_config_t tz_config;
	
	// Get the current timezone configuration
	ps_get_config(PS_CONFIG_TYPE_TZ, &tz_config);
	
	if (!(cmd_send_string(CMD_RSP, CMD_TIMEZONE, tz_config.tz))) {
		ESP_LOGE(TAG, "Couldn't send timezone");
	}
}


void cmd_handler_get_wifi(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	int i, n = 0;
	
	// Get the current configuration
	ps_get_config(PS_CONFIG_TYPE_NET, &orig_net_config);
	
	// Pack the byte array - the response handler must unpack in the same order
	send_buf[n++] = (uint8_t) orig_net_config.mdns_en;
	send_buf[n++] = (uint8_t) orig_net_config.sta_mode;
	send_buf[n++] = (uint8_t) orig_net_config.sta_static_ip;
	
	for (i=0; i<PS_SSID_MAX_LEN+1; i++) send_buf[n++] = orig_net_config.ap_ssid[i];
	for (i=0; i<PS_SSID_MAX_LEN+1; i++) send_buf[n++] = orig_net_config.sta_ssid[i];
	for (i=0; i<PS_PW_MAX_LEN+1; i++) send_buf[n++] = orig_net_config.ap_pw[i];
	for (i=0; i<PS_PW_MAX_LEN+1; i++) send_buf[n++] = orig_net_config.sta_pw[i];
	
	for (i=0; i<4; i++) send_buf[n++] = orig_net_config.ap_ip_addr[i];
	for (i=0; i<4; i++) send_buf[n++] = orig_net_config.sta_ip_addr[i];
	for (i=0; i<4; i++) send_buf[n++] = orig_net_config.sta_netmask[i];
	
	if (!cmd_send_binary(CMD_RSP, CMD_WIFI_INFO, n, send_buf)) {
		ESP_LOGE(TAG, "Couldn't send wifi info");
	}
}


void cmd_handler_set_backlight(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	gui_config_t gui_config;
	
	if ((data_type == CMD_DATA_INT32) && (len == 4)) {
		ps_get_config(PS_CONFIG_TYPE_GUI, &gui_config);
		gui_config.lcd_brightness = ntohl(*((uint32_t*) &data[0]));
		ps_set_config(PS_CONFIG_TYPE_GUI, &gui_config);
		
		// Update ctrl_task to change the backlight level
		xTaskNotify(task_handle_ctrl, CTRL_NOTIFY_UPD_BACKLIGHT, eSetBits);
	}
}


void cmd_handler_set_mode(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	gui_config_t gui_config;
	
	if ((data_type == CMD_DATA_INT32) && (len == 4)) {
		ps_get_config(PS_CONFIG_TYPE_GUI, &gui_config);
		gui_config.hour_mode_24 = ntohl(*((uint32_t*) &data[0]));
		ps_set_config(PS_CONFIG_TYPE_GUI, &gui_config);
	}
}


void cmd_handler_set_poweroff(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	if (data_type == CMD_DATA_NONE) {
		xTaskNotify(task_handle_ctrl, CTRL_NOTIFY_SHUTDOWN, eSetBits);	
	}
}



void cmd_handler_set_time(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	if ((data_type == CMD_DATA_BINARY) && (len == CMD_TIME_LEN)) {
		te.tm_sec = (int) ntohl(*((uint32_t*) &data[0]));
		te.tm_min = (int) ntohl(*((uint32_t*) &data[4]));
		te.tm_hour = (int) ntohl(*((uint32_t*) &data[8]));
		te.tm_mday = (int) ntohl(*((uint32_t*) &data[12]));
		te.tm_mon = (int) ntohl(*((uint32_t*) &data[16]));
		te.tm_year = (int) ntohl(*((uint32_t*) &data[20]));
		te.tm_wday = (int) ntohl(*((uint32_t*) &data[24]));
		te.tm_yday = (int) ntohl(*((uint32_t*) &data[28]));
		te.tm_isdst = (int) ntohl(*((uint32_t*) &data[32]));
		
		time_set(&te);
	}
}


void cmd_handler_set_timezone(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	tz_config_t tz_config;
	
	if (data_type == CMD_DATA_STRING) {
		ps_get_config(PS_CONFIG_TYPE_TZ, &tz_config);
		if (strcmp(tz_config.tz, (char*) data) != 0) {
			strncpy(tz_config.tz, (char*) data, PS_TZ_MAX_LEN);
			tz_config.tz[PS_TZ_MAX_LEN] = 0;
			ps_set_config(PS_CONFIG_TYPE_TZ, &tz_config);
			time_timezone_set(tz_config.tz);
		}
	}
}


void cmd_handler_set_wifi(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	int i, n = 0;
	
	if ((data_type == CMD_DATA_BINARY) && (len == CMD_WIFI_INFO_LEN)) {
		// Get the current configuration
		ps_get_config(PS_CONFIG_TYPE_NET, &orig_net_config);
		
		// Unpack in same order as loaded
		new_net_config.mdns_en = (bool) data[n++];
		new_net_config.sta_mode = (bool) data[n++];
		new_net_config.sta_static_ip = (bool) data[n++];
		
		for (i=0; i<PS_SSID_MAX_LEN+1; i++) new_net_config.ap_ssid[i] = data[n++];
		for (i=0; i<PS_SSID_MAX_LEN+1; i++) new_net_config.sta_ssid[i] = data[n++];
		for (i=0; i<PS_PW_MAX_LEN+1; i++) new_net_config.ap_pw[i] = data[n++];
		for (i=0; i<PS_PW_MAX_LEN+1; i++) new_net_config.sta_pw[i] = data[n++];
		
		for (i=0; i<4; i++) new_net_config.ap_ip_addr[i] = data[n++];
		for (i=0; i<4; i++) new_net_config.sta_ip_addr[i] = data[n++];
		for (i=0; i<4; i++) new_net_config.sta_netmask[i] = data[n++];
		
		if (!net_config_structs_eq(&orig_net_config, &new_net_config)) {
			// Update PS if changed
			ps_set_config(PS_CONFIG_TYPE_NET, &new_net_config);
			
			// Notify ctrl_task to restart the network
			xTaskNotify(task_handle_ctrl, CTRL_NOTIFY_RESTART_NETWORK, eSetBits);
		}
	}
}



//
// Internal functions
//
static bool net_config_structs_eq(net_config_t* s1, net_config_t* s2)
{
	if (s1->mdns_en != s2->mdns_en) return false;
	if (s1->sta_mode != s2->sta_mode) return false;
	if (s1->sta_static_ip != s2->sta_static_ip) return false;
	
	if (strcmp(s1->ap_ssid, s2->ap_ssid) != 0) return false;
	if (strcmp(s1->sta_ssid, s2->sta_ssid) != 0) return false;
	if (strcmp(s1->ap_pw, s2->ap_pw) != 0) return false;
	if (strcmp(s1->sta_pw, s2->sta_pw) != 0) return false;
	
	for (int i=0; i<4; i++) {
		if (s1->ap_ip_addr[i] != s2->ap_ip_addr[i]) return false;
		if (s1->sta_ip_addr[i] != s2->sta_ip_addr[i]) return false;
		if (s1->sta_netmask[i] != s2->sta_netmask[i]) return false;
	}
	
	return true;
}
