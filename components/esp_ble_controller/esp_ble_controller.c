
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"

#include "esp_gatt_common_api.h"
#include "esp_gattc_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"


#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "esp_ble_controller.h"

#define BLE_BASE_UUID {0xF9, 0x80, 0xAB, 0x40, 0x65, 0xF5, 0x44, 0x67, 0x00, 0x00, 0xEA, 0x02, 0x49, 0x16, 0x63, 0xC3}

#define IBEACON_SENDER 1
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

#define ESP_APP_ID                    0x55
#define PROFILE_APP_IDX               0

static const char *TAG = "BLE_CONTROLLER";
static uint8_t beacon_uuid[16] = BLE_BASE_UUID;
uint8_t device_name[32] = {0};
static uint8_t adv_config_done      = 0;

static uint16_t ble_handle_table[BLE_IDX_NB];

static ble_current_gatts_connection_t curr_client;

uint16_t* ble_handle_table_ptr(void) { return ble_handle_table; }
ble_current_gatts_connection_t* ble_curr_client_ptr(void) { return &curr_client; }

// Put unique part to UUID by changing Least Significant Bytes with HEX representation of last 8 characters of EUI
void set_uuid(uint8_t *device_eui){
    
    for(int i = 0; i < 6; i++){
        beacon_uuid[15 - i] = device_eui[5 - i];
    }

    // Set the device name
    snprintf((char*)device_name, sizeof(device_name), "CENZ-%02X%02X%02X%02X", device_eui[2], device_eui[3], device_eui[4], device_eui[5]);

    #ifdef BLE_MODE_PERIPHERAL

    for(int i = 0; i < 8; i++){
        uint8_t t = beacon_uuid[i];
        beacon_uuid[i] = beacon_uuid[15-i];
        beacon_uuid[15-i] = t;
    };

    // Set GATT UUIDs
    static uint8_t ble_svc_uuid[16] = {0};
    memcpy(ble_svc_uuid, beacon_uuid, sizeof(beacon_uuid));
    ble_svc_uuid[8] = 0x01; // Change the 13th byte to set the service UUID

    static uint8_t ble_uuid_write[16]= {0};
    memcpy(ble_uuid_write, beacon_uuid, sizeof(beacon_uuid));
    ble_uuid_write[8] = 0x02; // Change the 13th byte to set the characteristic UUID for write

    // Update the middle bytes of the UUIDs for service and characteristic

            
    #endif    
}

void ble_stack_initialize(uint8_t *device_eui){

    set_uuid(device_eui);
    //ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
}
#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t notify_char_handle;
    esp_bd_addr_t remote_bda;
};

typedef struct {
    uint8_t flags[3];
    uint8_t length;
    uint8_t type;
    uint16_t company_id;
    uint16_t beacon_type;
}__attribute__((packed)) esp_ble_ibeacon_head_t;

typedef struct {
    uint8_t proximity_uuid[16];
    uint16_t major;
    uint16_t minor;
    int8_t measured_power;
}__attribute__((packed)) esp_ble_ibeacon_vendor_t;

typedef struct {
    esp_ble_ibeacon_head_t ibeacon_head;
    esp_ble_ibeacon_vendor_t ibeacon_vendor;
}__attribute__((packed)) esp_ble_ibeacon_t;

esp_ble_ibeacon_head_t ibeacon_common_head = {
    .flags = {0x02, ESP_BLE_AD_TYPE_FLAG, ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT},
    .length = 0x1A,
    .type = ESP_BLE_AD_MANUFACTURER_SPECIFIC_TYPE,
    .company_id = 0x004C,
    .beacon_type = 0x1502
};

bool esp_ble_is_ibeacon_packet (uint8_t *adv_data, uint8_t adv_data_len){
    bool result = false;

    if ((adv_data != NULL) && (adv_data_len == 0x1E)){
        if (!memcmp(adv_data, (uint8_t*)&ibeacon_common_head, sizeof(ibeacon_common_head))){
            result = true;
        }
    }

    return result;
}

