/*
IUALIA - Component to show messages on LCD screen
*/

#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
//#include "rom/uart.h"
#include "esp32/rom/uart.h"

#include "smbus.h"
#include "i2c-lcd1602.h"

#include "iua_lcd.h"

#define TAG "IUALIA"

#ifndef LOAD_PARAMS
// LCD1602
#define LCD_NUM_ROWS               2
#define LCD_NUM_COLUMNS            32
#define LCD_NUM_VISIBLE_COLUMNS    16

// LCD2004
//#define LCD_NUM_ROWS               4
//#define LCD_NUM_COLUMNS            40
//#define LCD_NUM_VISIBLE_COLUMNS    20

#define I2C_MASTER_SDA_IO        18
#define I2C_MASTER_SCL_IO        19

#define IUA_MAX_DISP_MSG             5

#endif

// Undefine USE_STDIN if no stdin is available (e.g. no USB UART) - a fixed delay will occur instead of a wait for a keypress.
//#define USE_STDIN  1
#undef USE_STDIN

#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN    0                     // disabled
#define I2C_MASTER_RX_BUF_LEN    0                     // disabled
#define I2C_MASTER_FREQ_HZ       100000




ESP_EVENT_DECLARE_BASE(IAU_SCREEN_BASE);
ESP_EVENT_DEFINE_BASE(IUA_SCREEN_BASE);


enum {
    IUA_SCREEN_EVENT_ID,                     // raised during an iteration of the loop within the task
}; 

QueueHandle_t iua_display_queue;

i2c_lcd1602_info_t * iua_lcd_info; 

i2c_lcd1602_info_t * get_lcd_struct(){
    return iua_lcd_info;
}

esp_event_loop_handle_t iua_screen_handler;

esp_event_loop_args_t iua_screen_args = {
        .queue_size = 5,
        .task_name = "iua_screen_task",
        .task_priority = tskIDLE_PRIORITY,
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };

// uart_rx_one_char_block() causes a watchdog trigger, so use the non-blocking
// uart_rx_one_char() and delay briefly to reset the watchdog.
static uint8_t _wait_for_user(void)
{
    uint8_t c = 0;

#ifdef USE_STDIN
    while (!c)
    {
       STATUS s = uart_rx_one_char(&c);
       if (s == OK) {
          printf("%c", c);
       }
       vTaskDelay(1);
    }
#else
    vTaskDelay(2000 / portTICK_RATE_MS);
#endif
    return c;
}


void display_on_event(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{   

    char *screenReceivedItem;
    if(xQueueReceive(iua_display_queue, &screenReceivedItem, 0) == pdTRUE){
        iua_lcd_print(screenReceivedItem);
        
    }
}

void iua_print(char * input_string){
    char *iua_out = malloc(strlen(input_string)); 
    strcpy(iua_out, input_string);
    ESP_LOGW(TAG, "%s",iua_out);
    
    if(xQueueSend(iua_display_queue, &iua_out, 10) == pdFALSE){
        ESP_LOGW(TAG, "Too many responses in queue!");
    }

    esp_event_post_to(iua_screen_handler, IUA_SCREEN_BASE, IUA_SCREEN_EVENT_ID, NULL, 0, portMAX_DELAY);    
    free(iua_out);
}

#ifdef LOAD_PARAMS
static void i2c_master_initp(int lcd_sda, int lcd_scl)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = lcd_sda;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
    conf.scl_io_num = lcd_scl;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_LEN,
                       I2C_MASTER_TX_BUF_LEN, 0);
}

