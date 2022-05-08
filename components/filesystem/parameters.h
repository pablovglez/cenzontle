#ifndef __G_PARAMS_H__
#define __G_PARAMS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define UUID_SZ         37
#define CONF_LINE_SIZE  64
#define MAX_ADV_NAME    20

typedef enum {
    WF_SSID,
    WF_PASS,
    MQTT_URI,
    BT_NAME,
    BLE_UUID,
    LCD_ROWS,
    LCD_COLS,
    LCD_VIS_COLS,
    LCD_SDA,
    LCD_SCL,
    LCD_MAX_MSG,
    RDM_NUM,
    PARAM_END
} CztParamEnum;

typedef struct Settings {
  char wifi_ssid[CONF_LINE_SIZE/2];
  char wifi_pass[CONF_LINE_SIZE];
  char mqtt_uri[CONF_LINE_SIZE/2];
  char bt_name[MAX_ADV_NAME];
  char ble_uiid[UUID_SZ];
  int lcd_rows;
  int lcd_cols;
  int lcd_visible_columns;
  int lcd_sda;
  int lcd_scl;
  int lcd_max_msg;
  bool random_number; //just a number to remember how to parse numbers
} CztPersistentSettings;

extern CztPersistentSettings global_params;

/**
 * @brief Read persistent parameters from a file
 * 
 * @return Number of parameters successfully read
 */
int loadPersistentSettings(const char* filename);

/**
 * @brief Write persistent parameters to a file
 * 
 * @return Number of parameters successfully written
 */
int pushPersistentSettings(const char* filename);

/**
 * @brief Read the settings file, and print each line to the console
 * 
 * @return Number of parameters successfully read
 */
int printPersistentSettingsFile(const char* filename);

/**
 * @brief Reset the default configuration (to the default_params variable value) and write it to the file
 * 
 * @return Number of parameters successfully written
 */
int resetPersistentSettings(const char* filename);

#endif