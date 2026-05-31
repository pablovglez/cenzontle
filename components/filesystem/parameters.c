#include "esp_err.h"
#include "esp_log.h"

#include "parameters.h"

static const char *TAG = "CONFIG_MGR";

int dloadPersistentSettings(const char* filename) {

    return 0;
}

#ifdef BLE_MODE_BEACON
const char *g_params_names[] = {
    "project_name",
    "ble_uuid",
    "ble_major",
    "ble_minor"
    };

PersistentSettings global_params = {"ESP32", {0}, 1, 2}; 

int loadPersistentSettings(const char* filename) {
    IxtliParamEnum next = PROJECT_NAME;
    char line[CONF_LINE_SIZE];

    FILE *conf_file = fopen(filename, "r");
    while (fgets(line, CONF_LINE_SIZE, conf_file)) {
        line[strcspn(line, "\n")] = 0;
        // Skip empty lines and comment lines
        if (line[0] == '\0' || line[0] == ';' || line[0] == '#') continue;
        const char* val = strrchr(line, '=') + 1; // +1 to remove the '=' char
        switch (next) {
        case PROJECT_NAME:
            strcpy(global_params.project_name, val);
            next = BLE_UUID;
            break;
        case BLE_UUID:
            ESP_LOGI(TAG, "Parsing BLE UUID: %s", val);
            for (int i = 0; i < 16; i++) {
                // Skip if val is whitespace
                if (val[i * 2] == ' ' || val[i * 2] == '\t') {
                    continue;
                }
                sscanf(&val[i * 2], "%2hhx", &global_params.ble_uuid[i]);
            }
            next = BLE_MAJOR;
        case BLE_MAJOR:
            global_params.ble_major = (uint16_t)atoi(val);
            next = BLE_MINOR;
            break;
        case BLE_MINOR:
            global_params.ble_minor = (uint16_t)atoi(val);
            next = PARAM_END;
            break;
        case PARAM_END:
        default:
            goto free_file;
            break;
        }
    }

free_file:
    fclose(conf_file);
    return (int)next;
}

int pushPersistentSettings(const char* filename) {
    IxtliParamEnum next = PROJECT_NAME;
    FILE *conf_file = fopen(filename, "w");
    if (!conf_file)
        return ESP_FAIL;
    fseek(conf_file, 0, SEEK_SET);

    while (next != PARAM_END) {
        switch (next) {
        case PROJECT_NAME:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.project_name);
            next = BLE_UUID;
            break;
        case BLE_UUID:  
            fprintf(conf_file, "%s=", g_params_names[next]);
            for (int i = 0; i < 16; i++) {
                fprintf(conf_file, "%02X", global_params.ble_uuid[i]);
            }
            fprintf(conf_file, "\n");
            next = BLE_MAJOR;
            break;
        case BLE_MAJOR:
            fprintf(conf_file, "%ul", global_params.ble_major);
            next = BLE_MINOR;
            break;
        case BLE_MINOR:
            fprintf(conf_file, "%ul", global_params.ble_minor);
            next = PARAM_END;
            break;
        case PARAM_END:
        default:
            goto free_file;
        }
    }

free_file:
    fclose(conf_file);
    return (int)next;
}
#elif defined (BLE_MODE_PERIPHERAL)

const char *g_params_names[] = {
    "project_name",
    "ds18b20_gpio_pin",
    "poll_interval_ms",
    "neo6m_gpio_tx_pin",
    "neo6m_gpio_rx_pin",
    "neo6m_uart_port_num",
    "wifi_ssid",
    "wifi_pass",
    "mqtt_broker_url",
    "mqtt_base_topic",
    "ble_uuid"
    };

PersistentSettings global_params = {"ESP32", {0}};

int loadPersistentSettings(const char* filename) {
    IxtliParamEnum next = PROJECT_NAME;
    char line[CONF_LINE_SIZE];

    FILE *conf_file = fopen(filename, "r");
    while (fgets(line, CONF_LINE_SIZE, conf_file)) {
        line[strcspn(line, "\n")] = 0;
        // Skip empty lines and comment lines
        if (line[0] == '\0' || line[0] == ';' || line[0] == '#') continue;
        const char* val = strrchr(line, '=') + 1; // +1 to remove the '=' char
        switch (next) {
        case PROJECT_NAME:
            strcpy(global_params.project_name, val);
            next = BLE_UUID;
            break;
        case BLE_UUID:
            ESP_LOGI(TAG, "Parsing BLE UUID: %s", val);
            for (int i = 0; i < 16; i++) {
                // Skip if val is whitespace
                if (val[i * 2] == ' ' || val[i * 2] == '\t') {
                    continue;
                }
                sscanf(&val[i * 2], "%2hhx", &global_params.ble_uuid[i]);
            }
            next = PARAM_END;
            break;
        case PARAM_END:
        default:
            goto free_file;
            break;
        }
    }

free_file:
    fclose(conf_file);
    return (int)next;
}

int pushPersistentSettings(const char* filename) {
    IxtliParamEnum next = PROJECT_NAME;
    FILE *conf_file = fopen(filename, "w");
    if (!conf_file)
        return ESP_FAIL;
    fseek(conf_file, 0, SEEK_SET);

    while (next != PARAM_END) {
        switch (next) {
        case PROJECT_NAME:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.project_name);
            next = BLE_UUID;
            break;
        case BLE_UUID:  
            fprintf(conf_file, "%s=", g_params_names[next]);
            for (int i = 0; i < 16; i++) {
                fprintf(conf_file, "%02X", global_params.ble_uuid[i]);
            }
            fprintf(conf_file, "\n");
            next = PARAM_END;
            break;
        case PARAM_END:
        default:
            goto free_file;
        }
    }

