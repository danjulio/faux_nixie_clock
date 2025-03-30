/*
 * Command handlers updating or retrieving application state
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
#ifndef CMD_HANDLERS_H
#define CMD_HANLDERS_H

#include "cmd_utilities.h"
#include <stdbool.h>
#include <stdint.h>



//
// API
//
void cmd_handler_get_backlight(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_get_mode(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_get_sys_info(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_get_time(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_get_timezone(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_get_wifi(cmd_data_t data_type, uint32_t len, uint8_t* data);

void cmd_handler_set_backlight(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_set_mode(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_set_poweroff(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_set_time(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_set_timezone(cmd_data_t data_type, uint32_t len, uint8_t* data);
void cmd_handler_set_wifi(cmd_data_t data_type, uint32_t len, uint8_t* data);

#endif /* CMD_HANDLERS_H */
