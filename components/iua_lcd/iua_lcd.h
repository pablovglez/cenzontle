#ifndef __IUA_LCD_H__
#define __IUA_LCD_H__


#include "freertos/FreeRTOS.h"

#define LOAD_PARAMS 

/**
 * @brief   Initialize and set up LCD Screen.
 * 
 * @return  Return ESP_OK at success
 */
esp_err_t iua_display_init(void);

/**
 * @brief   Initialize and set up LCD Screen. 
 * 
 * @return  Return ESP_OK at success
 */
esp_err_t iua_display_init(void);

/**
 * @brief   Initialize and set up LCD Screen. 
 *          Same as before but global config is loaded by a config file
 * 
 * @return  Return ESP_OK at success
 */
esp_err_t iua_display_initp(int lcd_rows, int lcd_cols, int lcd_visible_columns, int lcd_max_msg);

/**
 * @brief   Print some text to LCD Screen
 * 
 * @param[in]  string_out String to print
 */
esp_err_t iua_lcd_print(char * string_out);

/**
 * @brief   Print some text to LCD Screen as an event
 * 
 * @param[in]  input_string String to print
 */
void iua_print(char * input_string);

#endif //__IUA_LCD_H__
