/*
 * Copyright [2014] [Cisco Systems, Inc.]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     LICENSE-2.0" target="_blank" rel="nofollow">http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LEDMGR_RBUS_H_
#define _LEDMGR_RBUS_H_


#include "ansc_platform.h"
#include "ledmgr_rdkbus_common.h"

#define NUM_OF_RBUS_PARAMS                          3
#define LEDMGR_WEBCONFIG_FULLJSON_DATA              "Device.X_RDK_LedManager.FullJsonData"
#define LEDMGR_WEBCONFIG_BRIGHTNESS_DATA            "Device.X_RDK_LedManager.BrightnessData"
#define LEDMGR_WEBCONFIG_ONOFF_DATA                 "Device.X_RDK_LedManager.OnOffData"

#define LED_DATA_MEM_SIZE                           10000
#define LED_CONF_DATA_FILE                          "/data/webconfig_led_config_gb.json"  
#define LED_CONF_FULL_FILE                          "/data/led_config_full.json"  
#define LED_CONF_BACKUP_FILE                        "/data/led_config_backup.bak"
#define LED_CONF_FACTORYRESET_FILE                  "/data/led_config_factoryreset.bak"
#define LED_CONF_DATA_SCHEMA                        "/etc/rdk/schemas/led_config_full_schema.json"
#define LED_CONF_ONOFF_SCHEMA                       "/etc/rdk/schemas/led_config_onOff_schema.json"
#define LED_CONF_BRIGHTNESS_SCHEMA                  "/etc/rdk/schemas/led_config_brightness_schema.json"

typedef struct {
    CHAR* data;
}LED_DATA;

ANSC_STATUS LedMgr_Rbus_Init();
ANSC_STATUS LedMgr_Rbus_Exit();
ANSC_STATUS LedMgr_Rbus_String_EventPublish(char *dm_event, char *dm_value);
ANSC_STATUS LedMgr_updateTree();

void LedMgr_Rbus_SubscribeDML(void);
void LedMgr_Rbus_UnSubscribeDML(void);
int LedMgr_parseOnOffJson (char* buffer_onOff);
int LedMgr_parseBrightnessJson(char* buffer_brightness);
int LedMgr_validateSchema(char* json_string, char* schema_path);

#endif //_LEDMGR_RBUS_H_
