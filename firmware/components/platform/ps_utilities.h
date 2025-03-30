/*
 * Persistent Storage Module
 *
 * Manage the persistent storage kept in the gCore EFM8 RAM and provide access
 * routines to it.
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
 *
 */
#ifndef PS_UTILITIES_H
#define PS_UTILITIES_H

#include <stdbool.h>
#include <stdint.h>



//
// PS Utilities Constants
//

//
// Configuration types
#define PS_NUM_CONFIGS           3

#define PS_CONFIG_TYPE_GUI       0
#define PS_CONFIG_TYPE_NET       1
#define PS_CONFIG_TYPE_TZ        2

// PS Size
//  - must be less than contained in gCore's EFM8 RAM
//  - should be fairly small to keep I2C burst length down
#define PS_RAM_SIZE         320
#define PS_RAM_STARTADDR    0

// Default 24-hour mode display
#define PS_DEFAULT_HOUR_MODE_24 false

// Default backlight brightness (percent)
#define PS_DEFAULT_BACKLIGHT    80

// Default timezone
#define PS_DEFAULT_TZ           "MST7MDT,M3.2.0,M11.1.0"

// Base part of the default SSID/Clock name - the last 4 nibbles of the ESP32's
// mac address are appended as ASCII characters
#define PS_DEFAULT_AP_SSID "NixieClock-"

// Maximum timezone length
#define PS_TZ_MAX_LEN       80

// Field lengths
#define PS_SSID_MAX_LEN     32
#define PS_PW_MAX_LEN       63



//
// PS Utilities config typedefs
//

typedef struct {
	bool hour_mode_24;
	uint8_t lcd_brightness;
} gui_config_t;

typedef struct {
	bool mdns_en;                      // 0: mDNS discovery disabled, 1: mDNS discovery enabled
	bool sta_mode;                     // 0: AP mode, 1: STA mode
	bool sta_static_ip;                // In station mode: 0: DHCP-served IP, 1: Static IP
	char ap_ssid[PS_SSID_MAX_LEN+1];   // AP SSID is also the Camera Name
	char sta_ssid[PS_SSID_MAX_LEN+1];
	char ap_pw[PS_PW_MAX_LEN+1];
	char sta_pw[PS_PW_MAX_LEN+1];
	uint8_t ap_ip_addr[4];
	uint8_t sta_ip_addr[4];
	uint8_t sta_netmask[4];
} net_config_t;

typedef struct {
	char tz[PS_TZ_MAX_LEN+1];
} tz_config_t;



//
// PS Utilities API
//
bool ps_init();
void ps_save_to_flash();
bool ps_get_config(int index, void* cfg);
bool ps_set_config(int index, void* cfg);
bool ps_reinit_all();
bool ps_reinit_config(int index);
bool ps_has_new_ap_name(const char* name);
char ps_nibble_to_ascii(uint8_t n);

#endif /* PS_UTILITIES_H */