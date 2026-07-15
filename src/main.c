#include <esp_system.h>
#include <nvs_flash.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_spiffs.h"
#include "esp_log.h"

#include "config_file.h"
#include "parameters.h"
#include "version.h"

#if !defined(BLE_MODE_BEACON) && !defined(BLE_MODE_PERIPHERAL)
#include "connect_wifi.h"
#endif

#ifdef BLE_ENABLED
#include "esp_bt.h"
#include "esp_ble_controller.h"
#include "esp_bt_main.h"
#endif

#ifdef RELAY_MODE
#include "relay_controller.h"
#endif

#include <stdbool.h>
#include <inttypes.h>

static char* TAG = "CENZONTLE";

static esp_err_t init_spiffs(void){
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 20,   // This decides the maximum number of files that can be created on the storage
      .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Mount/Format failed");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }
    else {
        size_t spiffs_total = 0, spiffs_used = 0;
        esp_spiffs_info(NULL, &spiffs_total, &spiffs_used);
        ESP_LOGI(TAG, "SPIFFS space used %dB/%dB.", spiffs_used, spiffs_total);
    }
    return ESP_OK;
}

void app_main(){
    show_version();
    ESP_ERROR_CHECK(init_spiffs());

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    // Load parameters
    loadPersistentSettings(CONF_FILEPATH);
    
    // If BLE_MODE != 0 We initialze WiFi
    #if !defined(BLE_MODE_BEACON) && !defined(BLE_MODE_PERIPHERAL)

    connect_wifi(global_params.project_name, global_params.wifi_ssid, global_params.wifi_pass);
    char ip_address[16] = {0};
    get_ip_address(ip_address, 16);
    
    #endif
    
    #ifdef RELAY_MODE
    relay_init();
    #endif
    
    #ifdef BLE_ENABLED

    uint8_t mac[6];
    get_ble_mac_address(mac);
    ble_init(mac, global_params.ble_uuid);

    #endif
    
}