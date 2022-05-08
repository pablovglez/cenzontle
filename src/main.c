#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "wifi_setup.h"
#include "czt_mqtt.h"
#include "iua_lcd.h"
#include "parameters.h"
#include "czt_config.h"

char * TAG = "MAIN";


/****** STEPPER INIT
 * 
 * 
 * 
 *  
*/
struct stepper_pins{
    uint8_t pin1;
    uint8_t pin2;
    uint8_t pin3;
    uint8_t pin4;
};

const uint8_t steps_port[8] = {0x09,0x01,0x03,0x02,0x06,0x04,0x0c,0x08};

void stepper_init(struct stepper_pins *stepper_ptr)
{
    gpio_set_direction(stepper_ptr->pin1,GPIO_MODE_OUTPUT);
    gpio_set_direction(stepper_ptr->pin2,GPIO_MODE_OUTPUT);
    gpio_set_direction(stepper_ptr->pin3,GPIO_MODE_OUTPUT);
    gpio_set_direction(stepper_ptr->pin4,GPIO_MODE_OUTPUT);

}

void step(struct stepper_pins *stepper_ptr, int step)
{
    gpio_set_level(stepper_ptr->pin1,step&1);
    gpio_set_level(stepper_ptr->pin2,(step&2)>>1);
    gpio_set_level(stepper_ptr->pin3,(step&4)>>2);
    gpio_set_level(stepper_ptr->pin4,(step&8)>>3);
}

void full_steps(struct stepper_pins *stepper_ptr,int steps, bool dir)
{       
    int n;
    int i = 0;

    if(dir){
        i=0;
		for(n=0;n<steps;n++){
		    if(i>8)
		        i=0;
		    step(stepper_ptr, steps_port[i]);
		    vTaskDelay(5 / portTICK_PERIOD_MS );    
		    i++;
		}
    }

    else{
        i=7;
		for(n=0;n<steps;n++){
		    if(i<0)
		        i=7;
		    step(stepper_ptr, steps_port[i]);
		    vTaskDelay(5 / portTICK_PERIOD_MS);    
		    i--;
		}
    }
}


void led_ctl(void *pvParams){
    gpio_pad_select_gpio(32);
    gpio_set_direction (32,GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(32,1);
        vTaskDelay(1000/portTICK_RATE_MS);
        gpio_set_level(32,0);
        vTaskDelay(1000/portTICK_RATE_MS);
    }
}

void stepper_task(void *pvParams){
    struct stepper_pins stepper0;

    stepper0.pin1 = 27;
    stepper0.pin2 = 26;
    stepper0.pin3 = 25;
    stepper0.pin4 = 33;

    stepper_init(&stepper0);
    
    while (true) {
        
        //cz_print("Motor clockwise");
        full_steps(&stepper0, 2000, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
        //cz_print("Motor counter-clockwise");
        full_steps(&stepper0, 2000, 0); 
        vTaskDelay(1000 / portTICK_PERIOD_MS);
               

    }
}
/****** STEPPER END
 * 
 * 
 * 
 *  
*/

static esp_err_t init_spiffs(void) {
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

void app_main(void)
{
    ESP_ERROR_CHECK(init_spiffs());
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Load parameters
    loadPersistentSettings(CZT_CONF_FILEPATH);

    ESP_LOGI(TAG, "TEST %u", global_params.random_number);

    wifi_task(global_params.wifi_ssid, global_params.wifi_pass);
    //cz_display_init();
    //cz_display_init();

    iua_print("Cenzontle");
    
    mqtt_app_start(global_params.mqtt_uri);

    xTaskCreate(&led_ctl,"LED_BLINK",1024,NULL,0,NULL);

    iua_print("Motor start");
    xTaskCreate(&stepper_task,"MOTOR_STEP",1024,NULL,0,NULL);
    //stepper_task(NULL);

}