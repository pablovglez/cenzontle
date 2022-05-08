# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
       SRCS "main.c"
       INCLUDE_DIRS "include"
       REQUIRES wifi_setup mqtt_controller filesystem esp32-smbus esp32-i2c-lcd1602 iua_lcd
       PRIV_REQUIRES nvs_flash
   )
