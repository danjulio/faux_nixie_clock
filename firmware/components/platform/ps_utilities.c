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
#include "ps_utilities.h"
#include "system_config.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gcore.h"
#include <stdbool.h>
#include <string.h>


//
// PS Utilities internal constants
//

// "Magic Word" constants
#define PS_MAGIC_WORD_0 0x12
#define PS_MAGIC_WORD_1 0x34

// Layout version - to allow future firmware versions to change the layout without
// losing data
#define PS_LAYOUT_VERSION      1 

// Static Memory Array indicies
#define PS_MAGIC_WORD_0_ADDR   0
#define PS_MAGIC_WORD_1_ADDR   1
#define PS_LAYOUT_VERSION_ADDR 2
#define PS_FIRST_DATA_ADDR     3
#define PS_CHECKSUM_ADDR       (PS_RAM_SIZE - 1)

// Maximum bytes available for storage
#define PS_MAX_DATA_BYTES      (PS_RAM_SIZE - 4)


//
// PS Utilities enums and structs
//

enum ps_update_types_t {
	FULL,                      // Update all bytes in the external SRAM
	GUI,                       // Update GUI state related and checksum
	NET,                       // Update network state related and checksum
	TZ                         // Update timezone state and checksum
};


//
// Persistent Storage data structures - data stored in battery backed RAM
// Note: Careful attention must be paid to make sure these are packed structures
//

// PS pointers into the shadow buffer for parameter sub-regions
typedef struct {
	uint8_t start_index[PS_NUM_CONFIGS];
	uint8_t length[PS_NUM_CONFIGS];
} ps_sub_region_t;



//
// PS Utilities Internal variables
//
static const char* TAG = "ps_utilities";

// Our local copy for reading
static uint8_t ps_shadow_buffer[PS_RAM_SIZE];

// Copy read at boot to check for changes (and flash update)
static uint8_t ps_check_buffer[PS_RAM_SIZE];

// Indexes of and lengths of the parameter sub-regions
static ps_sub_region_t ps_sub_regions;



//
// PS Utilities Forward Declarations for internal functions
//
static bool _ps_read_array();
static bool _ps_write_array(enum ps_update_types_t t);
static void _ps_init_config_memory(int index);
static void _ps_init_array();
static bool _ps_valid_magic_word();
static uint8_t _ps_compute_checksum();
static bool _ps_write_bytes_to_gcore(uint16_t start_addr, uint16_t data_len);



//
// PS Utilities API
//

/**
 * Initialize persistent storage
 *   - Load our local buffer
 *   - Initialize it and the NVRAM with valid data if necessary
 */
bool ps_init()
{
	int n;
	
	// Setup our access data structure
	ps_sub_regions.start_index[PS_CONFIG_TYPE_GUI] = PS_FIRST_DATA_ADDR;
	ps_sub_regions.length[PS_CONFIG_TYPE_GUI] = sizeof(gui_config_t);
	n = sizeof(gui_config_t);
	ps_sub_regions.start_index[PS_CONFIG_TYPE_NET] = ps_sub_regions.start_index[PS_CONFIG_TYPE_GUI] +
	                                            ps_sub_regions.length[PS_CONFIG_TYPE_GUI];
	ps_sub_regions.length[PS_CONFIG_TYPE_NET] = sizeof(net_config_t);
	n += sizeof(net_config_t);
	ps_sub_regions.start_index[PS_CONFIG_TYPE_TZ] = ps_sub_regions.start_index[PS_CONFIG_TYPE_NET] +
	                                            ps_sub_regions.length[PS_CONFIG_TYPE_NET];
	ps_sub_regions.length[PS_CONFIG_TYPE_TZ] = sizeof(tz_config_t);
	n += sizeof(tz_config_t);
	if (n > PS_MAX_DATA_BYTES) {
		// This should never occur - mainly for debugging
		ESP_LOGE(TAG, "NVRAM does not have enough room for %d bytes\n", n);
		return false;
	} else {
		ESP_LOGI(TAG, "Using %d of %d bytes", n, PS_MAX_DATA_BYTES);
	}
	
	// Get the persistent data from the battery-backed PMIC/RTC chip
	if (!_ps_read_array()) {
		ESP_LOGE(TAG, "Failed to read persistent data from NVRAM");
	}
	
	// Make a copy to check for changes
	for (n=0; n<PS_RAM_SIZE; n++) {
		ps_check_buffer[n] = ps_shadow_buffer[n];
	}
	
	// Check if it is initialized with valid data, initialize if not
	if (!_ps_valid_magic_word() || (_ps_compute_checksum() != ps_shadow_buffer[PS_CHECKSUM_ADDR])) {
		ESP_LOGI(TAG, "Initialize persistent storage with default values");
		_ps_init_array();
		if (!_ps_write_array(FULL)) {
			ESP_LOGE(TAG, "Failed to write persistent data to NVRAM");
			return false;
		}
	}
	
	return true;
}


