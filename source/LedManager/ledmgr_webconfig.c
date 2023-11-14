/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Sky
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

#include "ledmgr_webconfig.h"
#include "ledmgr_rbus_handler_apis.h"
extern char conf_filepath[128];
/**
 *  Function to calculate timeout value for executing the blob
 *
 *  @param numOfEntries Number of Entries of blob
 *
 * returns timeout value
 */
size_t webconf_ssid_timeout_handler(size_t numOfEntries)
{
#if defined(_XB6_PRODUCT_REQ_) && !defined (_XB7_PRODUCT_REQ_)
    return (numOfEntries * XB6_DEFAULT_TIMEOUT);
#else
    return (numOfEntries * SSID_DEFAULT_TIMEOUT);
#endif
}

static int bytes_contain_zero(const msgpack_object_bin *bin)
{
    size_t i;
    for (i = 0; i < bin->size; i++) {
        if (bin->ptr[i] == 0) {
            return 1;
        }
    }
    return 0;
}
/*
 *Adapted from jsonconv.c (https://github.com/msgpack/msgpack-c/) which is
 *Copyright (C) 2008-2015 FURUHASHI Sadayuki and other contributors
 *Licensed under the Boost Software License, Version 1.0
 */

/*
 * Convert msgpack format data to json string.
 * return >0: success, 0: length of buffer not enough, -1: failed
 */
size_t msgpack_object_print_jsonstr(char *buffer, size_t length, const msgpack_object o)
{
    char *aux_buffer = buffer;
    size_t aux_buffer_size = length;
    size_t ret;

    switch (o.type) {
    case MSGPACK_OBJECT_NIL:
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "null");
        break;

    case MSGPACK_OBJECT_BOOLEAN:
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, (o.via.boolean ? "true" : "false"));
        break;

    case MSGPACK_OBJECT_POSITIVE_INTEGER:
#if defined(PRIu64)
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "%" PRIu64, o.via.u64);
#else
        if (o.via.u64 > ULONG_MAX) {
            PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "%lu", ULONG_MAX);
        } else {
            PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "%lu", (unsigned long)o.via.u64);
        }
#endif
        break;

    case MSGPACK_OBJECT_NEGATIVE_INTEGER:
#if defined(PRIi64)
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "%" PRIi64, o.via.i64);
#else
        if (o.via.i64 > LONG_MAX) {
            PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "%ld", LONG_MAX);
        } else if (o.via.i64 < LONG_MIN) {
            PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "%ld", LONG_MIN);
        } else {
            PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "%ld", (signed long)o.via.i64);
        }
#endif
        break;

    case MSGPACK_OBJECT_FLOAT32:
    case MSGPACK_OBJECT_FLOAT64:
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "%f", o.via.f64);
        break;

    case MSGPACK_OBJECT_STR:
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "\"%.*s\"", (int)o.via.str.size, o.via.str.ptr);
        break;

    case MSGPACK_OBJECT_BIN:
        if (bytes_contain_zero(&o.via.bin)) {
            DEBUG("the value contains zero\n");
            return -1;
        }
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "\"%.*s\"", (int)o.via.bin.size, o.via.bin.ptr);
        break;

    case MSGPACK_OBJECT_EXT:
        DEBUG("not support type: MSGPACK_OBJECT_EXT.\n");
        return -1;

    case MSGPACK_OBJECT_ARRAY:
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "[");
        if (o.via.array.size != 0) {
            msgpack_object *p = o.via.array.ptr;
            msgpack_object *const pend = o.via.array.ptr + o.via.array.size;
            PRINT_JSONSTR_CALL(ret, msgpack_object_print_jsonstr, aux_buffer, aux_buffer_size, *p);
            ++p;
            for (; p < pend; ++p) {
                PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, ",");
                PRINT_JSONSTR_CALL(ret, msgpack_object_print_jsonstr, aux_buffer, aux_buffer_size, *p);
            }
        }
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "]");
        break;

    case MSGPACK_OBJECT_MAP:
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "{");
        if (o.via.map.size != 0) {
            msgpack_object_kv *p = o.via.map.ptr;
            msgpack_object_kv *const pend = o.via.map.ptr + o.via.map.size;

            for (; p < pend; ++p) {
                if (p->key.type != MSGPACK_OBJECT_STR) {
                    DEBUG("the key of in a map must be string.\n");
                    return -1;
                }
                if (p != o.via.map.ptr) {
                    PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, ",");
                }
                PRINT_JSONSTR_CALL(ret, msgpack_object_print_jsonstr, aux_buffer, aux_buffer_size, p->key);
                PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, ":");
                PRINT_JSONSTR_CALL(ret, msgpack_object_print_jsonstr, aux_buffer, aux_buffer_size, p->val);
            }
        }
        PRINT_JSONSTR_CALL(ret, snprintf, aux_buffer, aux_buffer_size, "}");
        break;

    default:
        DEBUG("unknown type.\n");
        return -1;
    }

    return length - aux_buffer_size;
}
#undef PRINT_JSONSTR_CALL

