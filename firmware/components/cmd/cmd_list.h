/*
 * Command definition for packets sent over the websocket interface.
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
#ifndef CMD_LIST_H
#define CMD_LIST_H

#include <stdbool.h>
#include <stdint.h>



// Command list (alphabetical order, starting with a value of 0)
typedef enum {
	CMD_BACKLIGHT = 0,
    CMD_INFO,
    CMD_MODE,
    CMD_POWEROFF,
    CMD_SHUTDOWN,
    CMD_SYS_INFO,
    CMD_TIME,
	CMD_TIMEZONE,
	CMD_WIFI_INFO
} cmd_id_t;

// Total Count should always use the last entry
#define CMD_TOTAL_COUNT   ((uint32_t) CMD_WIFI_INFO + 1)


#endif /* CMD_LIST_H */
