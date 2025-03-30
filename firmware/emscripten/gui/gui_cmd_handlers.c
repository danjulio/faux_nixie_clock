/*
 * Command handlers updating GUI state
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
#include "gui_cmd_handlers.h"
#include "gui_sub_page_info.h"
#include "gui_sub_page_time.h"
#include "gui_state.h"
#include "gui_utilities.h"
#include <string.h>
#include <stdio.h>
#include <time.h>


//
// Constants
//

// These must match code below and in cmd handlers and sender
#define CMD_TIME_LEN            36
#define CMD_WIFI_INFO_LEN       (3 + 2*(GUI_SSID_MAX_LEN+1) + 2*(GUI_PW_MAX_LEN+1) + 3*4)



//
// API
//
void cmd_handler_rsp_backlight(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	if ((data_type == CMD_DATA_INT32) && (len == 4)) {
		gui_state.lcd_brightness = ntohl(*((uint32_t*) &data[0]));
		gui_state_note_item_inited(GUI_STATE_INIT_BACKLIGHT);
	}
}


void cmd_handler_rsp_mode(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	if ((data_type == CMD_DATA_INT32) && (len == 4)) {
		gui_state.hour_mode_24 = ntohl(*((uint32_t*) &data[0]));
		gui_state_note_item_inited(GUI_STATE_INIT_MODE);
	}
}


void cmd_handler_rsp_sys_info(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	if (data_type == CMD_DATA_STRING) {
		gui_sub_page_info_set_string((char*) data);
	}
}


void cmd_handler_rsp_time(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	struct tm te;
	
	if ((data_type == CMD_DATA_BINARY) && (len == CMD_TIME_LEN)) {
		// Unpack the byte array in the same order the get command packed it
		te.tm_sec = (int) ntohl(*((uint32_t*) &data[0]));
		te.tm_min = (int) ntohl(*((uint32_t*) &data[4]));
		te.tm_hour = (int) ntohl(*((uint32_t*) &data[8]));
		te.tm_mday = (int) ntohl(*((uint32_t*) &data[12]));
		te.tm_mon = (int) ntohl(*((uint32_t*) &data[16]));
		te.tm_year = (int) ntohl(*((uint32_t*) &data[20]));
		te.tm_wday = (int) ntohl(*((uint32_t*) &data[24]));
		te.tm_yday = (int) ntohl(*((uint32_t*) &data[28]));
		te.tm_isdst = (int) ntohl(*((uint32_t*) &data[32]));
		
		gui_sub_page_time_set_time(&te);
	}
}


void cmd_handler_rsp_timezone(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	if (data_type == CMD_DATA_STRING) {
		strncpy(gui_state.timezone, (char*) data, GUI_TZ_MAX_LEN);
		gui_state.timezone[GUI_TZ_MAX_LEN] = 0;
		
		gui_state_note_item_inited(GUI_STATE_INIT_TIMEZONE);
	}
}


void cmd_handler_rsp_wifi(cmd_data_t data_type, uint32_t len, uint8_t* data)
{
	int i, n = 0;
	
	if ((data_type == CMD_DATA_BINARY) && (len == CMD_WIFI_INFO_LEN)) {
		// Unpack the byte array in the same order the get command packed it
		gui_state.mdns_en = (bool) data[n++];
		gui_state.sta_mode = (bool) data[n++];
		gui_state.sta_static_ip = (bool) data[n++];
		
		for (i=0; i<GUI_SSID_MAX_LEN+1; i++) gui_state.ap_ssid[i] = data[n++];
		for (i=0; i<GUI_SSID_MAX_LEN+1; i++) gui_state.sta_ssid[i] = data[n++];
		for (i=0; i<GUI_PW_MAX_LEN+1; i++) gui_state.ap_pw[i] = data[n++];
		for (i=0; i<GUI_PW_MAX_LEN+1; i++) gui_state.sta_pw[i] = data[n++];
		
		for (i=0; i<4; i++) gui_state.ap_ip_addr[i] = data[n++];
		for (i=0; i<4; i++) gui_state.sta_ip_addr[i] = data[n++];
		for (i=0; i<4; i++) gui_state.sta_netmask[i] = data[n++];
		
		gui_state_note_item_inited(GUI_STATE_INIT_WIFI);
	}
}
