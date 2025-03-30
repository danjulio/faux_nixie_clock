/*
 * Control Interface Task - Manage platform activities including battery monitoring,
 * shutdown control and persistant storage.
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
#ifndef CTRL_TASK_H
#define CTRL_TASK_H

#include <stdbool.h>
#include <stdint.h>


//
// Control Task Constants
//

// Control Task evaluation interval
#define CTRL_EVAL_MSEC                 50

// Battery monitoring periods (mSec)
#define CTRL_BATT_SAMPLE_MSEC          500

// Write NVRAM to flash check period (mSec)
#define CTRL_NVRAM_SAVE_MSEC           60000

// Control Task notifications
#define CTRL_NOTIFY_RESTART_NETWORK    0x00000001
#define CTRL_NOTIFY_SHUTDOWN           0x00000002
#define CTRL_NOTIFY_UPD_BACKLIGHT      0x00000010



//
// Control Task API
//
void ctrl_task();

#endif /* CTRL_TASK_H */