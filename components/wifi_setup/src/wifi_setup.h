#ifndef WIFI_SETUP_H_
#define WIFI_SETUP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"


/**
 * @brief Will initialize, setup WiFi and connect to defined SSID
 *
 * @param[in] None
 * @param[out] None
 *
 * @return
 *  - None
 */
void wifi_task(char* wifi_ssid, char * wifi_pass);

/**
 * @brief Register the events for wifi task.
 *
 * @param[in] event_handler the handler function which gets called when the event is dispatched
 * @param[in] event_base the base id of the event to register the handler for
 * @param[in] event_id the id of the event to register the handler for
 * @param[in] event_handler_arg data, aside from event data, that is passed to the handler when it is called
 *
 * @note the event loop library does not maintain a copy of event_handler_arg, therefore the user should
 * ensure that event_handler_arg still points to a valid location by the time the handler gets called
 *
 * @return
 *  - None
 */
void wifi_event_handler(void* handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#endif // #ifndef WIFI_SETUP_H_