free_file:
    fclose(conf_file);
    return (int)next;
}

#else
const char *g_params_names[] = {
    "project_name",
    "ds18b20_gpio_pin",
    "poll_interval_ms",
    "neo6m_gpio_tx_pin",
    "neo6m_gpio_rx_pin",
    "neo6m_uart_port_num",
    "wifi_ssid",
    "wifi_pass",
    "mqtt_broker_url",
    "mqtt_base_topic",
    "ble_uuid"
    };

PersistentSettings global_params = {"ESP32", 4, 60000, 17, 16, 1, "demo", "******", "mqtt://localhost", "esp32", {0}};

int loadPersistentSettings(const char* filename) {
    IxtliParamEnum next = PROJECT_NAME;
    char line[CONF_LINE_SIZE];

    FILE *conf_file = fopen(filename, "r");
    while (fgets(line, CONF_LINE_SIZE, conf_file)) {
        line[strcspn(line, "\n")] = 0;
        // Skip empty lines and comment lines
        if (line[0] == '\0' || line[0] == ';' || line[0] == '#') continue;
        const char* val = strrchr(line, '=') + 1; // +1 to remove the '=' char
        switch (next) {
        case PROJECT_NAME:
            strcpy(global_params.project_name, val);
            next = DS18B20_GPIO_PIN;
            break;
        case DS18B20_GPIO_PIN:
            global_params.ds18b20_gpio_pin = atoi(val);
            next = POLL_INTERVAL_MS;
            break;
        case POLL_INTERVAL_MS:
            global_params.poll_interval_ms = atoi(val);
            next = NEO6M_GPIO_TX_PIN;
            break;
        case NEO6M_GPIO_TX_PIN:
            global_params.neo6m_gpio_tx_pin = atoi(val);
            next = NEO6M_GPIO_RX_PIN;
            break;
        case NEO6M_GPIO_RX_PIN:
            global_params.neo6m_gpio_rx_pin = atoi(val);
            next = NEO6M_UART_PORT_NUM;
            break;
        case NEO6M_UART_PORT_NUM:
            global_params.neo6m_uart_port_num = atoi(val);
            next = WIFI_SSID;
            break;
        case WIFI_SSID:
            strcpy(global_params.wifi_ssid, val);
            next = WIFI_PASS;
            break;
        case WIFI_PASS:
            strcpy(global_params.wifi_pass, val);
            next = MQTT_BROKER_URL;
            break;
        case MQTT_BROKER_URL:
            strcpy(global_params.mqtt_broker_url, val);
            next = MQTT_BASE_TOPIC;
            break;
        case MQTT_BASE_TOPIC:
            strcpy(global_params.mqtt_base_topic, val);
            next = BLE_UUID;
            break;
        case BLE_UUID:
            ESP_LOGI(TAG, "Parsing BLE UUID: %s", val);
            for (int i = 0; i < 16; i++) {
                // Skip if val is whitespace
                if (val[i * 2] == ' ' || val[i * 2] == '\t') {
                    continue;
                }
                sscanf(&val[i * 2], "%2hhx", &global_params.ble_uuid[i]);
            }
            next = PARAM_END;
            break;
        case PARAM_END:
        default:
            goto free_file;
            break;
        }
    }

free_file:
    fclose(conf_file);
    return (int)next;
}

int pushPersistentSettings(const char* filename) {
    IxtliParamEnum next = PROJECT_NAME;
    FILE *conf_file = fopen(filename, "w");
    if (!conf_file)
        return ESP_FAIL;
    fseek(conf_file, 0, SEEK_SET);

    while (next != PARAM_END) {
        switch (next) {
        case PROJECT_NAME:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.project_name);
            next = DS18B20_GPIO_PIN;
            break;
        case DS18B20_GPIO_PIN:
            fprintf(conf_file, "%s=%d\n", g_params_names[next], global_params.ds18b20_gpio_pin);
            next = POLL_INTERVAL_MS;
            break;
        case POLL_INTERVAL_MS:
            fprintf(conf_file, "%s=%d\n", g_params_names[next], global_params.poll_interval_ms);
            next = NEO6M_GPIO_TX_PIN;
            break;
        case NEO6M_GPIO_TX_PIN:
            fprintf(conf_file, "%s=%d\n", g_params_names[next], global_params.neo6m_gpio_tx_pin);
            next = NEO6M_GPIO_RX_PIN;
            break;
        case NEO6M_GPIO_RX_PIN:
            fprintf(conf_file, "%s=%d\n", g_params_names[next], global_params.neo6m_gpio_rx_pin);
            next = NEO6M_UART_PORT_NUM;
            break;
        case NEO6M_UART_PORT_NUM:
            fprintf(conf_file, "%s=%d\n", g_params_names[next], global_params.neo6m_uart_port_num);
            next = WIFI_SSID;
            break;
        case WIFI_SSID:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.wifi_ssid);
            next = WIFI_PASS;
            break;
        case WIFI_PASS:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.wifi_pass);
            next = MQTT_BROKER_URL;
            break;
        case MQTT_BROKER_URL:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.mqtt_broker_url);
            next = MQTT_BASE_TOPIC;
            break;
        case MQTT_BASE_TOPIC:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.mqtt_base_topic);
            next = BLE_UUID;
            break;
        case BLE_UUID:  
            fprintf(conf_file, "%s=", g_params_names[next]);
            for (int i = 0; i < 16; i++) {
                fprintf(conf_file, "%02X", global_params.ble_uuid[i]);
            }
            fprintf(conf_file, "\n");
            next = PARAM_END;
            break;
        case PARAM_END:
        default:
            goto free_file;
        }
    }

free_file:
    fclose(conf_file);
    return (int)next;
}
#endif