/**
 * Reset persistent storage to factory default values.  Store these in both the
 * battery-backed PMIC/RTC chip and backing flash (if necessary).
 */
void ps_set_factory_default()
{
	// Re-initialize persistent data and write it to battery-backed RAM
	ESP_LOGI(TAG, "Re-initialize persistent storage with default values");
	_ps_init_array();
	if (!_ps_write_array(FULL)) {
		ESP_LOGE(TAG, "Failed to write persistent data to NVRAM");
	}
	
	// Save to the PMIC/RTC flash memory (will execute only if changes are detected)
	ps_save_to_flash();
}


/**
 * Write battery-backed RAM to flash in the PMIC/RTC if any changes are detected.
 * We perform the dirty check to avoid unnecessary flash writes.
 */
void ps_save_to_flash()
{
	bool dirty_flag = false;
	int i;
	uint8_t reg = 1;
	
	// Check for any changed data since we booted indicating the need to save
	// NVRAM to flash
	for (i=0; i<PS_RAM_SIZE; i++) {
		if (ps_check_buffer[i] != ps_shadow_buffer[i]) {
			dirty_flag = true;
			break;
		}
	}
	
	if (dirty_flag) {
		ESP_LOGI(TAG, "Saving NVRAM");
		
		// Trigger a write of the NVRAM to backing flash
		if (gcore_set_reg8(GCORE_REG_NV_CTRL, GCORE_NVRAM_WR_TRIG)) {
			// Wait after triggering the write
			//   1. 36 mSec to allow the EFM8 to erase the flash memory (it is essentially
			//      locked up while doing this and won't respond to I2C cycles).
			//   2. 128 mSec to allow the NVRAM to be written to flash.  This isn't
			//      strictly necessary before starting to poll for completion but since 
			//      it's possible an I2C cycle will fail during the writing process we
			//      wait so we don't freak anyone out who might be looking at the log
			//      output.
			vTaskDelay(pdMS_TO_TICKS(155));
			
			// Poll until write is done - this should fall through immediately
			while (reg != 0) {
				vTaskDelay(pdMS_TO_TICKS(10));
				(void) gcore_get_reg8(GCORE_REG_NV_CTRL, &reg);
			}
			
			// Update the check buffer
			for (int i=0; i<PS_RAM_SIZE; i++) {
				ps_check_buffer[i] = ps_shadow_buffer[i];
			}
		}
	} 
}


bool ps_get_config(int index, void* cfg)
{
	if ((index >=0) && (index < PS_NUM_CONFIGS)) {
		// Give them our local copy
		(void) memcpy(cfg,  &ps_shadow_buffer[ps_sub_regions.start_index[index]], ps_sub_regions.length[index]);
		return true;
	} else {
		ESP_LOGE(TAG, "Requested read of illegal config index %d", index);
		return false;
	}
}



bool ps_set_config(int index, void* cfg)
{
	enum ps_update_types_t t;

	if ((index >=0) && (index < PS_NUM_CONFIGS)) {
		// Update our local copy
		(void) memcpy(&ps_shadow_buffer[ps_sub_regions.start_index[index]], cfg, ps_sub_regions.length[index]);
		
		// Update NVRAM
		switch (index) {
			case PS_CONFIG_TYPE_GUI:
				t = GUI;
				break;
			case PS_CONFIG_TYPE_NET:
				t = NET;
				break;
			default:
				t = TZ;
		}
		ps_shadow_buffer[PS_CHECKSUM_ADDR] = _ps_compute_checksum();
		if (!_ps_write_array(t)) {
			ESP_LOGE(TAG, "Failed to write config index %d to NVRAM", index);
		}
		
		return true;
	} else {
		ESP_LOGE(TAG, "Requested write of illegal config index %d", index);
		return false;
	}
}


