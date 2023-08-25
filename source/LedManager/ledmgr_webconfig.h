 /*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/


#ifndef  _LED_MANAGER_WEBCONFIG_H
#define  _LED_MANAGER_WEBCONFIG_H

/* ---- Include Files ---------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <pthread.h>
#include "ledmgr_rbus_handler_apis.h"
#include "led_manager_global.h"

#include "webconfig_framework.h"
#include "platform_hal.h"
#include "ccsp_base_api.h"
#include "ccsp_psm_helper.h"
#include "ccsp_trace.h"
#include "msgpack.h"
#include "cJSON.h"
#include "dslh_dmagnt_interface.h"



/* ---- Macros ---------------------------------------- */
#define LedMgrSsidVersion               "eRT.com.cisco.spvtg.ccsp.Device.LedManager.%s_version"

#define SUBDOC_COUNT                    3
#define FULL_JSON_SUBDOC_NAME           "ledStatesConfigFull"
#define BRIGHTNESS_JSON_SUBDOC_NAME     "ledStatesConfigBrightness"
#define ON_OFF_JSON_SUBDOC_NAME         "ledStatesConfigOnOff"

#define SSID_DEFAULT_TIMEOUT            90
#define XB6_DEFAULT_TIMEOUT             15

#define PRINT_JSONSTR_CALL(ret, func, aux_buffer, aux_buffer_size, ...) \
    ret = func(aux_buffer, aux_buffer_size, __VA_ARGS__);               \
    if (ret <= 0)                                                       \
        return ret;                                                     \
    if (ret > aux_buffer_size)                                          \
        return 0;                                                       \
    aux_buffer = aux_buffer + ret;                                      \
    aux_buffer_size = aux_buffer_size - ret

#define DEBUG(...) printf(__VA_ARGS__)

/* ---- Structs ---------------------------------------- */
typedef struct
{
    void     *data;
    char      subdoc_name[32];
    uint64_t  version;
    uint16_t  transaction_id;
    unsigned  long msg_size;
} led_blob_data_t;

/* ---- Global Variables ---------------------------------------- */
extern ANSC_HANDLE bus_handle;
extern char g_Subsystem[32];

/* ---- Function Declarations ---------------------------------------- */

size_t webconf_ssid_timeout_handler(size_t numOfEntries);

int led_on_off_config_set(const char *buf, size_t len, pErr execRetVal);
pErr led_on_off_cfg_exec_handler(void *data);
int led_onOffBlobSet(void *data);

int led_brightness_config_set(const char *buf, size_t len, pErr execRetVal);
pErr led_brightness_cfg_exec_handler(void *data);
int led_brightnessBlobSet(void *data);

int led_full_json_config_set(const char *buf, size_t len, pErr execRetVal);
pErr led_full_json_cfg_exec_handler(void *data);
int led_fullJsonBlobSet(void *data);

int led_cfg_rollback_handler();
void led_cfg_free_resources(void *arg);

uint32_t getLedMgrBlobVersion(char* subdoc);
int setLedMgrBlobVersion(char* subdoc,uint32_t version);
int web_config_init();

#endif /* _LED_MANAGER_WEBCONFIG_H */
