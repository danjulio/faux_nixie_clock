/*
 * System Configuration File
 *
 * Contains system definition and configurable items.
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
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H


// ======================================================================================
// System hardware definitions
//

#define LCD_SCK_IO        18 
#define LCD_MOSI_IO       23
#define LCD_DC_IO         27
#define LCD_CSN_IO        5
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21 

//
// Hardware Configuration
//

// I2C
#define I2C_MASTER_NUM     1
#define I2C_MASTER_FREQ_HZ 100000

// SPI
#define LCD_SPI_HOST    VSPI_HOST
#define LCD_DMA_NUM     1
#define LCD_SPI_FREQ_HZ 80000000 
#define LCD_SPI_MODE    0



// ======================================================================================
// System configuration
//

// Battery state-of-charge curve
//   Based on 0.2C discharge rate on https://www.richtek.com/battery-management/en/designing-liion.html
//   This isn't particularly accurate...
#define BATT_75_THRESHOLD     3.9
#define BATT_50_THRESHOLD     3.72
#define BATT_25_THRESHOLD     3.66
#define BATT_0_THRESHOLD      3.55
#define BATT_CRIT_THRESHOLD   3.5

// Critical Battery Detection timeout
#define CRIT_BATTERY_DET_SEC 30

// Network re-init button press interval
#define NETWORK_RESET_BTN_MSEC 2000


#endif /* SYSTEM_CONFIG_H */