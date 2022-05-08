get_filename_component(MYPROJECT_DIR ../ ABSOLUTE)

idf_component_register(
       SRCS "czt_mqtt.c"
       INCLUDE_DIRS "."
       PRIV_REQUIRES mqtt
   )
