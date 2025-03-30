file(GLOB SOURCES *.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS . ../lvgl ../../main
                       REQUIRES esp_driver_gpio esp_driver_spi lvgl)
