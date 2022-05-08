idf_component_register(SRCS "iua_lcd.c"
    INCLUDE_DIRS "." 
    PRIV_REQUIRES spiffs esp32-i2c-lcd1602 esp32-smbus)
