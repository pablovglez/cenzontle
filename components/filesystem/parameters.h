#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define UUID_SZ         37
#define CONF_LINE_SIZE  64
#define MAX_ADV_NAME    20

#ifdef BLE_MODE_BEACON

typedef enum {
    PROJECT_NAME,
    BLE_UUID,
    BLE_MAJOR,
    BLE_MINOR
    PARAM_END,
} IxtliParamEnum;

typedef struct Settings {
    char project_name[MAX_ADV_NAME];
    uint8_t ble_uuid[16];
    uint16_t ble_major;
    uint16_t ble_minor;
} PersistentSettings;
#elif defined(BLE_MODE_PERIPHERAL)
typedef enum {
    PROJECT_NAME,
    BLE_UUID,
    FORCE_RELAY_UP,
    PARAM_END
} IxtliParamEnum;

typedef struct Settings {
    char project_name[MAX_ADV_NAME];
    uint8_t ble_uuid[16];
    uint8_t force_relay_up;
} PersistentSettings;

#else
typedef enum {
    PROJECT_NAME,
    DS18B20_GPIO_PIN,
    POLL_INTERVAL_MS,
    NEO6M_GPIO_TX_PIN,
    NEO6M_GPIO_RX_PIN,
    NEO6M_UART_PORT_NUM,
    WIFI_SSID,
    WIFI_PASS,
    MQTT_BROKER_URL,
    MQTT_BASE_TOPIC,
    BLE_UUID,
    PARAM_END
} IxtliParamEnum;

typedef struct Settings {
  char project_name[MAX_ADV_NAME];
  int ds18b20_gpio_pin;
  int poll_interval_ms;
  int neo6m_gpio_tx_pin;
  int neo6m_gpio_rx_pin;
  int neo6m_uart_port_num;
  char wifi_ssid[MAX_ADV_NAME];
  char wifi_pass[MAX_ADV_NAME];
  char mqtt_broker_url[CONF_LINE_SIZE];
  char mqtt_base_topic[MAX_ADV_NAME];
  uint8_t ble_uuid[16];
} PersistentSettings;

#endif

extern PersistentSettings global_params;

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