/**
 * Function to Parse Msg packed Led On/Off JSON Config
 * 
 * @param buf Pointer to the decoded string
 * @param len Length of the Decoded Message
 *
 *  returns 0 on success, error otherwise
 */

int led_on_off_config_set(const char *buf, size_t len, pErr execRetVal)
{
#define MAX_JSON_BUFSIZE 10240
    size_t  json_len = 0;
    msgpack_zone msg_z;
    msgpack_object msg_obj;
    msgpack_unpack_return mp_rv = 0;
    char *buffer = NULL;
    char *buffer_for_config = NULL;
    char *err = NULL;

    if (!buf || !execRetVal) {
        CcspTraceError(("%s: Empty input parameters for subdoc set\n",__FUNCTION__));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Empty subdoc", sizeof(execRetVal->ErrorMsg)-1);
        }
        return RETURN_ERR;
    }

    msgpack_zone_init(&msg_z, MAX_JSON_BUFSIZE);
    if(MSGPACK_UNPACK_SUCCESS != msgpack_unpack(buf, len, NULL, &msg_z, &msg_obj)) {
        CcspTraceError(("%s: Failed to unpack led msg blob. Error %d\n",__FUNCTION__,mp_rv));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Msg unpack failed", sizeof(execRetVal->ErrorMsg)-1);
        }
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s:Msg unpack success.\n", __FUNCTION__));

    buffer = (char*) malloc (MAX_JSON_BUFSIZE);
    if (!buffer) {
        CcspTraceError(("%s: Failed to allocate memory\n",__FUNCTION__));
        strncpy(execRetVal->ErrorMsg, "Failed to allocate memory", sizeof(execRetVal->ErrorMsg)-1);
        execRetVal->ErrorCode = VALIDATION_FALIED;
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    memset(buffer,0,MAX_JSON_BUFSIZE);
    json_len = msgpack_object_print_jsonstr(buffer, MAX_JSON_BUFSIZE, msg_obj);
    if (json_len <= 0) {
        CcspTraceError(("%s: Msgpack to json conversion failed\n",__FUNCTION__));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Msgpack to json conversion failed", sizeof(execRetVal->ErrorMsg)-1);
        }
        free(buffer);
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    buffer[json_len] = '\0';
    msgpack_zone_destroy(&msg_z);
    CcspTraceInfo(("%s:Msgpack to JSON success.\n", __FUNCTION__));

    // Need to extract the sub-object in the buffer which will be used for the config json file
    cJSON *json_container = cJSON_ParseWithLength(buffer, strlen(buffer));
    if (json_container == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            CcspTraceError(("%s %d: Error before: %s\n", __FUNCTION__, __LINE__, error_ptr));
            free(buffer);
            return RETURN_ERR;
        }
    }
    cJSON *sub_json_obj = NULL;
    sub_json_obj = cJSON_GetObjectItemCaseSensitive(json_container, ON_OFF_JSON_SUBDOC_NAME);
    if (sub_json_obj == NULL)
    {
        CcspTraceError(("%s %d: Error finding json object.\n", __FUNCTION__, __LINE__));
        cJSON_Delete(json_container);
        free(buffer);
        return RETURN_ERR;
    }
    // Save this sub-object into a variable to write to the config file
    buffer_for_config = cJSON_Print(sub_json_obj);
    if (buffer_for_config == NULL)
    {
        CcspTraceError(("%s %d: Error trying to save buffer_for_config object as a string.\n", __FUNCTION__, __LINE__));
        cJSON_Delete(json_container);
        free(buffer);
        return RETURN_ERR;
    }

    //validate against schema
    if(LedMgr_validateSchema(buffer_for_config, LED_CONF_ONOFF_SCHEMA) != RETURN_OK) {
        CcspTraceError(("%s %d - Error validating schema for led_config_onOff\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }
    //save backup and change json
    if(LedMgr_parseOnOffJson(buffer_for_config) != RETURN_OK) {
        CcspTraceError(("%s %d - Error  parsing + changing json [on/off case]\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }

    free(buffer);
    cJSON_free(buffer_for_config);
    cJSON_Delete(json_container);
    execRetVal->ErrorCode = BLOB_EXEC_SUCCESS;
    return RETURN_OK;

}

pErr led_on_off_cfg_exec_handler(void *data)
{
    pErr execRetVal = NULL;

    if (data == NULL) {
        CcspTraceError(("%s: Input Data is NULL\n",__FUNCTION__));
        return execRetVal;
    }

    led_blob_data_t *led_msg = (led_blob_data_t *) data;

    execRetVal = (pErr ) malloc (sizeof(Err));
    if (execRetVal == NULL ) {
        CcspTraceError(("%s : Failed in allocating memory for error struct\n",__FUNCTION__));
        return execRetVal;
    }
    memset(execRetVal,0,(sizeof(Err)));

    if (led_on_off_config_set((const char *)led_msg->data,led_msg->msg_size,execRetVal) == RETURN_OK) {
        CcspTraceInfo(("%s : Led on/off config set success\n",__FUNCTION__));
        if(LedMgr_updateTree() != ANSC_STATUS_SUCCESS) {
            CcspTraceError(("%s %d: LedMgr_updateTree() error\n", __FUNCTION__, __LINE__));
        }
    } else {
        CcspTraceError(("%s : Led on/off config set failed\n",__FUNCTION__));
    }

    return execRetVal;
}


int led_onOffBlobSet(void *data)
{
    char *decoded_data = NULL;
    unsigned long msg_size = 0;
    size_t offset = 0;
    msgpack_unpacked msg;
    msgpack_unpack_return mp_rv;
    msgpack_object_map *map = NULL;
    msgpack_object_kv* map_ptr  = NULL;
    led_blob_data_t *led_data = NULL;
    int i = 0;

    if (data == NULL) {
        CcspTraceError(("%s: Empty Blob Input\n",__FUNCTION__));
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s: the led_config_onOff data = %s\n",__FUNCTION__, data));

    decoded_data = (char *)AnscBase64Decode((unsigned char *)data, &msg_size);

    if (!decoded_data) {
        CcspTraceError(("%s: Failed in Decoding led manager config blob\n",__FUNCTION__));
        return RETURN_ERR;
    }

    msgpack_unpacked_init( &msg );
    /* The outermost wrapper MUST be a map. */
    mp_rv = msgpack_unpack_next( &msg, (const char*) decoded_data, msg_size+1, &offset );
    if (mp_rv != MSGPACK_UNPACK_SUCCESS) {
        CcspTraceError(("%s: Failed to unpack led manager blob. Error %d",__FUNCTION__,mp_rv));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s:Msg unpack success. Offset is %u\n", __FUNCTION__,offset));
    msgpack_object obj = msg.data;

    map = &msg.data.via.map;

    map_ptr = obj.via.map.ptr;
    if ((!map) || (!map_ptr)) {
        CcspTraceError(("Failed to get object map\n"));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }
    if (msg.data.type != MSGPACK_OBJECT_MAP) {
        CcspTraceError(("%s: Invalid msgpack type",__FUNCTION__));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }

    led_data = (led_blob_data_t *) malloc(sizeof(led_blob_data_t));
    if (led_data == NULL) {
        CcspTraceError(("%s: Led data malloc error\n",__FUNCTION__));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }

    /* Parsing Config Msg String to Led Structure */
    for (i = 0;i < (int)map->size;i++) {
        if (strncmp(map_ptr->key.via.str.ptr, "version", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                led_data->version = (uint64_t) map_ptr->val.via.u64;
                CcspTraceInfo(("Version type %d version %llu\n",map_ptr->val.type,led_data->version));
            }
        }
        else if (strncmp(map_ptr->key.via.str.ptr, "transaction_id", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                led_data->transaction_id = (uint16_t) map_ptr->val.via.u64;
                CcspTraceInfo(("Tx id type %d tx id %d\n",map_ptr->val.type,led_data->transaction_id));
            }
        }
        ++map_ptr;
    }

    msgpack_unpacked_destroy( &msg );

    led_data->msg_size = msg_size;
    led_data->data = decoded_data;

    execData *execDataPf = NULL ;
    execDataPf = (execData*) malloc (sizeof(execData));
    if (execDataPf != NULL) {
        memset(execDataPf, 0, sizeof(execData));
        execDataPf->txid = led_data->transaction_id;
        execDataPf->version = led_data->version;
        execDataPf->numOfEntries = 2;
        strncpy(execDataPf->subdoc_name, ON_OFF_JSON_SUBDOC_NAME, sizeof(execDataPf->subdoc_name)-1);
        execDataPf->user_data = (void*) led_data;
        execDataPf->calcTimeout = webconf_ssid_timeout_handler;
        execDataPf->executeBlobRequest = led_on_off_cfg_exec_handler;
        execDataPf->rollbackFunc = led_cfg_rollback_handler; // need to free and recreate tree/restart Led Manager
        execDataPf->freeResources = led_cfg_free_resources;
        PushBlobRequest(execDataPf);
        CcspTraceError(("%s %d: Decoded_data = %s\n", __FUNCTION__, __LINE__, decoded_data));
        CcspTraceInfo(("PushBlobRequest Complete\n"));
    }

    return RETURN_OK;
}

/**
 * Function to Parse Msg packed Led Brightness JSON Config
 * 
 * @param buf Pointer to the decoded string
 * @param len Length of the Decoded Message
 *
 *  returns 0 on success, error otherwise
 */

int led_brightness_config_set(const char *buf, size_t len, pErr execRetVal)
{
#define MAX_JSON_BUFSIZE 10240
    size_t  json_len = 0;
    msgpack_zone msg_z;
    msgpack_object msg_obj;
    msgpack_unpack_return mp_rv = 0;
    char *buffer = NULL;
    char *buffer_for_config = NULL;
    char *err = NULL;

    if (!buf || !execRetVal) {
        CcspTraceError(("%s: Empty input parameters for subdoc set\n",__FUNCTION__));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Empty subdoc", sizeof(execRetVal->ErrorMsg)-1);
        }
        return RETURN_ERR;
    }

    msgpack_zone_init(&msg_z, MAX_JSON_BUFSIZE);
    if(MSGPACK_UNPACK_SUCCESS != msgpack_unpack(buf, len, NULL, &msg_z, &msg_obj)) {
        CcspTraceError(("%s: Failed to unpack led msg blob. Error %d\n",__FUNCTION__,mp_rv));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Msg unpack failed", sizeof(execRetVal->ErrorMsg)-1);
        }
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s:Msg unpack success.\n", __FUNCTION__));

    buffer = (char*) malloc (MAX_JSON_BUFSIZE);
    if (!buffer) {
        CcspTraceError(("%s: Failed to allocate memory\n",__FUNCTION__));
        strncpy(execRetVal->ErrorMsg, "Failed to allocate memory", sizeof(execRetVal->ErrorMsg)-1);
        execRetVal->ErrorCode = VALIDATION_FALIED;
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    memset(buffer,0,MAX_JSON_BUFSIZE);
    json_len = msgpack_object_print_jsonstr(buffer, MAX_JSON_BUFSIZE, msg_obj);
    if (json_len <= 0) {
        CcspTraceError(("%s: Msgpack to json conversion failed\n",__FUNCTION__));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Msgpack to json conversion failed", sizeof(execRetVal->ErrorMsg)-1);
        }
        free(buffer);
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    buffer[json_len] = '\0';
    msgpack_zone_destroy(&msg_z);
    CcspTraceInfo(("%s:Msgpack to JSON success.\n", __FUNCTION__));

    // Need to extract the sub-object in the buffer which will be used for the config json file
    cJSON *json_container = cJSON_ParseWithLength(buffer, strlen(buffer));
    if (json_container == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            CcspTraceError(("%s %d: Error before: %s\n", __FUNCTION__, __LINE__, error_ptr));
            free(buffer);
            return RETURN_ERR;
        }
    }
    cJSON *sub_json_obj = NULL;
    sub_json_obj = cJSON_GetObjectItemCaseSensitive(json_container, BRIGHTNESS_JSON_SUBDOC_NAME);
    if (sub_json_obj == NULL)
    {
        CcspTraceError(("%s %d: Error finding json object.\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }
    // Save this sub-object into a variable to write to the config file
    buffer_for_config = cJSON_Print(sub_json_obj);
    if (buffer_for_config == NULL)
    {
        CcspTraceError(("%s %d: Error trying to save buffer_for_config object as a string.\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }

    //validate against schema
    if(LedMgr_validateSchema(buffer_for_config, LED_CONF_BRIGHTNESS_SCHEMA) != RETURN_OK) {
        CcspTraceError(("%s %d - Error validating schema for led_config_brightness\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }

    //save backup and change json
    if(LedMgr_parseBrightnessJson(buffer_for_config) != RETURN_OK) {
        CcspTraceError(("%s %d - Error parsing + changing json [brightness case]\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }


    free(buffer);
    cJSON_free(buffer_for_config);
    cJSON_Delete(json_container);
    execRetVal->ErrorCode = BLOB_EXEC_SUCCESS;
    return RETURN_OK;

}

pErr led_brightness_cfg_exec_handler(void *data)
{
    pErr execRetVal = NULL;

    if (data == NULL) {
        CcspTraceError(("%s: Input Data is NULL\n",__FUNCTION__));
        return execRetVal;
    }

    led_blob_data_t *led_msg = (led_blob_data_t *) data;

    execRetVal = (pErr ) malloc (sizeof(Err));
    if (execRetVal == NULL ) {
        CcspTraceError(("%s : Failed in allocating memory for error struct\n",__FUNCTION__));
        return execRetVal;
    }
    memset(execRetVal,0,(sizeof(Err)));

    if (led_brightness_config_set((const char *)led_msg->data,led_msg->msg_size,execRetVal) == RETURN_OK) {
        CcspTraceInfo(("%s : Led brightness config set success\n",__FUNCTION__));
        if(LedMgr_updateTree() != ANSC_STATUS_SUCCESS) {
            CcspTraceError(("%s %d: LedMgr_updateTree() error\n", __FUNCTION__, __LINE__));
        }
    } else {
        CcspTraceError(("%s : Led brightness config set failed\n",__FUNCTION__));
    }

    return execRetVal;
}

int led_brightnessBlobSet(void *data)
{
    char *decoded_data = NULL;
    unsigned long msg_size = 0;
    size_t offset = 0;
    msgpack_unpacked msg;
    msgpack_unpack_return mp_rv;
    msgpack_object_map *map = NULL;
    msgpack_object_kv* map_ptr  = NULL;
    led_blob_data_t *led_data = NULL;
    int i = 0;

    if (data == NULL) {
        CcspTraceError(("%s: Empty Blob Input\n",__FUNCTION__));
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s: the led_config_brightness data = %s\n",__FUNCTION__, data));

    decoded_data = (char *)AnscBase64Decode((unsigned char *)data, &msg_size);

    if (!decoded_data) {
        CcspTraceError(("%s: Failed in Decoding led manager config blob\n",__FUNCTION__));
        return RETURN_ERR;
    }

    msgpack_unpacked_init( &msg );
    /* The outermost wrapper MUST be a map. */
    mp_rv = msgpack_unpack_next( &msg, (const char*) decoded_data, msg_size+1, &offset );
    if (mp_rv != MSGPACK_UNPACK_SUCCESS) {
        CcspTraceError(("%s: Failed to unpack led manager blob. Error %d",__FUNCTION__,mp_rv));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s:Msg unpack success. Offset is %u\n", __FUNCTION__,offset));
    msgpack_object obj = msg.data;

    map = &msg.data.via.map;

    map_ptr = obj.via.map.ptr;
    if ((!map) || (!map_ptr)) {
        CcspTraceError(("Failed to get object map\n"));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }
    if (msg.data.type != MSGPACK_OBJECT_MAP) {
        CcspTraceError(("%s: Invalid msgpack type",__FUNCTION__));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }

    led_data = (led_blob_data_t *) malloc(sizeof(led_blob_data_t));
    if (led_data == NULL) {
        CcspTraceError(("%s: Led data malloc error\n",__FUNCTION__));
        free(decoded_data);
        return RETURN_ERR;
    }

    /* Parsing Config Msg String to Led Structure */
    for (i = 0;i < (int)map->size;i++) {
        if (strncmp(map_ptr->key.via.str.ptr, "version", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                led_data->version = (uint64_t) map_ptr->val.via.u64;
                CcspTraceInfo(("Version type %d version %llu\n",map_ptr->val.type,led_data->version));
            }
        }
        else if (strncmp(map_ptr->key.via.str.ptr, "transaction_id", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                led_data->transaction_id = (uint16_t) map_ptr->val.via.u64;
                CcspTraceInfo(("Tx id type %d tx id %d\n",map_ptr->val.type,led_data->transaction_id));
            }
        }
        ++map_ptr;
    }

    msgpack_unpacked_destroy( &msg );

    led_data->msg_size = msg_size;
    led_data->data = decoded_data;

    execData *execDataPf = NULL ;
    execDataPf = (execData*) malloc (sizeof(execData));
    if (execDataPf != NULL) {
        memset(execDataPf, 0, sizeof(execData));
        execDataPf->txid = led_data->transaction_id;
        execDataPf->version = led_data->version;
        execDataPf->numOfEntries = 2;
        strncpy(execDataPf->subdoc_name, BRIGHTNESS_JSON_SUBDOC_NAME, sizeof(execDataPf->subdoc_name)-1);
        execDataPf->user_data = (void*) led_data;
        execDataPf->calcTimeout = webconf_ssid_timeout_handler;
        execDataPf->executeBlobRequest = led_brightness_cfg_exec_handler;
        execDataPf->rollbackFunc = led_cfg_rollback_handler; // need to free and recreate tree/restart Led Manager
        execDataPf->freeResources = led_cfg_free_resources;
        PushBlobRequest(execDataPf);
        CcspTraceError(("%s %d: Decoded_data = %s\n", __FUNCTION__, __LINE__, decoded_data));
        CcspTraceInfo(("PushBlobRequest Complete\n"));
    }

    return RETURN_OK;
}

/**
 * Function to Parse Msg packed Led Full JSON Config
 * 
 * @param buf Pointer to the decoded string
 * @param len Length of the Decoded Message
 *
 *  returns 0 on success, error otherwise
 */

int led_full_json_config_set(const char *buf, size_t len, pErr execRetVal)
{
#define MAX_JSON_BUFSIZE 10240
    size_t  json_len = 0;
    msgpack_zone msg_z;
    msgpack_object msg_obj;
    msgpack_unpack_return mp_rv = 0;
    char *buffer = NULL;
    char *buffer_for_config = NULL;
    char *err = NULL;
    char *origBuffer, *newBuffer;
    long oldFileLength, newFileLength;

    if (!buf || !execRetVal) {
        CcspTraceError(("%s: Empty input parameters for subdoc set\n",__FUNCTION__));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Empty subdoc", sizeof(execRetVal->ErrorMsg)-1);
        }
        return RETURN_ERR;
    }

    msgpack_zone_init(&msg_z, MAX_JSON_BUFSIZE);
    if(MSGPACK_UNPACK_SUCCESS != msgpack_unpack(buf, len, NULL, &msg_z, &msg_obj)) {
        CcspTraceError(("%s: Failed to unpack led msg blob. Error %d\n",__FUNCTION__,mp_rv));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Msg unpack failed", sizeof(execRetVal->ErrorMsg)-1);
        }
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s:Msg unpack success.\n", __FUNCTION__));

    buffer = (char*) malloc (MAX_JSON_BUFSIZE);
    if (!buffer) {
        CcspTraceError(("%s: Failed to allocate memory\n",__FUNCTION__));
        strncpy(execRetVal->ErrorMsg, "Failed to allocate memory", sizeof(execRetVal->ErrorMsg)-1);
        execRetVal->ErrorCode = VALIDATION_FALIED;
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    memset(buffer,0,MAX_JSON_BUFSIZE);
    json_len = msgpack_object_print_jsonstr(buffer, MAX_JSON_BUFSIZE, msg_obj);
    if (json_len <= 0) {
        CcspTraceError(("%s: Msgpack to json conversion failed\n",__FUNCTION__));
        if (execRetVal) {
            execRetVal->ErrorCode = VALIDATION_FALIED;
            strncpy(execRetVal->ErrorMsg, "Msgpack to json conversion failed", sizeof(execRetVal->ErrorMsg)-1);
        }
        free(buffer);
        msgpack_zone_destroy(&msg_z);
        return RETURN_ERR;
    }

    buffer[json_len] = '\0';
    msgpack_zone_destroy(&msg_z);
    CcspTraceInfo(("%s:Msgpack to JSON success.\n", __FUNCTION__));

    // Need to extract the sub-object in the buffer which will be used for the config json file
    cJSON *json_container = cJSON_ParseWithLength(buffer, strlen(buffer));
    if (json_container == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            CcspTraceError(("%s %d: Error before: %s\n", __FUNCTION__, __LINE__, error_ptr));
            free(buffer);
            return RETURN_ERR;
        }
    }
    cJSON *sub_json_obj = NULL;
    sub_json_obj = cJSON_GetObjectItemCaseSensitive(json_container, FULL_JSON_SUBDOC_NAME);
    if (sub_json_obj == NULL)
    {
        CcspTraceError(("%s %d: Error finding json object.\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }
    // Save this sub-object into a variable to write to the config file
    buffer_for_config = cJSON_Print(sub_json_obj);
    if (buffer_for_config == NULL)
    {
        CcspTraceError(("%s %d: Error trying to save buffer_for_config object as a string.\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }

    // Validate buffer against schema
    if (LedMgr_validateSchema(buffer_for_config, LED_CONF_DATA_SCHEMA) != RETURN_OK) {
        CcspTraceError(("%s: Failed to fetch and validate led full config json. ErrorMsg: %s\n", __FUNCTION__,execRetVal->ErrorMsg));
        execRetVal->ErrorCode = VALIDATION_FALIED;
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s: Successfully validated with the JSON schema!\n", __FUNCTION__));

    //open runtime LED config file, read file to buffer
    FILE * fp = fopen(conf_filepath, "r");
    if(fp == NULL) {
        CcspTraceError(("%s %d - Unable to open original LED config file in the backup process\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }
    fseek(fp, 0, SEEK_END);
    oldFileLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    origBuffer = malloc(oldFileLength);
    if(!origBuffer) {
        CcspTraceError(("%s %d - Unable to allocate memory for buffer to read config file\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        fclose(fp);
        return RETURN_ERR;
    }
    fread(origBuffer, 1, oldFileLength, fp);
    fclose(fp);

    // Create a file with write access to backup original LED config file
    FILE * fp_backup = fopen(LED_CONF_BACKUP_FILE, "w");
    if(fp_backup == NULL) {
        CcspTraceError(("%s %d - Unable to create a backup file for the original LED config\n", __FUNCTION__, __LINE__));
        free(origBuffer);
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }
    fwrite(origBuffer, 1, oldFileLength, fp_backup);
    fclose(fp_backup);
    free(origBuffer);
    CcspTraceInfo(("%s %d - Contents of original LED config file have been backed up successfully!\n", __FUNCTION__, __LINE__));
    CcspTraceInfo(("%s %d - Buffer: %s\n", __FUNCTION__, __LINE__, buffer));
    
    // Replace json with the new json object buffer_to_config
    FILE * fp_new_json = fopen(conf_filepath, "w");
    if (fp_new_json == NULL)
    {
        CcspTraceError(("%s %d - Unable to open a new led config json file!\n", __FUNCTION__, __LINE__));
        free(buffer);
        cJSON_free(buffer_for_config);
        cJSON_Delete(json_container);
        return RETURN_ERR;
    }
    
    fwrite(buffer_for_config, 1, strlen(buffer_for_config)+1, fp_new_json);
    fclose(fp_new_json);
    CcspTraceInfo(("%s %d - Successfully replaced led config json file!\n", __FUNCTION__, __LINE__));

    free(buffer);
    cJSON_free(buffer_for_config);
    cJSON_Delete(json_container);
    execRetVal->ErrorCode = BLOB_EXEC_SUCCESS;
    return RETURN_OK;

}

/**
 * Function to carry out parsing of full JSON data
 *
 * @param data Pointer to the data pulled from cloud
 *
 * returns error code
 */
pErr led_full_json_cfg_exec_handler(void *data)
{
    pErr execRetVal = NULL;

    if (data == NULL) {
        CcspTraceError(("%s: Input Data is NULL\n",__FUNCTION__));
        return execRetVal;
    }

    led_blob_data_t *led_msg = (led_blob_data_t *) data;

    execRetVal = (pErr ) malloc (sizeof(Err));
    if (execRetVal == NULL ) {
        CcspTraceError(("%s : Failed in allocating memory for error struct\n",__FUNCTION__));
        return execRetVal;
    }
    memset(execRetVal,0,(sizeof(Err)));

    if (led_full_json_config_set((const char *)led_msg->data,led_msg->msg_size,execRetVal) == RETURN_OK) {
        CcspTraceInfo(("%s : Led full json config set success\n",__FUNCTION__));
        if(LedMgr_updateTree() != ANSC_STATUS_SUCCESS) {
            CcspTraceError(("%s %d: LedMgr_updateTree() error\n", __FUNCTION__, __LINE__));
        }
    } else {
        CcspTraceError(("%s : Led full json config set failed\n",__FUNCTION__));
    }

    return execRetVal;
}

/**
 * API to get full JSON blob from the cloud
 *
 * @param data Pointer to the data pulled from cloud
 *
 * returns 0 number if successful, -1 otherwise
 */

int led_fullJsonBlobSet(void *data)
{
    char *decoded_data = NULL;
    unsigned long msg_size = 0;
    size_t offset = 0;
    msgpack_unpacked msg;
    msgpack_unpack_return mp_rv;
    msgpack_object_map *map = NULL;
    msgpack_object_kv* map_ptr  = NULL;
    led_blob_data_t *led_data = NULL;
    int i = 0;

    if (data == NULL) {
        CcspTraceError(("%s: Empty Blob Input\n",__FUNCTION__));
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s: the led_config_full data = %s\n",__FUNCTION__, data));

    decoded_data = (char *)AnscBase64Decode((unsigned char *)data, &msg_size);

    if (!decoded_data) {
        CcspTraceError(("%s: Failed in Decoding led manager config blob\n",__FUNCTION__));
        return RETURN_ERR;
    }

    msgpack_unpacked_init( &msg );
    /* The outermost wrapper MUST be a map. */
    mp_rv = msgpack_unpack_next( &msg, (const char*) decoded_data, msg_size+1, &offset );
    if (mp_rv != MSGPACK_UNPACK_SUCCESS) {
        CcspTraceError(("%s: Failed to unpack led manager blob. Error %d",__FUNCTION__,mp_rv));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }

    CcspTraceInfo(("%s:Msg unpack success. Offset is %u\n", __FUNCTION__,offset));
    msgpack_object obj = msg.data;

    map = &msg.data.via.map;

    map_ptr = obj.via.map.ptr;
    if ((!map) || (!map_ptr)) {
        CcspTraceError(("Failed to get object map\n"));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }
    if (msg.data.type != MSGPACK_OBJECT_MAP) {
        CcspTraceError(("%s: Invalid msgpack type",__FUNCTION__));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }

    led_data = (led_blob_data_t *) malloc(sizeof(led_blob_data_t));
    if (led_data == NULL) {
        CcspTraceError(("%s: Led data malloc error\n",__FUNCTION__));
        msgpack_unpacked_destroy( &msg );
        free(decoded_data);
        return RETURN_ERR;
    }

    /* Parsing Config Msg String to Led Structure */
    for (i = 0;i < (int)map->size;i++) {
        if (strncmp(map_ptr->key.via.str.ptr, "version", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                led_data->version = (uint64_t) map_ptr->val.via.u64;
                CcspTraceInfo(("Version type %d version %llu\n",map_ptr->val.type,led_data->version));
            }
        }
        else if (strncmp(map_ptr->key.via.str.ptr, "transaction_id", map_ptr->key.via.str.size) == 0) {
            if (map_ptr->val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                led_data->transaction_id = (uint16_t) map_ptr->val.via.u64;
                CcspTraceInfo(("Tx id type %d tx id %d\n",map_ptr->val.type,led_data->transaction_id));
            }
        }
        ++map_ptr;
    }

    msgpack_unpacked_destroy( &msg );

    led_data->msg_size = msg_size;
    led_data->data = decoded_data;

    execData *execDataPf = NULL ;
    execDataPf = (execData*) malloc (sizeof(execData));
    if (execDataPf != NULL) {
        memset(execDataPf, 0, sizeof(execData));
        execDataPf->txid = led_data->transaction_id;
        execDataPf->version = led_data->version;
        execDataPf->numOfEntries = 2;
        strncpy(execDataPf->subdoc_name, FULL_JSON_SUBDOC_NAME, sizeof(execDataPf->subdoc_name)-1);
        execDataPf->user_data = (void*) led_data;
        execDataPf->calcTimeout = webconf_ssid_timeout_handler;
        execDataPf->executeBlobRequest = led_full_json_cfg_exec_handler;
        execDataPf->rollbackFunc = led_cfg_rollback_handler; // need to free and recreate tree/restart Led Manager
        execDataPf->freeResources = led_cfg_free_resources;
        PushBlobRequest(execDataPf);
        CcspTraceError(("%s %d: Decoded_data = %s\n", __FUNCTION__, __LINE__, decoded_data));
        CcspTraceInfo(("PushBlobRequest Complete\n"));
    }

    return RETURN_OK;
}

/**
 * Function is only called if webconfig does not correctly parse the data 
 * and aims to roll back to the previous stable state
 *
 * returns 0 number if successful, error otherwise
 */
int led_cfg_rollback_handler()
{
    // Restart LED Manager?
    // If there is an error, the app needs to "roll back" to a stable state
    // Copy the backup file and do a Ccsp trace print of being unable to 
    // apply led config from data collected by webconfig

    CcspTraceInfo(("%s: Rollback applied successfully\n",__FUNCTION__));
    return RETURN_OK;
}

/**
 * Function which frees all the necessary dynamically-allocated memory for webconfig
 *
 * @param arg Pointer to the resource
 *
 * non-return
 */
void led_cfg_free_resources(void *arg)
{

    CcspTraceInfo(("Entering: %s\n",__FUNCTION__));
    if (arg == NULL) {
        CcspTraceError(("%s: Input Data is NULL\n",__FUNCTION__));
        return;
    }

    execData *blob_exec_data  = (execData*) arg;

    led_blob_data_t *led_Data = (led_blob_data_t *) blob_exec_data->user_data;
 
    if (led_Data && led_Data->data) {
        free(led_Data->data);
        led_Data->data = NULL;
    }

    if (led_Data) {
        free(led_Data);
        led_Data = NULL;
    }

    free(blob_exec_data);
    blob_exec_data = NULL;
    CcspTraceInfo(("%s:Success in Clearing led config resources\n",__FUNCTION__));
}

/**
 * API to get Blob version from PSM db
 *
 * @param subdoc Pointer to name of the subdoc
 *
 * returns version number if present, 0 otherwise
 */
uint32_t getLedMgrBlobVersion(char* subdoc)
{
    char *subdoc_ver = NULL;
    char  buf[72] = {0};
    int retval;
    uint32_t version = 0;

    snprintf(buf,sizeof(buf), LedMgrSsidVersion, subdoc);

    retval = PSM_Get_Record_Value2(bus_handle,g_Subsystem, buf, NULL, &subdoc_ver);
    if ((retval == CCSP_SUCCESS) && (subdoc_ver))
    {
        version = strtoul(subdoc_ver, NULL, 10);
        CcspTraceInfo(("%s: Led Manager %s blob version %s\n",__FUNCTION__, subdoc,subdoc_ver));
        ((CCSP_MESSAGE_BUS_INFO *)bus_handle)->freefunc(subdoc_ver);
        return (uint32_t)version;
    }
    return 0;
}

/**
 * API to set Blob version in PSM db
 *
 * @param subdoc  Pointer to name of the subdoc
 * @param version Version of the blob
 *
 * returns 0 on success, error otherwise
 */
int setLedMgrBlobVersion(char* subdoc,uint32_t version)
{
    char subdoc_ver[32] = {0}, buf[72] = {0};
    int retval;

    snprintf(subdoc_ver,sizeof(subdoc_ver),"%u",version);
    snprintf(buf,sizeof(buf), LedMgrSsidVersion,subdoc);

    retval = PSM_Set_Record_Value2(bus_handle,g_Subsystem, buf, ccsp_string, subdoc_ver);
    if (retval == CCSP_SUCCESS) {
        CcspTraceInfo(("%s: Led Manager Blob version applied to PSM DB Successfully\n", __FUNCTION__));
        return RETURN_OK;
    } else {
        CcspTraceError(("%s: Failed to apply blob version to PSM DB\n", __FUNCTION__));
        return RETURN_ERR;
    }
}

/**
  *  API to register all the supported subdocs , versionGet 
  *  and versionSet are callback functions to get and set 
  *  the subdoc versions in db 
  *
  *  returns 0 on success, error otherwise
  */
int web_config_init()
{
    char *sub_docs[SUBDOC_COUNT+1]= {FULL_JSON_SUBDOC_NAME,BRIGHTNESS_JSON_SUBDOC_NAME,ON_OFF_JSON_SUBDOC_NAME,(char *) 0 };
    static blobRegInfo *blobData = NULL;
    blobRegInfo *blobDataPointer = NULL;
    int i;

    if(blobData == NULL) {
        blobData = (blobRegInfo*) malloc(SUBDOC_COUNT * sizeof(blobRegInfo));
    }
    if (blobData == NULL) {
        CcspTraceError(("%s: Malloc error\n",__FUNCTION__));
        return RETURN_ERR;
    }
    memset(blobData, 0, SUBDOC_COUNT * sizeof(blobRegInfo));

    blobDataPointer = blobData;
    for (i=0 ;i < SUBDOC_COUNT; i++)
    {
        strncpy(blobDataPointer->subdoc_name, sub_docs[i], sizeof(blobDataPointer->subdoc_name)-1);
        blobDataPointer++;
    }
    blobDataPointer = blobData;
    
    getVersion versionGet = getLedMgrBlobVersion;
    setVersion versionSet = setLedMgrBlobVersion;
    CcspTraceInfo(("%s %d: Subdoc name = %s, subdoc version = %d\n", __FUNCTION__, __LINE__, blobData->subdoc_name, blobData->version));
    register_sub_docs(blobData,SUBDOC_COUNT,versionGet,versionSet);

    return RETURN_OK;
}
