/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
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
#include <rbus.h>
#include "led_manager.h"
#include "led_manager_events.h"
#include "ledmgr_rbus_handler_apis.h"
#include "json_schema_validator_wrapper.h"
#include "led_manager_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "cJSON.h"

extern led_data_t g_led_data;
extern led_mode_data_t g_mode_data;
extern char conf_filepath[BUFLEN_128];

static LED_DATA g_LedMgr;
static rbusHandle_t rbusHandle;
char componentName[32] = "LEDMANAGER";
unsigned int gSubscribersCount = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

rbusError_t LedMgr_Rbus_GetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts);
rbusError_t LedMgr_Rbus_FullJson_SetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);
rbusError_t LedMgr_Rbus_Brightness_SetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);
rbusError_t LedMgr_Rbus_OnOff_SetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts);

/***********************************************************************
  Data Elements declaration:
 ***********************************************************************/
rbusDataElement_t ledMgrRbusDataElements[NUM_OF_RBUS_PARAMS] = {
    {LEDMGR_WEBCONFIG_FULLJSON_DATA,  RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {LedMgr_Rbus_GetHandler, LedMgr_Rbus_FullJson_SetHandler, NULL, NULL, NULL, NULL}},
    {LEDMGR_WEBCONFIG_BRIGHTNESS_DATA,  RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {LedMgr_Rbus_GetHandler, LedMgr_Rbus_Brightness_SetHandler, NULL, NULL, NULL, NULL}},
    {LEDMGR_WEBCONFIG_ONOFF_DATA,  RBUS_ELEMENT_TYPE_EVENT | RBUS_ELEMENT_TYPE_PROPERTY, {LedMgr_Rbus_GetHandler, LedMgr_Rbus_OnOff_SetHandler, NULL, NULL, NULL, NULL}},
};
rbusError_t LedMgr_Rbus_GetHandler(rbusHandle_t handle, rbusProperty_t property, rbusGetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(property);
    rbusValue_t value;
    uint32_t index = 0;
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    if(name == NULL)
    {
        CcspTraceInfo(("%s %d - Property get name is NULL\n", __FUNCTION__, __LINE__));
        return RBUS_ERROR_BUS_ERROR;
    }
    rbusValue_Init(&value);
    rbusValue_SetString(value, g_LedMgr.data);
    rbusProperty_SetValue(property, value);

    rbusValue_Release(value);
    return ret;
}

rbusError_t LedMgr_Rbus_OnOff_SetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(prop);
    rbusValue_t value = rbusProperty_GetValue(prop);
    rbusValueType_t type = rbusValue_GetType(value);
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    uint32_t index = 0;
    int strLength = 0;
    char* strParameter = 0;
    if(name == NULL)
    {
        CcspTraceInfo(("%s %d - Property get name is NULL\n", __FUNCTION__, __LINE__));
        return RBUS_ERROR_BUS_ERROR;
    }
    strParameter = rbusValue_GetString(value, &strLength);
    if (strLength > LED_DATA_MEM_SIZE) {
        CcspTraceError(("%s %d - Freeing original memory size and now allocating new memory for the required size of data\n", __FUNCTION__, __LINE__));
        free(g_LedMgr.data);
        g_LedMgr.data = (char*)malloc(strLength);
        if(g_LedMgr.data == NULL) {
            CcspTraceError(("%s %d - error allocating g_LedMgr.data\n", __FUNCTION__, __LINE__));
            return RBUS_ERROR_INVALID_INPUT;
        }
    }

    // Webconfig call
    ret = led_onOffBlobSet(strParameter);
    if(ret != RBUS_ERROR_SUCCESS) {
        CcspTraceError(("%s %d - Error connecting with webconfig\n", __FUNCTION__, __LINE__));
        return RBUS_ERROR_INVALID_INPUT;
    }

    AnscCopyString(g_LedMgr.data, strParameter);
    CcspTraceInfo(("%s %d - LedMgr_Rbus_OnOff_SetHandler() success\n", __FUNCTION__, __LINE__, ret));
    return ret;
}

