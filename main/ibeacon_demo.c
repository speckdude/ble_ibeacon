/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



/****************************************************************************
*
* This file is for iBeacon demo. It supports both iBeacon sender and receiver
* which is distinguished by macros IBEACON_SENDER and IBEACON_RECEIVER,
*
* iBeacon is a trademark of Apple Inc. Before building devices which use iBeacon technology,
* visit https://developer.apple.com/ibeacon/ to obtain a license.
*
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_ibeacon_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include <math.h>

#define ARRAYSIZE 10
#define ROOMSIZE  5//size in meters of room

typedef struct {
    int raw[10];
    int count;
    int transmittedRSSI;
    double lastDistance; 
}storedData;

static const char* DEMO_TAG = "IBEACON_DEMO";
extern esp_ble_ibeacon_vendor_t vendor_config;

///Declare static functions
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

void average(void);
double getDistance(int averageRSSI, int measuredRSSI);
int foundBefore(int x);
int addtoFound(int x);
double getAverage(int x[]);
void findLocation();
void zeroLocation();
void printLocation();

int sumPower, numSignals;
double averagePower, n;
int P;

int foundArray[ARRAYSIZE];
int foundCount;
double finalArray[ARRAYSIZE];

storedData rawData[ARRAYSIZE];

uint8_t possibleLocations[ROOMSIZE*4][ROOMSIZE*4];  //give quarter meter precision


#if (IBEACON_MODE == IBEACON_RECEIVER)
static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

#elif (IBEACON_MODE == IBEACON_SENDER)
static esp_ble_adv_params_t ble_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
#endif


static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{   

    esp_err_t err;

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:{
#if (IBEACON_MODE == IBEACON_SENDER)
        esp_ble_gap_start_advertising(&ble_adv_params);
#endif
        break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
#if (IBEACON_MODE == IBEACON_RECEIVER)
        //the unit of the duration is second, 0 means scan permanently
        uint32_t duration = 0;
        esp_ble_gap_start_scanning(duration);
#endif
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if ((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(DEMO_TAG, "Scan start failed: %s", esp_err_to_name(err));
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //adv start complete event to indicate adv start successfully or failed
        if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(DEMO_TAG, "Adv start failed: %s", esp_err_to_name(err));
        }
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            /* Search for BLE iBeacon Packet */
            if (esp_ble_is_ibeacon_packet(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len)){
                
                
                esp_ble_ibeacon_t *ibeacon_data = (esp_ble_ibeacon_t*)(scan_result->scan_rst.ble_adv);
                //ESP_LOGI(DEMO_TAG, "----------iBeacon Found----------");
                
                // esp_log_buffer_hex("IBEACON_DEMO: Device address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN );
                // esp_log_buffer_hex("IBEACON_DEMO: Proximity UUID:", ibeacon_data->ibeacon_vendor.proximity_uuid, ESP_UUID_LEN_128);

                // uint16_t major = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.major);
                uint16_t minor = ENDIAN_CHANGE_U16(ibeacon_data->ibeacon_vendor.minor);
                // ESP_LOGI(DEMO_TAG, "Major: 0x%04x (%d)", major, major);
                //ESP_LOGI(DEMO_TAG, "Minor: 0x%04x (%d)", minor, minor);
                // ESP_LOGI(DEMO_TAG, "Measured power (RSSI at a 1m distance):%d dbm", ibeacon_data->ibeacon_vendor.measured_power);
                // ESP_LOGI(DEMO_TAG, "RSSI of packet:%d dbm", scan_result->scan_rst.rssi);
            
                int ind = foundBefore(minor);
                if(ind == -2){
                    ESP_LOGI(DEMO_TAG, "Too many unique beacons found. No more data analysis possible");
                }
                if(ind == -1){
                    int x = addtoFound(minor);

                    rawData[x].raw[rawData[x].count] = scan_result->scan_rst.rssi;
                    //add transmitted RSSI for specific ibeacon 
                    rawData[x].transmittedRSSI = ibeacon_data->ibeacon_vendor.measured_power;
                    //increment count
                    rawData[x].count = rawData[x].count + 1;

                }
                else{
                    //add measured rssi to raw data arrray 
                    rawData[ind].raw[rawData[ind].count] = scan_result->scan_rst.rssi;
                    //increment count
                    rawData[ind].count = rawData[ind].count + 1;
                    if(rawData[ind].count == 0){
                        rawData[ind].transmittedRSSI = ibeacon_data->ibeacon_vendor.measured_power;
                    }
                    else if(rawData[ind].count == 10){
                        ESP_LOGI(DEMO_TAG, "MEASURED RSSI FROM BEACON (supplied) %d: %d \n", foundArray[ind], rawData[ind].transmittedRSSI);
                        //get average and add to final array
                        double avg = getAverage(rawData[ind].raw);
                        ESP_LOGI(DEMO_TAG, "AVG POWER FROM BEACON %d: %lf \n", foundArray[ind], avg);
                        double distance = getDistance(avg, rawData[ind].transmittedRSSI);
                        ESP_LOGI(DEMO_TAG, "DISTSNCE FROM BEACON %d: %lf \n", foundArray[ind], distance);
                        rawData[ind].lastDistance = distance;
                        //get ready for a new set of values, move back to index 0
                        rawData[ind].count = 0;
                        findLocation();
                    }
                }
                //P = ibeacon_data->ibeacon_vendor.measured_power;
                //ESP_LOGI(DEMO_TAG, "Measured power (RSSI at a 1m distance):%d dbm", ibeacon_data->ibeacon_vendor.measured_power);
                //sumPower += scan_result->scan_rst.rssi;
                //numSignals ++;
/*
                if(numSignals == 10)
                {
                    average();
                    getDistance();
                }
                */
            }
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(DEMO_TAG, "Scan stop failed: %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(DEMO_TAG, "Stop scan successfully");
        }
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(DEMO_TAG, "Adv stop failed: %s", esp_err_to_name(err));
        }
        else {
            ESP_LOGI(DEMO_TAG, "Stop adv successfully");
        }
        break;

    default:
        break;
    }
}