esp_err_t iua_display_initp(int lcd_rows, int lcd_cols, int lcd_visible_columns, int lcd_max_msg){

    i2c_master_initp(lcd_rows, lcd_cols);
    
    i2c_port_t i2c_num = I2C_MASTER_NUM;
    //uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;
    uint8_t address = 0x27;

    // Set up the SMBus
    smbus_info_t * smbus_info = smbus_malloc();
    ESP_ERROR_CHECK(smbus_init(smbus_info, i2c_num, address));
    ESP_ERROR_CHECK(smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS));

    // Set up the LCD1602 device with backlight off
    iua_lcd_info = i2c_lcd1602_malloc();
    ESP_ERROR_CHECK(i2c_lcd1602_init(iua_lcd_info, smbus_info, true,
                                     lcd_rows, lcd_cols, lcd_visible_columns));

    ESP_ERROR_CHECK(i2c_lcd1602_reset(iua_lcd_info));
    
    // turn off backlight
    ESP_LOGI(TAG, "backlight off");
    _wait_for_user();
    i2c_lcd1602_set_backlight(iua_lcd_info, false);
    
    
    // turn on backlight
    ESP_LOGI(TAG, "backlight on");
    _wait_for_user();
    i2c_lcd1602_set_backlight(iua_lcd_info, true);

    esp_event_loop_create(&iua_screen_args, &iua_screen_handler);

    esp_event_handler_register_with(iua_screen_handler, IUA_SCREEN_BASE, IUA_SCREEN_EVENT_ID, display_on_event, NULL);
    iua_display_queue = xQueueCreate(lcd_max_msg, sizeof(char *));

    return ESP_OK;
}
#else
    static void i2c_master_init(void)
        {
            int i2c_master_port = I2C_MASTER_NUM;
            i2c_config_t conf;
            conf.mode = I2C_MODE_MASTER;
            conf.sda_io_num = I2C_MASTER_SDA_IO;
            conf.sda_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
            conf.scl_io_num = I2C_MASTER_SCL_IO;
            conf.scl_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
            conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
            i2c_param_config(i2c_master_port, &conf);
            i2c_driver_install(i2c_master_port, conf.mode,
                            I2C_MASTER_RX_BUF_LEN,
                            I2C_MASTER_TX_BUF_LEN, 0);
        }
    
    esp_err_t iua_display_init(){

    i2c_master_init();
    
    i2c_port_t i2c_num = I2C_MASTER_NUM;
    //uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;
    uint8_t address = 0x27;

    // Set up the SMBus
    smbus_info_t * smbus_info = smbus_malloc();
    ESP_ERROR_CHECK(smbus_init(smbus_info, i2c_num, address));
    ESP_ERROR_CHECK(smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS));

    // Set up the LCD1602 device with backlight off
    iua_lcd_info = i2c_lcd1602_malloc();
    ESP_ERROR_CHECK(i2c_lcd1602_init(iua_lcd_info, smbus_info, true,
                                     LCD_NUM_ROWS, LCD_NUM_COLUMNS, LCD_NUM_VISIBLE_COLUMNS));

    ESP_ERROR_CHECK(i2c_lcd1602_reset(iua_lcd_info));
    
    // turn off backlight
    ESP_LOGI(TAG, "backlight off");
    _wait_for_user();
    i2c_lcd1602_set_backlight(iua_lcd_info, false);
    
    
    // turn on backlight
    ESP_LOGI(TAG, "backlight on");
    _wait_for_user();
    i2c_lcd1602_set_backlight(iua_lcd_info, true);

    esp_event_loop_create(&iua_screen_args, &iua_screen_handler);

    esp_event_handler_register_with(iua_screen_handler, IUA_SCREEN_BASE, IUA_SCREEN_EVENT_ID, display_on_event, NULL);
    iua_display_queue = xQueueCreate(IUA_MAX_DISP_MSG, sizeof(char *));

    return ESP_OK;
}
#endif




esp_err_t iua_lcd_print(char * string_out){
    i2c_lcd1602_clear(iua_lcd_info);
    _wait_for_user();
    i2c_lcd1602_home(iua_lcd_info);
    i2c_lcd1602_write_string(iua_lcd_info, string_out);

    _wait_for_user();

    return ESP_OK;

}
