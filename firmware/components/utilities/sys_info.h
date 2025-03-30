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
#ifndef SYS_INFO_H
#define SYS_INFO_H

#include <stdbool.h>
#include <stdint.h>

//
// Constants
//
#define SYS_INFO_MAX_LEN 1024


//
// API
//
char* sys_info_get_string();


#endif /* SYS_INFO_H */