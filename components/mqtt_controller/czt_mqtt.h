#ifndef __CZT_MQTT_H__
#define __CZT_MQTT_H__


/**
 * @brief   explicitly start MQTT client.
 * 
 * @return  Return ESP_OK at success
 */
void mqtt_app_start(char * mqtt_uri);

#endif //__CZT_MQTT_H__