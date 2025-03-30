/**
 * @file disp_driver.c
 */

#include "disp_driver.h"
#include "disp_spi.h"
#include "ili9488.h"



void disp_driver_init(bool init_spi)
{
	if (init_spi) {
		disp_spi_init();
	}

	ili9488_init();
}

void disp_driver_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
	ili9488_flush(drv, area, color_map);
}