esp_err_t esp_ble_config_ibeacon_data (esp_ble_ibeacon_vendor_t *vendor_config, esp_ble_ibeacon_t *ibeacon_adv_data){
    
    /*
    (esp_ble_ibeacon_vendor_t *vendor_config, esp_ble_ibeacon_t *ibeacon_adv_data){
    if ((vendor_config == NULL) || (ibeacon_adv_data == NULL) || (!memcmp(vendor_config->proximity_uuid, uuid_zeros, sizeof(uuid_zeros)))){
        return ESP_ERR_INVALID_ARG;
    }
    */
    if (ibeacon_adv_data == NULL) {
        ESP_LOGE(TAG, "Invalid ibeacon_adv_data pointer");
        return ESP_ERR_INVALID_ARG;
    }
    else if (vendor_config == NULL) {
        ESP_LOGE(TAG, "Invalid vendor_config pointer");
        return ESP_ERR_INVALID_ARG;
    }
    //memcpy(cx_vendor_config.proximity_uuid, uuid_to_buf(global_params.BLE_uuid), ESP_UUID_LEN_128);
    //else if (!memcmp(vendor_config->proximity_uuid, uuid_zeros, sizeof(uuid_zeros))) {
    
    else if (!memcmp(vendor_config->proximity_uuid, beacon_uuid, sizeof(beacon_uuid))) {
        ESP_LOGE(TAG, "Invalid proximity UUID");
        return ESP_ERR_INVALID_ARG;
    }
    //memcpy(vendor_config->proximity_uuid, beacon_uuid, ESP_UUID_LEN_128);

    ESP_LOGI(TAG, "SET UUID: %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
             vendor_config->proximity_uuid[0], vendor_config->proximity_uuid[1], vendor_config->proximity_uuid[2], vendor_config->proximity_uuid[3],
             vendor_config->proximity_uuid[4], vendor_config->proximity_uuid[5], vendor_config->proximity_uuid[6], vendor_config->proximity_uuid[7],
             vendor_config->proximity_uuid[8], vendor_config->proximity_uuid[9], vendor_config->proximity_uuid[10], vendor_config->proximity_uuid[11], 
             vendor_config->proximity_uuid[12], vendor_config->proximity_uuid[13], vendor_config->proximity_uuid[14], vendor_config->proximity_uuid[15]);

    memcpy(&ibeacon_adv_data->ibeacon_head, &ibeacon_common_head, sizeof(esp_ble_ibeacon_head_t));
    memcpy(&ibeacon_adv_data->ibeacon_vendor, vendor_config, sizeof(esp_ble_ibeacon_vendor_t));

    return ESP_OK;
}

