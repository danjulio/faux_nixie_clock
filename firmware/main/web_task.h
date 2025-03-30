/*
 *
 * Web Task - web server and associated callbacks
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
#ifndef WEB_TASK_H
#define WEB_TASK_H

#include <stdbool.h>
#include <stdint.h>

//
// WEB Task Constants
//

//
// WEB Task notifications
//

// From ctrl_task
#define WEB_NOTIFY_NETWORK_DISC_MASK        0x00000001
#define WEB_NOTIFY_SHUTDOWN_MASK            0x00000002



//
// WEB Task API
//
void web_task();
bool web_has_client();

#endif /* WEB_TASK_H */