void ble_ibeacon_appRegister(void)
{
    esp_err_t status;

    ESP_LOGI(DEMO_TAG, "register callback");

    //register the scan callback function to the gap module
    if ((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
        ESP_LOGE(DEMO_TAG, "gap register error: %s", esp_err_to_name(status));
        return;
    }

}

void ble_ibeacon_init(void)
{
    esp_bluedroid_init();
    esp_bluedroid_enable();
    ble_ibeacon_appRegister();
}


void average(void)
{
    averagePower = (double) sumPower/ (double) numSignals;
    ESP_LOGI(DEMO_TAG, "Measured averagePower:%lf dbm", averagePower);

    sumPower = 0;
    numSignals = 0;
    //getDistance();
}

int foundBefore(int x){
    if(foundCount == ARRAYSIZE) return -2;
    int i;
    for(i = 0; i < ARRAYSIZE; i++){
        if(x == foundArray[i]) return i;
    }
    return -1;
}

int addtoFound(int x){
    foundArray[foundCount] = x; 
    foundCount++;
    return foundCount-1;
}

double getDistance(int averageRSSI, int measuredRSSI)
{
    double distance = pow(10, ((double)measuredRSSI-averageRSSI)/((double)10*n));
     //ESP_LOGI(DEMO_TAG, "estimated distance:%lf m", distance);
    return distance;
}

void initData(void){
    int i;
    for(i = 0; i < ARRAYSIZE; i++){
        rawData[i].count = 0;
        rawData[i].lastDistance = 0;
    }
}

void zeroLocation(){
    for(int i = 0; i< ROOMSIZE*4; i++)
    {
        for(int j = 0; j< ROOMSIZE*4; j++)
        {
            possibleLocations[i][j] = 0;
        }
    }
}

void findLocation(){
    zeroLocation();
    int i,j;
    int loc = foundBefore(0);
    if(loc > -1)
    {
        for(i = 0; i< rawData[loc].lastDistance*4; i++)
        {
             for(j=0; j< rawData[loc].lastDistance*4; j++)
            {
                if (j>(ROOMSIZE*4)-1) break;
                possibleLocations[i][j] = possibleLocations [i][j] + 1;
            }
        if (i>(ROOMSIZE*4)-1) break;
        }
    }
    loc = foundBefore(1);
    if(loc > -1)
    {
        for(i = 0; i< rawData[loc].lastDistance*4; i++)
        {
            for(j=0; j< rawData[loc].lastDistance*4; j++)
            {
                if (j>(ROOMSIZE*4)-1) break;
                possibleLocations[i][ROOMSIZE*4-j] = possibleLocations [i][ROOMSIZE*4-j] + 1;
            }
        if (i>(ROOMSIZE*4)-1) break;
        }
    }
    loc = foundBefore(2);
    if(loc > -1)
    {
        for(i = 0; i< rawData[loc].lastDistance*4; i++)
        {
            for(j=0; j< rawData[loc].lastDistance*4; j++)
            {
                if (j>(ROOMSIZE*4)-1) break;
                possibleLocations[ROOMSIZE*4-i][j] = possibleLocations [ROOMSIZE*4-i][j] + 1;
            }
        if (i>(ROOMSIZE*4)-1) break;
        }
    }
    loc = foundBefore(3);
    if(loc > -1)
    {
        for(i = 0; i< rawData[loc].lastDistance*4; i++)
        {
            for(j=0; j< rawData[loc].lastDistance*4; j++)
            {
                if (j>(ROOMSIZE*4)-1) break;
                possibleLocations[ROOMSIZE*4-i][ROOMSIZE*4-j] = possibleLocations [ROOMSIZE*4-i][ROOMSIZE*4-j] + 1;
            }
        if (i>(ROOMSIZE*4)-1) break;
        }
    }
    printLocation();

}

void printLocation()
{
    for(int i=0; i< ROOMSIZE*4; i++)
    {
        for(int j=0; j< ROOMSIZE*4; j++)
        {
            printf("%d ",possibleLocations[i][j] );
        }
        printf("\n");
    }
}

double getAverage(int x[]){
    int tot = 0; 
    int i;
    for(i = 0; i < 10; i++){
        tot += x[i];
    }
    double y = (double) tot;
    return (y/10.0);
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);
    initData();
    // finalArray = (int *)malloc(sizeof(double)* ARRAYSIZE);
    // foundArray = (int *)malloc(sizeof(int)*ARRAYSIZE);
    // int i;
    //
    sumPower = 0;
    numSignals = 0;
    averagePower = 0;
    foundCount = 0;

    n =2;

    ble_ibeacon_init();
    /* set scan parameters */
#if (IBEACON_MODE == IBEACON_RECEIVER)
    esp_ble_gap_set_scan_params(&ble_scan_params);

#elif (IBEACON_MODE == IBEACON_SENDER)
    esp_ble_ibeacon_t ibeacon_adv_data;
    esp_err_t status = esp_ble_config_ibeacon_data (&vendor_config, &ibeacon_adv_data);
    if (status == ESP_OK){
        esp_ble_gap_config_adv_data_raw((uint8_t*)&ibeacon_adv_data, sizeof(ibeacon_adv_data));
    }
    else {
        ESP_LOGE(DEMO_TAG, "Config iBeacon data failed: %s\n", esp_err_to_name(status));
    }
#endif
}