#ifdef BLE_MODE_BEACON
static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min        = ESP_BLE_GAP_ADV_ITVL_MS(100),
    .adv_int_max        = ESP_BLE_GAP_ADV_ITVL_MS(100),
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
#elif BLE_MODE_PERIPHERAL
static esp_ble_adv_params_t ble_adv_params = {
        .adv_int_min        = ESP_BLE_GAP_ADV_ITVL_MS(20),
        .adv_int_max        = ESP_BLE_GAP_ADV_ITVL_MS(40),
        .adv_type           = ADV_TYPE_IND,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
#elif BLE_MODE_SCANNER
    static esp_ble_scan_params_t ble_scan_params = {
        .scan_type              = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval          = ESP_BLE_GAP_SCAN_ITVL_MS(50),
        .scan_window            = ESP_BLE_GAP_SCAN_WIN_MS(30),
        .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
    };
#endif

static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param){
    esp_err_t err;
        
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        #ifdef BLE_MODE_PERIPHERAL
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&ble_adv_params);
            }
        #endif
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        #ifdef BLE_MODE_PERIPHERAL
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&ble_adv_params);
            }
        #endif
            break;
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        #ifdef BLE_MODE_BEACON
            adv_config_done &= (~ADV_CONFIG_FLAG); {
                esp_ble_gap_start_advertising(&ble_adv_params);
            }
        #endif
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        #ifdef BLE_MODE_SCANNER
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&ble_adv_params);
            }
        #endif
            break;
        
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            //adv start complete event to indicate adv start successfully or failed
            if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed, error %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "Advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(TAG, "Advertising stop failed, error %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(TAG, "Advertising stop successfully");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        #ifdef BLE_MODE_PERIPHERAL
        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            ESP_LOGI(TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
            //iOS and Unix Clients can bypass authentication and keep connected even with a wrong key
            //Thus, disconnection and unbonding must be forced upon authentication failure !!. 
            if(!param->ble_security.auth_cmpl.success) {
                esp_bd_addr_t bd_addr;
                memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
                ESP_LOGI(TAG, "Failed to authenticate client - reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
                esp_ble_gap_disconnect(bd_addr);
                esp_ble_remove_bond_device(bd_addr);
                ESP_LOGI(TAG, "Client %08x%04x unbonded",\
                    (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                    (bd_addr[4] << 8) + bd_addr[5]);
            } else {
                ESP_LOGI(TAG, "Auth mode = 0x%02X", param->ble_security.auth_cmpl.auth_mode);
            }
            break;
        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
            ESP_LOGD(TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d", param->remove_bond_dev_cmpl.status);
            ESP_LOGI(TAG, "-----ESP_GAP_BLE_REMOVE_BOND_DEV----");
            ESP_LOG_BUFFER_HEX(TAG, (void *)param->remove_bond_dev_cmpl.bd_addr, sizeof(esp_bd_addr_t));
            break;
        case ESP_GAP_BLE_SET_LOCAL_PRIVACY_COMPLETE_EVT:
            if (param->local_privacy_cmpl.status != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(TAG, "config local privacy failed, error status = %x", param->local_privacy_cmpl.status);
                break;
            }

            break;
        case ESP_GAP_BLE_SEC_REQ_EVT:
            // Received security request from peer
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;
        #endif
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: 
        #ifdef BLE_MODE_SCANNER
            // the unit of the duration is second, 0 means scan permanently
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            #endif
        break;
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        #ifdef BLE_MODE_SCANNER
            //scan start complete event to indicate scan start successfully or failed
            if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Scanning start failed, error %s", esp_err_to_name(err));
            } else {
                ESP_LOGI(TAG, "Scanning start successfully");
            }
        #endif
            break;
        
        #ifdef BLE_MODE_SCANNER
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
            switch (scan_result->scan_rst.search_evt) {
            case ESP_GAP_SEARCH_INQ_RES_EVT:
                /* Search for BLE iBeacon Packet */
                /*
                if (esp_ble_is_ibeacon_packet(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len)){
                    esp_ble_ibeacon_t *ibeacon_data = (esp_ble_ibeacon_t*)(scan_result->scan_rst.ble_adv);
                    ESP_LOGI(TAG, "----------iBeacon Found----------");
                    ESP_LOGI(TAG, "Device address: "ESP_BD_ADDR_STR"", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda));
                    //#ESP_LOG_BUFFER_HEX(TAG, "Proximity UUID", ibeacon_data->ibeacon_vendor.proximity_uuid, BLE_BASE_UUID);

                    uint16_t major = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.major);
                    uint16_t minor = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.minor);
                    ESP_LOGI(TAG, "Major: 0x%04x (%d)", major, major);
                    ESP_LOGI(TAG, "Minor: 0x%04x (%d)", minor, minor);
                    ESP_LOGI(TAG, "Measured power (RSSI at a 1m distance): %d dBm", ibeacon_data->ibeacon_vendor.measured_power);
                    ESP_LOGI(TAG, "RSSI of packet: %d dbm", scan_result->scan_rst.rssi);
                }*/
                break;
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
            if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
                ESP_LOGE(TAG, "Scanning stop failed, error %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(TAG, "Scanning stop successfully");
            }
            break;
        
        #endif
        default:
            break;
    }
}


/* ------------------------ */
// GATT RELATED DEFINITIONS
/* ------------------------ */

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = false,
    .include_txpower = false,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0100, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x07,
    .manufacturer_len = 0, 
    .p_manufacturer_data =  NULL, 
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(beacon_uuid),
    .p_service_uuid = beacon_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0100, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(beacon_uuid),
    .p_service_uuid = beacon_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name((char*)device_name);
            if (set_dev_name_ret){
                ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;

            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, BLE_IDX_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
       	    break;
        case ESP_GATTS_WRITE_EVT:
            
            //Requests to BLE Service

            if (!param->write.is_prep){
                ESP_LOGI(TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);

                //Requests to Backhaul Characteristic 
                if (ble_handle_table[BLE_CHAR_BH_NTF_CFG] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    //Enable notifications via BLE
                    if (descr_value == 0x0001){
                        curr_client.notify_enable = true;    
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, ble_handle_table[BLE_CHAR_BH_VAL],
                                            sizeof(ble_notify_response), (uint8_t *)ble_notify_response, true);
                       /*
                       Callback for write event option 0x0001, which is the value to enable notifications.
                       */
                       ESP_LOGI(TAG, "Enabling notifications for client with conn_id %d", param->write.conn_id);
                    }
                    else if (descr_value == 0x0000){
                        //Disable notifications via BLE
                        curr_client.notify_enable = false;
                    }
                    else{
                        ESP_LOGE(TAG, "Unknown descr value");
                    }
                }

                //Requests to BLE API Characteristic
                else if (ble_handle_table[BLE_API_WRITE] == param->write.handle ){
                    // Callback for write event to update current time
                    
                }
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                    
                }
            }else{
                break;
            }
      	    break;

        //Set the MTU, strictly needed to maximize Backhaul
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            curr_client.mtu = param->mtu.mtu;
            break;

        //To keep control of confirmations so packets are resent if needed    
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            //ble_confirm_status = true;
            //stop_ble_confirmation_timer();
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;

        //Upon connection, set client parameters: needed to manage notifications to client
        case ESP_GATTS_CONNECT_EVT:
            // Advertise as a connectable device
            esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
            ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 0x0C80;   // timeout = 400*10ms = 4000ms
            esp_ble_gap_update_conn_params(&conn_params);
            
            // Keep track of bonded devices. In the future limit the connection to one device or add the device pool 
            // in the configuration file or pull from a central server.
            
            //esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
            //esp_ble_get_bond_device_list(&dev_num, dev_list);
            /*
            int dev_num = esp_ble_get_bond_device_num();
            bool update_curr_client = false;
            
            for (int i = 0; i < dev_num; i++) {
                for (int k = 0; k < 6; k++) {
                if (param->connect.remote_bda[k] != dev_list[i].bd_addr[k]) {
                    update_curr_client = false;
                }
                else{
                    update_curr_client = true;
                }
            }
            if (update_curr_client == true){
                curr_client.connect = true;
                curr_client.conn_id = param->connect.conn_id;
                curr_client.curr_gatts_if = gatts_if;
                curr_client.mtu = param->mtu.mtu;
                }
            }
            */

            // In the future stop advertising if the connected client is not bonded
                        
            //free(dev_list);
            break;

        //Once the client is disconnected, pre BLE Backhaul state must set and Ceryx restarts advertising
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            // Check if we need to restart advertising
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(TAG, "Create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != BLE_IDX_NB){
                ESP_LOGE(TAG, "Failed to create attribute table, num_handle (%d) doesn't equal to BLE_IDX_NB(%d)", param->add_attr_tab.num_handle, BLE_IDX_NB);
            }
            else {
                ESP_LOGI(TAG, "Create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(ble_handle_table, param->add_attr_tab.handles, sizeof(ble_handle_table));
                esp_ble_gatts_start_service(ble_handle_table[BLE_GATT_SVC]);
            }
            break;
        }

        default:
            break;
    }
}

