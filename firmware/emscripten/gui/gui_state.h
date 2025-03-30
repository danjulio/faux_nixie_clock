/*
 * Utlities for intializing GUI state from remote controller
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
#ifndef GUI_STATE_H
#define GUI_STATE_H

#include <stdbool.h>
#include <stdint.h>



//
// Constants
//

// Initialization mask
#define GUI_STATE_INIT_BACKLIGHT  0x00000001
#define GUI_STATE_INIT_MODE       0x00000002
#define GUI_STATE_INIT_TIMEZONE   0x00000004
#define GUI_STATE_INIT_WIFI       0x00000008

#define GUI_STATE_INIT_ALL_MASK   (GUI_STATE_INIT_BACKLIGHT | \
                                   GUI_STATE_INIT_MODE | \
                                   GUI_STATE_INIT_TIMEZONE | \
                                   GUI_STATE_INIT_WIFI \
                                  )

// Field lengths
#define GUI_TZ_MAX_LEN            64
#define GUI_SSID_MAX_LEN          32
#define GUI_PW_MAX_LEN            63


// Background color (should match theme background - A kludge, I know.  Specified here because
// themes don't allow direct access to it and IMHO the LVGL theme system is incredibly hard to use)
#define GUI_THEME_BG_COLOR         lv_color_hex(0x444b5a)
#define GUI_THEME_SLD_BG_COLOR     lv_color_hex(0x3d4351)
#define GUI_THEME_RLR_BG_COLOR     lv_color_hex(0x3d4351)
#define GUI_THEME_TBL_BG_COLOR     lv_color_hex(0x3d4351)



//
// Typedefs
//
// GUI state
typedef struct {
	bool hour_mode_24;
	bool mdns_en;
	bool sta_mode;
	bool sta_static_ip;
	char timezone[GUI_TZ_MAX_LEN+1];
	char ap_ssid[GUI_SSID_MAX_LEN+1];
	char sta_ssid[GUI_SSID_MAX_LEN+1];
	char ap_pw[GUI_PW_MAX_LEN+1];
	char sta_pw[GUI_PW_MAX_LEN+1];
	uint8_t ap_ip_addr[4];
	uint8_t sta_ip_addr[4];
	uint8_t sta_netmask[4];
	uint32_t lcd_brightness;
} gui_state_t;


//
// Externally accessible state
//
extern gui_state_t gui_state;


//
// API
//
void gui_state_init();
void gui_state_note_item_inited(uint32_t mask);
bool gui_state_init_complete();

#endif /* GUI_STATE_H */