bool ps_reinit_all()
{
	bool ret = true;
	
	ret &= ps_reinit_config(PS_CONFIG_TYPE_GUI);
	ret &= ps_reinit_config(PS_CONFIG_TYPE_NET);
	ret &= ps_reinit_config(PS_CONFIG_TYPE_TZ);
	
	return ret;
}



bool ps_reinit_config(int index)
{
	enum ps_update_types_t t;

	if ((index >=0) && (index < PS_NUM_CONFIGS)) {
		// Reset default values to our local copy
		_ps_init_config_memory(index);
		
		// Update NVRAM
		switch (index) {
			case PS_CONFIG_TYPE_GUI:
				t = GUI;
				break;
			case PS_CONFIG_TYPE_NET:
				t = NET;
				break;
			default:
				t = TZ;
		}
		ps_shadow_buffer[PS_CHECKSUM_ADDR] = _ps_compute_checksum();
		if (!_ps_write_array(t)) {
			ESP_LOGE(TAG, "Failed to write config index %d to NVRAM", index);
		}
		
		return true;
	} else {
		ESP_LOGE(TAG, "Requested reinit of illegal config index %d", index);
		return false;
	}
}


bool ps_has_new_ap_name(const char* name)
{
	net_config_t* net_configP;
	
	// Get a config-specific pointer to the data so we can parse it
	net_configP = (net_config_t*) &ps_shadow_buffer[ps_sub_regions.start_index[PS_CONFIG_TYPE_NET]];
	
	return(strncmp(name, net_configP->ap_ssid, PS_SSID_MAX_LEN) != 0);
}


char ps_nibble_to_ascii(uint8_t n)
{
	n = n & 0x0F;
	
	if (n < 10) {
		return '0' + n;
	} else {
		return 'A' + n - 10;
	}
}



//
// PS Utilities internal functions
//

static bool _ps_read_array()
{
	return (gcore_get_nvram_bytes(PS_RAM_STARTADDR, ps_shadow_buffer, PS_RAM_SIZE) == true);
}


static bool _ps_write_array(enum ps_update_types_t t)
{
	bool ret = false;
	
	switch(t) {
	case FULL:
		ret = _ps_write_bytes_to_gcore(0, PS_RAM_SIZE);
		break;

	case GUI:
		if (_ps_write_bytes_to_gcore(ps_sub_regions.start_index[PS_CONFIG_TYPE_GUI],
		                          ps_sub_regions.length[PS_CONFIG_TYPE_GUI]))
		{
			ret = gcore_set_nvram_byte(PS_RAM_STARTADDR + PS_CHECKSUM_ADDR,
			                       ps_shadow_buffer[PS_CHECKSUM_ADDR]);
		} else {
			ret = false;
		}
		break;
	
	case NET:
		if (_ps_write_bytes_to_gcore(ps_sub_regions.start_index[PS_CONFIG_TYPE_NET],
		                          ps_sub_regions.length[PS_CONFIG_TYPE_NET]))
		{
			ret = gcore_set_nvram_byte(PS_RAM_STARTADDR + PS_CHECKSUM_ADDR,
			                       ps_shadow_buffer[PS_CHECKSUM_ADDR]);
		} else {
			ret = false;
		}
		break;	
		
	case TZ:
		if (_ps_write_bytes_to_gcore(ps_sub_regions.start_index[PS_CONFIG_TYPE_TZ],
		                          ps_sub_regions.length[PS_CONFIG_TYPE_TZ]))
		{
			ret = gcore_set_nvram_byte(PS_RAM_STARTADDR + PS_CHECKSUM_ADDR,
			                       ps_shadow_buffer[PS_CHECKSUM_ADDR]);
		} else {
			ret = false;
		}
		break;
	}
	return ret;
}