//One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
/*
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t notify_char_handle;
    esp_bd_addr_t remote_bda;
};
*/

struct gatts_profile_inst ble_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            ble_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(TAG, "Regigster app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == ble_profile_tab[idx].gatts_if) {
                if (ble_profile_tab[idx].gatts_cb) {
                    ble_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}


int set_gattc(void){
    esp_ble_gatts_register_callback(gatts_event_handler);

    esp_err_t ret = esp_ble_gatts_app_register(ESP_APP_ID);
        if (ret){
            return -1;
        }

    ret = esp_ble_gatt_set_local_mtu(GATTS_DEMO_CHAR_VAL_LEN_MAX);
    if (ret){
        return -2;
    }

    return 0;
}

void bond_client(void){
    //Set the security iocap & auth_req & key size & init key response key parameters to the stack

    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND; // Bond with peer after auth
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT; //So the wave asks for the code
    uint8_t key_size = 16; //Maximum possible key size
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    // In the future get passkey from config file or central server. For now we set a fixed passkey
    uint32_t passkey = 123456; // Fixed passkey for now
    
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;  // As we stick to BLE to authenticate
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

/* ------------------------- */
/* BLE CONTROLLER INITIALIZATION */
/* ------------------------- */


int ble_init(uint8_t *device_eui){
    ble_stack_initialize(device_eui);

    ESP_LOGI(TAG, "Initializing in mode %d", BLE_MODE);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t ret  = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", esp_err_to_name(ret));
        return -1;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s", esp_err_to_name(ret));
        return -1;
    }

    esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s", esp_err_to_name(ret));
        return -1;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s", esp_err_to_name(ret));
        return -1;
    }


    ESP_LOGI(TAG, "register callback");

    //register the scan callback function to the gap module
    ret = esp_ble_gap_register_callback(esp_gap_cb);
    if (ret) {
        ESP_LOGE(TAG, "gap register error: %s", esp_err_to_name(ret));
        return -1;
    }

    #ifdef BLE_MODE_PERIPHERAL
        
    int err = set_gattc();
    switch (err) {
        case -1:
            ESP_LOGE(TAG, "GATTS app register error, error code = %x", ret);
            break;
        case -2:
            ESP_LOGE(TAG, "Set local  MTU failed, error code = %x", ret);
            break;
        default:
            break;  
    }

    bond_client();
    /*esp_event_loop_args_t ble_api_loop_args = {
        .queue_size = 5,
        .task_name = "ceryx_ble_api_response_task",
        .task_priority = tskIDLE_PRIORITY,
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };*/
        // In the future, create a handler for the BLE API response task
        //ESP_ERROR_CHECK(esp_event_loop_create(&ble_api_loop_args, &ble_api_loop_handle));
        
        //Register the event handler
        //esp_event_handler_register_with(ble_api_loop_handle, BLE_API_EVENT_BASE, BLE_API_RESPONSE, ble_api_response_handler, NULL);

        // Start the BLE API response task and register the event handler for the advertise switch event
        //esp_event_handler_register_with(ble_api_loop_handle, BLE_API_EVENT_BASE, BLE_ADV_SWITCH, ble_advertise_handler, NULL);
    

    // Set device advertising UUID
    

    #endif

    #ifdef BLE_MODE_BEACON

    esp_ble_ibeacon_t ibeacon_adv_data;
    esp_ble_ibeacon_vendor_t vendor_config;
    esp_err_t status = esp_ble_config_ibeacon_data (&vendor_config, &ibeacon_adv_data);
    if (status == ESP_OK){
        esp_ble_gap_config_adv_data_raw((uint8_t*)&ibeacon_adv_data, sizeof(ibeacon_adv_data));
    }
    else {
        ESP_LOGE(TAG, "Config iBeacon data failed: %s", esp_err_to_name(status));
    }
    #endif

    return 0;
}
