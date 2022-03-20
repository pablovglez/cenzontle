#include "esp_err.h"
#include "esp_log.h"

#include "parameters.h"

static const char *TAG = "CX_CONF";
const char *g_params_names[] = {
    "wifi_ssid",
    "wifi_pass",
    "mqtt_uri",
    "BLE_minor",
    "bt_name",
    "ble_uiid",
    "random_number"
    };

CztPersistentSettings global_params = {"fake_ap", "dummy", "mqtt://fake_mqtt.com:1883", "Cenzontle_BLE", "ffeeddcc-bbaa-9988-7766-554433221100",0};

int loadPersistentSettings(const char* filename) {
    CztParamEnum next = WF_SSID;
    char line[CONF_LINE_SIZE];

    FILE *conf_file = fopen(filename, "r");
    while (fgets(line, CONF_LINE_SIZE, conf_file)) {
        line[strcspn(line, "\n")] = 0;
        const char* val = strrchr(line, '=') + 1; // +1 to remove the '=' char
        switch (next) {
        case WF_SSID:
            strcpy(global_params.wifi_ssid, val);
            next = WF_PASS;
            break;
        case WF_PASS:
            strcpy(global_params.wifi_pass, val);
            next = MQTT_URI;
            break;
        case MQTT_URI:
            strcpy(global_params.mqtt_uri, val);
            next = BT_NAME;
            break;
        case BT_NAME:
            strcpy(global_params.bt_name, val);
            next = BLE_UUID;
            break;
        case BLE_UUID:
            strcpy(global_params.ble_uiid, val);
            next = RDM_NUM;
            break;
        case RDM_NUM:
            global_params.random_number = atoi(val);
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
    CztParamEnum next = WF_SSID;
    FILE *conf_file = fopen(filename, "w");
    if (!conf_file)
        return ESP_FAIL;
    fseek(conf_file, 0, SEEK_SET);

    while (next != PARAM_END) {
        switch (next) {
        case WF_SSID:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.wifi_ssid);
            next = WF_PASS;
            break;
        case WF_PASS:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.wifi_pass);
            next = MQTT_URI;
            break;
        case MQTT_URI:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.mqtt_uri);
            next = BT_NAME;
            break;
        case BT_NAME:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.bt_name);
            next = BLE_UUID;
            break;
        case BLE_UUID:
            fprintf(conf_file, "%s=%s\n", g_params_names[next], global_params.ble_uiid);
            next = RDM_NUM;
            break;
        case RDM_NUM:
            fprintf(conf_file, "%s=%d\n", g_params_names[next], global_params.random_number);
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