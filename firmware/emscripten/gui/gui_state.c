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
#include "gui_state.h"
#include "cmd_list.h"
#include "cmd_utilities.h"



//
// Externally accessible state
//
gui_state_t gui_state = { 0 };


//
// Internal variables
//
static uint32_t gui_init_mask;



//
// API
//
#include <stdio.h>
void gui_state_init()
{
	// Request GUI state from the controller - this has to be updated whenever gui_state_t
	// is changed
	gui_init_mask = 0;
	(void) cmd_send(CMD_GET, CMD_BACKLIGHT);
	(void) cmd_send(CMD_GET, CMD_MODE);
	(void) cmd_send(CMD_GET, CMD_TIMEZONE);
	(void) cmd_send(CMD_GET, CMD_WIFI_INFO);
}


void gui_state_note_item_inited(uint32_t mask)
{
	gui_init_mask |= mask;
}


bool gui_state_init_complete()
{
	return ((gui_init_mask & GUI_STATE_INIT_ALL_MASK) == GUI_STATE_INIT_ALL_MASK);
}