rbusError_t LedMgr_Rbus_Brightness_SetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(prop);
    rbusValue_t value = rbusProperty_GetValue(prop);
    rbusValueType_t type = rbusValue_GetType(value);
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    uint32_t index = 0;
    int strLength = 0;
    char* strParameter = 0;

    if(name == NULL)
    {
        CcspTraceInfo(("%s %d - Property get name is NULL\n", __FUNCTION__, __LINE__));
        return RBUS_ERROR_BUS_ERROR;
    }
    strParameter = rbusValue_GetString(value, &strLength);
    if (strLength > LED_DATA_MEM_SIZE) {
        CcspTraceError(("%s %d - Freeing original memory size and now allocating new memory for the required size of data\n", __FUNCTION__, __LINE__));
        free(g_LedMgr.data);
        g_LedMgr.data = (char*)malloc(strLength);
        if(g_LedMgr.data == NULL) {
            CcspTraceError(("%s %d - error allocating g_LedMgr.data\n", __FUNCTION__, __LINE__));
            return RBUS_ERROR_INVALID_INPUT;
        }
    }

    // Webconfig call
    ret = led_brightnessBlobSet(strParameter);
    if(ret != RBUS_ERROR_SUCCESS) {
        CcspTraceError(("%s %d - Error connecting with webconfig\n", __FUNCTION__, __LINE__));
        return RBUS_ERROR_INVALID_INPUT;
    }

    AnscCopyString(g_LedMgr.data, strParameter);
    CcspTraceInfo(("%s %d - LedMgr_Rbus_Brightness_SetHandler() success\n", __FUNCTION__, __LINE__, ret));
    return ret;
}

