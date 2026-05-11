#ifndef __ESP_BLE_CONTROLLER_H__
#define __ESP_BLE_CONTROLLER_H__
#include <stdint.h>
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"

//#define DEVICE_NAME                   "ESP_BLE"
#define SVC_INST_ID                   0
#define CHAR_DECLARATION_SIZE         (sizeof(uint8_t))
#define GATTS_DEMO_CHAR_VAL_LEN_MAX   500
//#define BLE_CONFIRM_TIMEOUT           5 // in seconds, the time BLE Tx confirmation is used before resend

#ifdef BLE_MODE_BEACON
    #define BLE_MODE 0
#elif BLE_MODE_PERIPHERAL
    #define BLE_MODE 1
#elif BLE_MODE_SCANNER
    #define BLE_MODE 2
#endif

typedef enum {
    BEACON = 0, // Simple BLE beacon mode, broadcasting data without connection
    PERIPHERAL = 1, // BLE peripheral mode, allowing connections and data exchange with central devices
    SCANNER = 2 // BLE scanner mode, scanning for nearby beacons 
} ble_mode_t;

//CRX_BLE_SVC
// Problably need to update the UUIDs for final version and remove some of them
static uint8_t ble_svc_uuid[16] = {0}; 
static uint8_t ble_uuid_write[16] = {0}; 
//static uint8_t ble_svc_uuid[16] = {0x98, 0x27, 0x8D, 0x9C, 0x1D, 0xF0, 0xCE, 0x87, 0xF5, 0x4B, 0x98, 0x9E, 0x00, 0xFF, 0x94, 0x29}; 
//static uint8_t ble_uuid_write[16]={0x98, 0x27, 0x8D, 0x9C, 0x1D, 0xF0, 0xCE, 0x87, 0xF5, 0x4B, 0x98, 0x9E, 0x02, 0xFF, 0x94, 0x29};


static const uint16_t primary_service_uuid          = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid    = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid  = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read                 = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write                = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_notify          = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write_notify    = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
// Problably need to update the response uuid for final version
static const uint8_t ble_notify_response[9]       = {0x4e, 0x6f, 0x74, 0x69, 0x66, 0x79, 0x20, 0x6f, 0x6e};
// Client Characteristic Configuration Descriptor value for enabling notifications
static const uint8_t ble_push_ccc[2]              =  {0x4f, 0x4e}; // ON

/* Generic Response for BLE API Events */
static uint8_t ble_api_rsp[16] = {
     0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x64 
};

// Instructions to manage by the BLE API Handler
typedef enum {
    AT_EUI_HANDLER,         // Get EUI,
    ESP_VERSION_HANDLER,    // Get Version
    PATCH_WIFI_HANDLER,     // Edit WiFi settings 
    ESP_OTA_HANDLER,        // Trigger OTA for ESP
    ESP_OTA_WRITER,         // Process OTA chunks
    POST_CURRENT_TIME,      // Time handler
    CENZ_STATUS_HANDLER,      //Get Ceryx status
    REBOOT_HANDLER,         //Trigger hard reboot
    STM_VERSION_HANDLER,    // Get Version
    GET_TIME_HANDLER,       //Get current internal time
    CENZ_BLE_BEACON,          //Turn off/on iBeacon mode
    CENZ_BLE_OPTOUT,          //Show/hide Wifi Optout
} ble_api_command_t;

typedef enum
{
    BLE_GATT_SVC,

    BLE_CHAR_BH,
    BLE_CHAR_BH_VAL,
    BLE_CHAR_BH_NTF_CFG,

    BLE_API_PARS,
    BLE_API_WRITE,
    
    BLE_IDX_NB,
} cx_gatts_table;

typedef struct{
    bool connect; // Flag to save connection status of the client
    bool notify_enable; //Flag to keep track of notification state
    esp_gatt_if_t curr_gatts_if; //GATT interface of the client
    uint16_t conn_id; //Connexion ID in the internal table
    esp_bd_addr_t remote_address; //MAC Address of the Client
    uint16_t mtu; //Requested MTU
}   ble_current_gatts_connection_t;

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[BLE_IDX_NB] =
{
    // BLE Service Declaration
    [BLE_GATT_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(ble_svc_uuid), (uint8_t *)&ble_svc_uuid}},
      //sizeof(uint16_t), sizeof(BLE_SVC), (uint8_t *)&BLE_SVC}},

    // BLE API Characteristic Declaration 
    [BLE_API_PARS]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_WRITE,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},
    
    // BLE API Controller Characteristic Value
    [BLE_API_WRITE] =
    //{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&BLE_UUID_WRITE, ESP_GATT_PERM_WRITE,
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&ble_uuid_write, ESP_GATT_PERM_WRITE,
      GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(ble_api_rsp), (uint8_t *)ble_api_rsp}},

};

int ble_init(uint8_t *device_eui);

#endif // __ESP_BLE_CONTROLLER_H__