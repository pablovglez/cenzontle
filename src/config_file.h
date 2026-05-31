#ifndef __CONFIG_FILE_H__
#define __CONFIG_FILE_H__

#if defined(BLE_MODE_BEACON)
#define CONF_FILEPATH        "/spiffs/settings_beacon.conf"
#elif defined(BLE_MODE_PERIPHERAL)
#define CONF_FILEPATH        "/spiffs/settings_peripheral.conf"
#else
#define CONF_FILEPATH        "/spiffs/settings.conf"
#endif
#endif //__CONFIG_FILE_H__