rbusError_t LedMgr_Rbus_FullJson_SetHandler(rbusHandle_t handle, rbusProperty_t prop, rbusSetHandlerOptions_t* opts)
{
    char const* name = rbusProperty_GetName(prop);
    rbusValue_t value = rbusProperty_GetValue(prop);
    rbusValueType_t type = rbusValue_GetType(value);
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    uint32_t index = 0;
    int strLength = 0;
    long oldFileLength, newFileLength;
    char *origBuffer, *newBuffer;
    char* strParameter = 0;
    const cJSON *subdoc_name = NULL;
    const cJSON *json = NULL;
    if(name == NULL)
    {
        CcspTraceInfo(("%s %d - Property get name is NULL\n", __FUNCTION__, __LINE__));
        return RBUS_ERROR_BUS_ERROR;
    }
    strParameter = rbusValue_GetString(value, &strLength);
    if (strLength > LED_DATA_MEM_SIZE) {
        CcspTraceError(("%s %d - Freeing original memory size and now allocating new memory for the required size of data\n", __FUNCTION__, __LINE__));
        free(g_LedMgr.data);
        g_LedMgr.data = (char*)malloc(strLength);
        if(g_LedMgr.data == NULL) {
            CcspTraceError(("%s %d - error allocating g_LedMgr.data\n", __FUNCTION__, __LINE__));
            return RBUS_ERROR_INVALID_INPUT;
        }
    }

    // Webconfig call
    ret = led_fullJsonBlobSet(strParameter);
    if(ret != RBUS_ERROR_SUCCESS) {
        CcspTraceError(("%s %d - Error connecting with webconfig\n", __FUNCTION__, __LINE__));
        return RBUS_ERROR_INVALID_INPUT;
    }

    AnscCopyString(g_LedMgr.data, strParameter);
    CcspTraceInfo(("%s %d - LedMgr_Rbus_FullJson_SetHandler() success\n", __FUNCTION__, __LINE__, ret));
    return ret;
}
ANSC_STATUS LedMgr_getUintParamValue (char * param, UINT * value)
{
    if ((param == NULL) || (value == NULL))
    {
        CcspTraceError(("%s %d: invalid args\n", __FUNCTION__, __LINE__));
        return ANSC_STATUS_FAILURE;
    }
    if (rbus_getUint(rbusHandle, param, value) != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("%s %d: unbale to get value of param %s\n", __FUNCTION__, __LINE__, param));
        return ANSC_STATUS_FAILURE;
    }
    return ANSC_STATUS_SUCCESS;
}
int LedMgr_validateSchema(char* json_string, char* schema_path) {
    int ret;
    ret = json_validator_init(schema_path);
    if(ret != VALIDATOR_RETURN_OK) {
        CcspTraceError(("%s %d - Error opening %s schema (json_validator_init returned: %d)\n", __FUNCTION__, __LINE__, schema_path, ret));
        return RBUS_ERROR_INVALID_INPUT;
    }
    ret = json_validator_validate_request(json_string);
    if(ret != VALIDATOR_RETURN_OK)
    {
        CcspTraceError(("%s %d - Validation failed for %s schema\n", __FUNCTION__, __LINE__, schema_path));
        return RBUS_ERROR_INVALID_INPUT;
    }
    json_validator_terminate();
    CcspTraceInfo(("%s %d - LedMgr_validateSchema() success\n", __FUNCTION__, __LINE__, ret));
    return RBUS_ERROR_SUCCESS;
}
ANSC_STATUS LedMgr_updateTree() {
    char * buffer = NULL;
    cpe_event_t t_event;
    int cmd_length = BUFLEN_128;
    char cmd_buff[BUFLEN_128] = {0};
    int event_return;
    led_mode_t * mode = NULL;
    LEDMGMT_PARAMS led_data;
    led_hal_command_t * cmd = NULL;
    uint8_t i,j;
    char ** led_previous_states = NULL;
    led_t * led_data_arr = g_led_data.led_obj_head;
    led_mode_t * mode_data_arr = g_mode_data.mode_obj_head;
    //save current states name in buffers
    led_previous_states = malloc(sizeof(char *) * g_led_data.no_of_leds);
    for (i = 0; i < g_led_data.no_of_leds; i++)
    {
        led_previous_states[i] = malloc(strlen(led_data_arr[i].current_state->name) + 1);
        strncpy(led_previous_states[i], led_data_arr[i].current_state->name, strlen(led_data_arr[i].current_state->name));
        led_previous_states[i][strlen(led_data_arr[i].current_state->name)] = '\0';
        CcspTraceInfo(("%s %d: LED: %s state is currently %s.\n", __FUNCTION__, __LINE__, led_data_arr[i].name, led_previous_states[i]));
    }

    ledmgr_free_data();

    CcspTraceInfo(("%s %d: LedMgr_updateTree() started.\n", __FUNCTION__, __LINE__));

    FILE * config_file = fopen(conf_filepath, "r");
    if (config_file == NULL) {
        CcspTraceError(("%s %d: Cannot read config file (fp: config_file). \n", __FUNCTION__, __LINE__));
        for (i = 0; i < g_led_data.no_of_leds; i++)
        {
            free(led_previous_states[i]);
        }
        free(led_previous_states);
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d: updateTree config_file path: %s\n", __FUNCTION__, __LINE__, conf_filepath));

    buffer = ledmgr_read_config_file(config_file);
    if (buffer == NULL) {
        CcspTraceError(("%s %d: Cannot read config file. (buffer) \n", __FUNCTION__, __LINE__));
        for (i = 0; i < g_led_data.no_of_leds; i++)
        {
            free(led_previous_states[i]);
        }
        free(led_previous_states);
        return ANSC_STATUS_FAILURE;
    }

    CcspTraceInfo(("%s %d: updateTree buffer dump (pre-parse): %s\n", __FUNCTION__, __LINE__, buffer));

    if(ledmgr_parse_config_file(buffer) != SUCCESS) {
        CcspTraceError(("%s %d: Cannot parse config file\n", __FUNCTION__, __LINE__));
        for (i = 0; i < g_led_data.no_of_leds; i++)
        {
            free(led_previous_states[i]);
        }
        free(led_previous_states);
        return ANSC_STATUS_FAILURE;
    }
    led_data_arr = g_led_data.led_obj_head;
    mode_data_arr = g_mode_data.mode_obj_head;
    for (i = 0; i < g_led_data.no_of_leds; i++)
    {
        //iterate through all states and if previous state is found, transfer to that state
        for (j = 0; j < led_data_arr->no_of_states; j++)
        {
            if (strncmp(led_data_arr[i].states[j].name,led_previous_states[i],strlen(led_previous_states[i])) == 0)
            {
                led_data_arr[i].current_state = &led_data_arr[i].states[j];
                mode = led_data_arr[i].current_state->led_mode;
                cmd = mode->hal_command;
                memset(&led_data, 0, sizeof(LEDMGMT_PARAMS));
                led_data.led_name = led_data_arr[i].name;
                led_data.led_param = cmd->cmd;
                CcspTraceInfo(("HAL command: LED Name: %s Colour LED Param :%s \n", led_data_arr[i].name, cmd->cmd));
                CcspTraceInfo(("Setting LED...\n"));
                if (platform_hal_setLed (&led_data) != SUCCESS)
                {
                    CcspTraceError(("HAL command failed for LED Name: %s cmd: %s \n", led_data_arr[i].name, cmd->cmd));
                    break; //break from transition and check next LED
                }
                break;
            }
            continue;
        }
    }

    for (i = 0; i < g_led_data.no_of_leds; i++)
    {
        free(led_previous_states[i]);
    }
    free(led_previous_states);
    CcspTraceInfo(("%s %d: handle_event successful\n", __FUNCTION__, __LINE__));

    free(buffer);
    // pclose(cmd_out);
    CcspTraceInfo(("%s %d - LedMgr_updateTree() success\n", __FUNCTION__, __LINE__));
    return ANSC_STATUS_SUCCESS;
}
int LedMgr_parseOnOffJson (char* buffer_onOff) {
    int length_full;
    int length_bak;
    int brightness0_count = 0;
    int brightness_array_length = 0;
    FILE * fp_full;
    FILE * fp_bak;
    char * buffer_full;
    char * buffer_full_replace;
    char * buffer_bak;
    cJSON * json_onOff = NULL;
    cJSON * json_full = NULL;
    //(dont need to free these, will be freed by freeing above)
    cJSON * onOff_ledActive = NULL;
    cJSON * led_modes = NULL;
    cJSON * led_mode = NULL;
    cJSON * command = NULL;
    cJSON * brightness = NULL;

    //open config .json file, read to buffer_full
    fp_full = fopen(conf_filepath, "r");
    if(fp_full == NULL) {
        CcspTraceError(("%s %d - Error opening %s for reading\n", __FUNCTION__, __LINE__, conf_filepath));
        return RBUS_ERROR_INVALID_INPUT;
    }
    fseek(fp_full, 0, SEEK_END);
    length_full = ftell(fp_full);
    fseek(fp_full, 0, SEEK_SET);
    buffer_full = malloc(length_full);
    if(!buffer_full) {
        CcspTraceError(("%s %d - Error allocating for buffer_full\n", __FUNCTION__, __LINE__));
        fclose(fp_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    fread(buffer_full, 1, length_full, fp_full);
    fclose(fp_full);

    //parse both buffers into json structs
    json_onOff = cJSON_ParseWithLength(buffer_onOff, strlen(buffer_onOff));
    if(json_onOff == NULL) {
        CcspTraceError(("%s %d - Error parsing onOff json data\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    json_full = cJSON_ParseWithLength(buffer_full, strlen(buffer_full));
    if(json_full == NULL) {
        CcspTraceError(("%s %d - Error parsing full json data\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        cJSON_Delete(json_onOff);
        return RBUS_ERROR_INVALID_INPUT;
    }

    //read json_onOff for LedActive property
    onOff_ledActive = cJSON_GetObjectItemCaseSensitive(json_onOff, "LedActive");
    if(onOff_ledActive == NULL) {
        CcspTraceError(("%s %d - Error getting LedActive value from onOff json\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        cJSON_Delete(json_onOff);
        cJSON_Delete(json_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    if(strcmp(onOff_ledActive->valuestring, "true") != 0 && strcmp(onOff_ledActive->valuestring, "false") != 0 ) {
        CcspTraceError(("%s %d - Error: onOff LedActive value needs to be true or false\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        cJSON_Delete(json_onOff);
        cJSON_Delete(json_full);
        return RBUS_ERROR_INVALID_INPUT;
    }

    //if LedActive == "true", all brightness = 0x00, 
    if(strcmp(onOff_ledActive->valuestring, "true") == 0) {
        led_modes = cJSON_GetObjectItemCaseSensitive(json_full, "led_modes");
        brightness_array_length = cJSON_GetArraySize(led_modes);
        cJSON_ArrayForEach(led_mode, led_modes) {
            command = cJSON_GetObjectItemCaseSensitive(led_mode, "command");
            brightness = cJSON_GetObjectItemCaseSensitive(command, "brightness");
            if(strcmp(brightness->valuestring, "0x00") == 0) {
                brightness0_count += 1;
            }
        }
        //   restore from .bak
        if(brightness_array_length == brightness0_count) {
            fp_bak = fopen(LED_CONF_BACKUP_FILE, "r");
            if(fp_bak == NULL) {
                CcspTraceError(("%s %d - Error opening %s for writing\n", __FUNCTION__, __LINE__, LED_CONF_BACKUP_FILE));
                free(buffer_full);
                cJSON_Delete(json_onOff);
                cJSON_Delete(json_full);
                return RBUS_ERROR_INVALID_INPUT;
            }
            fseek(fp_bak, 0, SEEK_END);
            length_bak = ftell(fp_bak);
            fseek(fp_bak, 0, SEEK_SET);
            buffer_bak = malloc(length_bak);
            if(!buffer_bak) {
                CcspTraceError(("%s %d - Error allocating for buffer_bak\n", __FUNCTION__, __LINE__));
                fclose(fp_bak);
                free(buffer_full);
                cJSON_Delete(json_onOff);
                cJSON_Delete(json_full);
                return RBUS_ERROR_INVALID_INPUT;
            }
            fread(buffer_bak, 1, length_bak, fp_bak);
            fclose(fp_bak);
            fp_full = fopen(conf_filepath, "w");
            if(fp_full == NULL) {
                CcspTraceError(("%s %d - Error opening %s for writing\n", __FUNCTION__, __LINE__, conf_filepath));
                free(buffer_bak);
                free(buffer_full);
                cJSON_Delete(json_onOff);
                cJSON_Delete(json_full);
                return RBUS_ERROR_INVALID_INPUT;
            }
            fwrite(buffer_bak, 1, strlen(buffer_bak), fp_full);
            fclose(fp_full);
            fp_bak = fopen(LED_CONF_BACKUP_FILE, "w");
            if(fp_bak == NULL) {
                CcspTraceError(("%s %d - Error opening %s for writing\n", __FUNCTION__, __LINE__, LED_CONF_BACKUP_FILE));
                free(buffer_bak);
                free(buffer_full);
                cJSON_Delete(json_onOff);
                cJSON_Delete(json_full);
                return RBUS_ERROR_INVALID_INPUT;
            }
            fwrite(buffer_full, 1, strlen(buffer_full), fp_bak);
            fclose(fp_bak);
            free(buffer_bak);
        }
    } 
    //if LedActive == "false" + all brightness vals not 0x00
    if(strcmp(onOff_ledActive->valuestring, "false") == 0) {
        led_modes = cJSON_GetObjectItemCaseSensitive(json_full, "led_modes");
        brightness_array_length = cJSON_GetArraySize(led_modes);
        cJSON_ArrayForEach(led_mode, led_modes) {
            command = cJSON_GetObjectItemCaseSensitive(led_mode, "command");
            brightness = cJSON_GetObjectItemCaseSensitive(command, "brightness");
            if(strcmp(brightness->valuestring, "0x00") == 0) {
                brightness0_count += 1;
            }
        }
        //   change all brightness to 0x00 in full json struct + write to file
        if(brightness_array_length != brightness0_count) {
            cJSON_ArrayForEach(led_mode, led_modes) {
                command = cJSON_GetObjectItemCaseSensitive(led_mode, "command");
                brightness = cJSON_GetObjectItemCaseSensitive(command, "brightness");
                strncpy(brightness->valuestring, "0x00", strlen("0x00"));
            }
            buffer_full_replace = malloc(length_full);
            if(!buffer_full_replace) {
                CcspTraceError(("%s %d - Error mallocing for buffer_full_replace\n", __FUNCTION__, __LINE__));
                free(buffer_full);
                cJSON_Delete(json_onOff);
                cJSON_Delete(json_full);
                return RBUS_ERROR_INVALID_INPUT;
            }
            char *print_json_full = NULL;
            print_json_full = cJSON_Print(json_full);
            strncpy(buffer_full_replace, print_json_full, strlen(print_json_full));
            free(print_json_full);
            fp_full = fopen(conf_filepath, "w");
            if(fp_full == NULL) {
                CcspTraceError(("%s %d - Error opening %s for writing\n", __FUNCTION__, __LINE__, conf_filepath));
                free(buffer_full_replace);
                free(buffer_full);
                cJSON_Delete(json_onOff);
                cJSON_Delete(json_full);
                return RBUS_ERROR_INVALID_INPUT;
            }
            fwrite(buffer_full_replace, 1, strlen(buffer_full_replace), fp_full);
            fclose(fp_full);
            fp_bak = fopen(LED_CONF_BACKUP_FILE, "w");
            if(fp_bak == NULL) {
                CcspTraceError(("%s %d - Error opening %s for writing\n", __FUNCTION__, __LINE__, LED_CONF_BACKUP_FILE));
                free(buffer_full_replace);
                free(buffer_full);
                cJSON_Delete(json_onOff);
                cJSON_Delete(json_full);
                return RBUS_ERROR_INVALID_INPUT;
            }
            fwrite(buffer_full, 1, strlen(buffer_full), fp_bak);
            fclose(fp_bak);
            free(buffer_full_replace);
        }
    }

    free(buffer_full);
    cJSON_Delete(json_onOff);
    cJSON_Delete(json_full);
    CcspTraceInfo(("%s %d - LedMgr_parseOnOffJson success\n", __FUNCTION__, __LINE__));
    return RBUS_ERROR_SUCCESS;
   
}
int LedMgr_parseBrightnessJson(char* buffer_brightness) {
    int length_brightness;
    int length_full;
    int length_hex;
    int brightness_dec = 0;
    int brightness_255_int = 0;
    FILE * fp_full;
    FILE * fp_bak;
    char * buffer_full;
    char * buffer_full_replace;
    char * brightness_hex_str;
    cJSON *json_brightness = NULL;
    cJSON *json_full = NULL;
    //(dont need to free these, will be freed by freeing above)
    cJSON *brightness_value = NULL;
    cJSON *led_modes = NULL;
    cJSON *led_mode = NULL;
    cJSON *command = NULL;
    cJSON * name = NULL;
    cJSON *brightness = NULL;
    fp_full = fopen(conf_filepath, "r");
    if(fp_full == NULL) {
        CcspTraceError(("%s %d - Error opening %s for reading\n", __FUNCTION__, __LINE__, conf_filepath));
        return RBUS_ERROR_INVALID_INPUT;
    }
    fseek(fp_full, 0, SEEK_END);
    length_full = ftell(fp_full);
    fseek(fp_full, 0, SEEK_SET);
    buffer_full = malloc(length_full);
    if(!buffer_full) {
        CcspTraceError(("%s %d - Error allocating for buffer_full\n", __FUNCTION__, __LINE__));
        fclose(fp_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    fread(buffer_full, 1, length_full, fp_full);
    fclose(fp_full);
    json_brightness = cJSON_ParseWithLength(buffer_brightness, strlen(buffer_brightness));
    if(json_brightness == NULL) {
        CcspTraceError(("%s %d - Error parsing brightness json data\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    json_full = cJSON_ParseWithLength(buffer_full, strlen(buffer_full));
    if(json_full == NULL) {
        CcspTraceError(("%s %d - Error parsing full json data\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        cJSON_Delete(json_brightness);
        return RBUS_ERROR_INVALID_INPUT;
    }
    brightness_value = cJSON_GetObjectItemCaseSensitive(json_brightness, "ledBrightness");
    if(brightness_value == NULL) {
        CcspTraceError(("%s %d - Error getting ledBrightness value from brightness json\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        cJSON_Delete(json_brightness);
        cJSON_Delete(json_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    brightness_dec = atoi(brightness_value->valuestring);
    if(brightness_dec > 100 || brightness_dec < 0) {
        CcspTraceError(("%s %d - Error: brightness is not between 0-100\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        cJSON_Delete(json_brightness);
        cJSON_Delete(json_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    brightness_255_int = (int)roundf( (float)brightness_dec * (float)2.55 );
    length_hex = strlen("0x00");
    brightness_hex_str = (char*)malloc(length_hex);
    sprintf(brightness_hex_str, "0x%02X", brightness_255_int);
    //backup files
    fp_bak = fopen(LED_CONF_BACKUP_FILE, "w");
    if(fp_bak == NULL) {
        CcspTraceError(("%s %d - Error opening %s for writing\n", __FUNCTION__, __LINE__, LED_CONF_BACKUP_FILE));
        free(buffer_full);
        free(brightness_hex_str);
        cJSON_Delete(json_brightness);
        cJSON_Delete(json_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    fwrite(buffer_full, 1, strlen(buffer_full), fp_bak);
    fclose(fp_bak);
    led_modes = cJSON_GetObjectItemCaseSensitive(json_full, "led_modes");
    cJSON_ArrayForEach(led_mode, led_modes) {
        command = cJSON_GetObjectItemCaseSensitive(led_mode, "command");
        name = cJSON_GetObjectItemCaseSensitive(led_mode, "name");
        brightness = cJSON_GetObjectItemCaseSensitive(command, "brightness");
        if(strcmp(name->valuestring, "LED_OFF") == 0) {
            strncpy(brightness->valuestring, "0x00", strlen("0x00"));
        }
        else {
            strncpy(brightness->valuestring, brightness_hex_str, strlen(brightness_hex_str));
        }
    }
    buffer_full_replace = malloc(length_full);
    if(!buffer_full_replace) {
        CcspTraceError(("%s %d - Error mallocing for buffer_full_replace\n", __FUNCTION__, __LINE__));
        free(buffer_full);
        free(brightness_hex_str);
        cJSON_Delete(json_brightness);
        cJSON_Delete(json_full);
        return RBUS_ERROR_INVALID_INPUT;
    }

    char *print_json_full = NULL;
    print_json_full = cJSON_Print(json_full);
    strncpy(buffer_full_replace, print_json_full, strlen(print_json_full));
    free(print_json_full);
    fp_full = fopen(conf_filepath, "w");
    if(fp_full == NULL) {
        CcspTraceError(("%s %d - Error opening %s for writing\n", __FUNCTION__, __LINE__, conf_filepath));
        free(buffer_full);
        free(brightness_hex_str);
        free(buffer_full_replace);
        cJSON_Delete(json_brightness);
        cJSON_Delete(json_full);
        return RBUS_ERROR_INVALID_INPUT;
    }
    fwrite(buffer_full_replace, 1, strlen(buffer_full_replace), fp_full);
    fclose(fp_full);
    free(buffer_full);
    free(buffer_full_replace);
    free(brightness_hex_str);
    cJSON_Delete(json_brightness);
    cJSON_Delete(json_full);
    CcspTraceInfo(("%s %d - LedMgr_parseBrightnessJson() success\n", __FUNCTION__, __LINE__));
    return 0;
}
static void LedMgr_Rbus_EventReceiveHandler(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription)
{
    (void)handle;
    (void)subscription;
    const char* eventName = event->name;
    rbusValue_t valBuff = rbusObject_GetValue(event->data, NULL );
    if((valBuff == NULL) || (eventName == NULL))
    {
        CcspTraceError(("%s : FAILED , value is NULL\n",__FUNCTION__));
        return;
    }
    if (strcmp(eventName, LEDMGR_WEBCONFIG_FULLJSON_DATA) == 0 ||
        strcmp(eventName, LEDMGR_WEBCONFIG_BRIGHTNESS_DATA) == 0 ||
        strcmp(eventName, LEDMGR_WEBCONFIG_ONOFF_DATA) == 0)
    {
        CcspTraceInfo(("%s %d: change in %s\n", __FUNCTION__, __LINE__, eventName));
        UINT newValue = rbusValue_GetUInt32(valBuff);
        CcspTraceInfo(("%s:%d Received [%s:%u]\n",__FUNCTION__, __LINE__,eventName, newValue));
    }
    else
    {
        CcspTraceError(("%s:%d Unexpected Event Received [%s:%s]\n",__FUNCTION__, __LINE__,eventName));
    }
}
void LedMgr_Rbus_SubscribeDML(void)
{
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    ret = rbusEvent_Subscribe(rbusHandle, LEDMGR_WEBCONFIG_FULLJSON_DATA, LedMgr_Rbus_EventReceiveHandler, NULL, 0);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("%s %d - Failed to Subscribe %s, Error=%s \n", __FUNCTION__, __LINE__, rbusError_ToString(ret), LEDMGR_WEBCONFIG_FULLJSON_DATA));
    }
    ret = rbusEvent_Subscribe(rbusHandle, LEDMGR_WEBCONFIG_BRIGHTNESS_DATA, LedMgr_Rbus_EventReceiveHandler, NULL, 0);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("%s %d - Failed to Subscribe %s, Error=%s \n", __FUNCTION__, __LINE__, rbusError_ToString(ret), LEDMGR_WEBCONFIG_BRIGHTNESS_DATA));
    }
    ret = rbusEvent_Subscribe(rbusHandle, LEDMGR_WEBCONFIG_ONOFF_DATA, LedMgr_Rbus_EventReceiveHandler, NULL, 0);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("%s %d - Failed to Subscribe %s, Error=%s \n", __FUNCTION__, __LINE__, rbusError_ToString(ret), LEDMGR_WEBCONFIG_ONOFF_DATA));
    }
    CcspTraceInfo(("LedMgr_SubscribeDML done\n"));
}
void LedMgr_Rbus_UnSubscribeDML(void)
{
    rbusError_t ret = RBUS_ERROR_SUCCESS;
    ret = rbusEvent_Unsubscribe(rbusHandle, LEDMGR_WEBCONFIG_FULLJSON_DATA);
    if(ret != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("%s %d - Failed to Subscribe %s, Error=%s \n", __FUNCTION__, __LINE__, LEDMGR_WEBCONFIG_FULLJSON_DATA, rbusError_ToString(ret)));
    }
    CcspTraceInfo(("LedMgr_UnSubscribeDML done\n"));
}
/***********************************************************************
  LedMgr_Rbus_Init(): Initialize Rbus and data elements
 ***********************************************************************/
ANSC_STATUS LedMgr_Rbus_Init()
{
    int rc = ANSC_STATUS_FAILURE;
    rc = rbus_open(&rbusHandle, componentName);
    if (rc != RBUS_ERROR_SUCCESS)
    {
        CcspTraceError(("LedMgr_Rbus_Init rbus initialization failed\n"));
        return rc;
    }
    // Register data elements
    rc = rbus_regDataElements(rbusHandle, NUM_OF_RBUS_PARAMS, ledMgrRbusDataElements);
    if (rc != RBUS_ERROR_SUCCESS)
    {
        CcspTraceWarning(("rbus register data elements failed\n"));
        rbus_close(rbusHandle);
        return rc;
    }
    g_LedMgr.data = (char*)malloc(LED_DATA_MEM_SIZE);
    if ( g_LedMgr.data == NULL )
    {
        AnscTraceError(("%s:%d:: Unable to allocate dynamic memory!\n", __FUNCTION__, __LINE__ ));
        return ANSC_STATUS_FAILURE;
    }
    strncpy(g_LedMgr.data, "None", strlen("None")+1);
    return ANSC_STATUS_SUCCESS;
}
/*******************************************************************************
  LedMgr_RbusExit(): Unreg data elements and Exit
 ********************************************************************************/
ANSC_STATUS LedMgr_RbusExit()
{
    CcspTraceInfo(("%s %d - LedMgr_RbusExit called\n", __FUNCTION__, __LINE__ ));
    rbus_unregDataElements(rbusHandle, NUM_OF_RBUS_PARAMS, ledMgrRbusDataElements);
    LedMgr_Rbus_UnSubscribeDML();
    rbus_close(rbusHandle);
    return ANSC_STATUS_SUCCESS;
}
