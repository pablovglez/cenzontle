#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

// Constants
#define WIFI_SSID    "Raspberry"  //"Moto G (5) Plus 8389"//
#define WIFI_PASS   "Q3kaJ*iBma3aK@"
#define EXAMPLE_ESP_MAXIMUM_RETRY  20
#define MAX_HTTP_OUTPUT_BUFFER 4096  //<-- Replace by malloc



// Ported projects

// Variables
static int s_retry_num = 0;
   

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_FAIL_BIT = BIT1;



// Function Prototypes
void wifi_task();
static void wifi_event_handler();
esp_err_t http_event_handler();


void wifi_task(char* wifi_ssid, char * wifi_pass)
{
    const char *TAG = "WIFI_TASK";
    //esp_err_t ret;
    s_wifi_event_group = xEventGroupCreate();
    //tcpip_adapter_init();
    
    ESP_ERROR_CHECK(esp_netif_init());
    //esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();


    //ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_LOGI(TAG, "Starting esp_wifi_init");
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    
    //ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config =
        {
            .sta = {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
                //.threshold.authmode = WIFI_AUTH_WPA2_PSK,
                .pmf_cfg = {
                    .capable = true,
                    .required = false
                },
            },
        };

    if(wifi_ssid != NULL || wifi_pass != NULL){
        memcpy(wifi_config.sta.ssid, wifi_ssid, strlen(wifi_ssid));
        memcpy(wifi_config.sta.password, wifi_pass, strlen(wifi_pass));
    }
    
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    ESP_LOGI(TAG, "SSID %s", wifi_config.sta.ssid);
    ESP_LOGI(TAG, "PASS %s", wifi_config.sta.password);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to AP %s",
                 wifi_config.sta.ssid);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to to AP %s",
                 wifi_config.sta.ssid);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler));
    vEventGroupDelete(s_wifi_event_group);
}


void wifi_event_handler(void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    const char *TAG = "WIFI_HANDLER";
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        //ESP_LOGI(TAG, "Connecting");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);  // <--- This helps to retry to connect
            //xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);  
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        
    }
}
