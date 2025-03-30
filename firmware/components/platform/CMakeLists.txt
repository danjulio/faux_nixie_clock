file(GLOB SOURCES *.c)

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS . ../i2c ../../main
                       REQUIRES esp_adc esp_driver_gpio)