// This routine has to be updated if any config changes.  It assumes the index is valid.
static void _ps_init_config_memory(int index)
{
	gui_config_t* gui_configP;
	net_config_t* net_configP;
	tz_config_t* tz_configP;
	uint8_t sys_mac_addr[6];
	
	switch (index) {
		case PS_CONFIG_TYPE_GUI:
			gui_configP = (gui_config_t*) &ps_shadow_buffer[ps_sub_regions.start_index[index]];
			
			gui_configP->hour_mode_24 = PS_DEFAULT_HOUR_MODE_24;
			gui_configP->lcd_brightness = PS_DEFAULT_BACKLIGHT;
			break;
		
		case PS_CONFIG_TYPE_NET:
			// Get the system's default MAC address and add 1 to match the "Soft AP" mode
			esp_efuse_mac_get_default(sys_mac_addr);
			sys_mac_addr[5] = sys_mac_addr[5] + 1;
			
			// Get a struct-friendly pointer to local storage and initialize the struct
			net_configP = (net_config_t*) &ps_shadow_buffer[ps_sub_regions.start_index[index]];
			net_configP->mdns_en = true;
			net_configP->sta_mode = false;
			net_configP->sta_static_ip = false;
						
			// Text fields start off empty
			for (int i=0; i<PS_SSID_MAX_LEN+1; i++) {
				net_configP->ap_ssid[i] = 0;
				net_configP->ap_pw[i] = 0;
				net_configP->sta_ssid[i] = 0;
				net_configP->sta_pw[i] = 0;
			}
			
			// Add our default AP SSID/Clock name
			sprintf(net_configP->ap_ssid, "%s%c%c%c%c", PS_DEFAULT_AP_SSID,
				ps_nibble_to_ascii(sys_mac_addr[4] >> 4),
		    	ps_nibble_to_ascii(sys_mac_addr[4]),
		    	ps_nibble_to_ascii(sys_mac_addr[5] >> 4),
	 	    	ps_nibble_to_ascii(sys_mac_addr[5]));

			
			// Default IP addresses (match espressif defaults)
			net_configP->ap_ip_addr[3] = 192;
			net_configP->ap_ip_addr[2] = 168;
			net_configP->ap_ip_addr[1] = 4;
			net_configP->ap_ip_addr[0] = 1;
			net_configP->sta_ip_addr[3] = 192;
			net_configP->sta_ip_addr[2] = 168;
			net_configP->sta_ip_addr[1] = 4;
			net_configP->sta_ip_addr[0] = 2;
			net_configP->sta_netmask[3] = 255;
			net_configP->sta_netmask[2] = 255;
			net_configP->sta_netmask[1] = 255;
			net_configP->sta_netmask[0] = 0;
			break;
		
		case PS_CONFIG_TYPE_TZ:
			tz_configP = (tz_config_t*) &ps_shadow_buffer[ps_sub_regions.start_index[index]];
			
			strcpy(tz_configP->tz, PS_DEFAULT_TZ);
			break;
	}
}


/**
 * Initialize our local array with default values.
 */
static void _ps_init_array()
{
	// Zero buffer
	memset(ps_shadow_buffer, 0, PS_RAM_SIZE);
	
	// Control fields
	ps_shadow_buffer[PS_MAGIC_WORD_0_ADDR] = PS_MAGIC_WORD_0;
	ps_shadow_buffer[PS_MAGIC_WORD_1_ADDR] = PS_MAGIC_WORD_1;
	ps_shadow_buffer[PS_LAYOUT_VERSION_ADDR] = PS_LAYOUT_VERSION;
	
	// Parameters
	_ps_init_config_memory(PS_CONFIG_TYPE_GUI);
	_ps_init_config_memory(PS_CONFIG_TYPE_NET);
	_ps_init_config_memory(PS_CONFIG_TYPE_TZ);
	
	// Finally compute and load checksum
	ps_shadow_buffer[PS_CHECKSUM_ADDR] = _ps_compute_checksum();
}


static bool _ps_valid_magic_word()
{
	return ((ps_shadow_buffer[PS_MAGIC_WORD_0_ADDR] == PS_MAGIC_WORD_0) &&
	        (ps_shadow_buffer[PS_MAGIC_WORD_1_ADDR] == PS_MAGIC_WORD_1));
}


static uint8_t _ps_compute_checksum()
{
	int i;
	uint8_t cs = 0;
	
	for (i=0; i<PS_CHECKSUM_ADDR; i++) {
		cs += ps_shadow_buffer[i];
	}
	
	return cs;
}


static bool _ps_write_bytes_to_gcore(uint16_t start_addr, uint16_t data_len)
{
	return (gcore_set_nvram_bytes(PS_RAM_STARTADDR + start_addr, &ps_shadow_buffer[start_addr], data_len));
}
