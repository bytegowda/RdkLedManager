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

#include "led_manager.h"
#include "led_manager_events.h"
#include "led_manager_global.h"
#include "ccsp_psm_helper.h"

#ifdef LEDMGR_WEBCONFIG
#include <cjson/cJSON.h>
#include <sys/stat.h>

#define COLOURS_DEFINITION_JSON_KEY  "colours_definition"
#define BUFFER_LENGTH                256
#endif

#define LED_MODES_JSON_KEY           "led_modes"
#define LED_MODE_JSON_KEY            "led_mode"
#define LED_JSON_KEY                 "leds"
#define STATES_JSON_KEY              "states"
#define NAME_JSON_KEY                "name"
#define MODE_CMD_JSON_KEY            "command"
#define TRANSITIONS_JSON_KEY         "transitions"
#define EVENT_JSON_KEY               "event"
#define NEXT_STATE_JSON_KEY          "next_state"

#ifdef WAN_STATUS_LED_EVENT
#define IPV4_STATE                   "ipv4_state"
#define IPV6_STATE                   "ipv6_state"
#define MAPT_STATE                   "mapt_state"
cpe_wan_led_events_t led_wan_events[] =
{
{ "down", "down", "down", "rdkb_wan_link_down"},
{ "down", "up",   "down", "rdkb_ipv6_only"    },
{ "up",   "down", "down", "rdkb_ipv4_only"    },
{ "up",   "up",   "down", "rdkb_dualstack"    },
{ "down", "up",   "up",   "rdkb_mapt"         },
{ "up",   "up",   "up",   "rdkb_mapt"         }
};

#define MAX_WAN_EVENTS  sizeof(led_wan_events)/sizeof(led_wan_events[0])
#endif

extern char g_Subsystem[32];
extern ANSC_HANDLE bus_handle;
led_data_t g_led_data;
led_mode_data_t g_mode_data;
extern cpe_led_events_t led_events[];

static char * ledmgr_get_str_from_event (cpe_event_t event)
{
    if ((event >= 0) && (event < MAX_EVENTS))
    {
        return led_events[event].event_str;
    }
    return NULL;
}

cpe_event_t ledmgr_get_event_from_str (char * event_str)
{
    if (event_str == NULL)
        return MAX_EVENTS;

    cpe_event_t i;
    for (i = 0; i < MAX_EVENTS; i++)
    {
        if (strncmp(led_events[i].event_str, event_str, strlen(event_str)) == 0)
        {
            return i;
        }
    }
    return i;
}
#ifdef WAN_STATUS_LED_EVENT
static int parse_wan_event(char * event_str, char * wan_event)
{
    cpe_wan_led_events_t *led_wan_data;
    char *tmp_buf;
    int i;
    uint32_t length;
    if((event_str == NULL) || (wan_event == NULL))
    {
        CcspTraceError(("%s %d Invalid input",__FUNCTION__,__LINE__));
        return FAILURE;
    }
    length = strlen(event_str);
    if ((tmp_buf = malloc(length+1)) == NULL)
    {
        CcspTraceError(("%s %d memory allocation failed",__FUNCTION__,__LINE__));
        return FAILURE;
    }

    led_wan_data = (cpe_wan_led_events_t*) malloc(sizeof(cpe_wan_led_events_t));
    memset(led_wan_data, 0, sizeof(cpe_wan_led_events_t));
    memset(tmp_buf, 0, length+1);
    strncpy(tmp_buf, event_str, length+1);

    char *token = strtok(tmp_buf, ",");

    while (token != NULL) {
        char *colon = strchr(token, ':');

        if (colon != NULL) {
            *colon = '\0'; // Null-terminate the string at the colon
            char *event = token;
            char *value = colon + 1; // Point to the character after the colon

            if((event != NULL) && (value != NULL))
            {
                // Print the parsed event and value for debug
                CcspTraceError(("%s Event: [%s], Value: [%s]\n", __FUNCTION__, event, value));
                if(strcmp(event, IPV4_STATE)==0 )
                {
                    strncpy(led_wan_data->ipv4_event_value, value, sizeof(led_wan_data->ipv4_event_value)-1);
                }
                else if(strcmp(event, IPV6_STATE)==0 )
                {
                    strncpy(led_wan_data->ipv6_event_value, value, sizeof(led_wan_data->ipv6_event_value)-1);
                }
                else if(strcmp(event, MAPT_STATE)==0 )
                {
                    strncpy(led_wan_data->mapt_event_value, value, sizeof(led_wan_data->mapt_event_value)-1);
                }
            }
        }

        // Get the next token
        token = strtok(NULL, ",");
    }

    free(tmp_buf);
    for (i = 0; i < MAX_WAN_EVENTS; i++)
    {
        if ((strcmp(led_wan_events[i].ipv4_event_value, led_wan_data->ipv4_event_value) == 0) && 
            (strcmp(led_wan_events[i].ipv6_event_value, led_wan_data->ipv6_event_value) == 0) &&
            (strcmp(led_wan_events[i].mapt_event_value, led_wan_data->mapt_event_value) == 0))
        {
            strncpy(wan_event, led_wan_events[i].wan_event, BUFLEN_64-1);
            CcspTraceError(("Final Wan Status Event: %s\n", led_wan_events[i].wan_event));
	    free(led_wan_data);
            return SUCCESS;
        }
    }
    free(led_wan_data);
    return FAILURE;
}

cpe_event_t ledmgr_get_wan_event_from_str(char * event_str)
{
    if (event_str == NULL)
        return MAX_EVENTS;

    char wan_event[BUFLEN_64] = {0};
    cpe_event_t event_index = MAX_EVENTS;
    //parse the string and get the right wan event
    if(parse_wan_event(event_str, wan_event) == SUCCESS)
    {
        event_index = ledmgr_get_event_from_str (wan_event);
    }
    return event_index;
}
#endif
static led_mode_t * get_mode_data_for_mode_name (const char * mode)
{
    led_mode_t * mode_data_arr = g_mode_data.mode_obj_head;
    int no_of_modes = g_mode_data.no_of_modes;

    if ((mode == NULL) || (strlen(mode) >= OBJ_NAME_LEN) || (mode_data_arr == NULL))
    {
        CcspTraceError(("%s %d Invalid args...\n", __FUNCTION__, __LINE__));
        return NULL;
    }   

    led_mode_t * tmp_mode_data = NULL;

    int i = 0;

    for (i = 0; i < no_of_modes; i++)
    {
        tmp_mode_data = &mode_data_arr[i];
        if (strncmp(tmp_mode_data->name, mode, strlen(mode)) == 0)
        {
            return tmp_mode_data;
        }
    }

    return NULL;
}

char * ledmgr_read_config_file(FILE * fp)
{
    char * buffer = NULL;
    long fsize = 0;
    long bytes = 0;

    if (fp == NULL)
    {
        CcspTraceError(("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return NULL;
    }

    // obtain file size
    fseek(fp, 0, SEEK_END); // seek to the end of the file
    fsize = ftell(fp);  // get the current file pointer 
    rewind(fp);  // rewind to the beginning of the file

    // allocate memory for size of file + 1 
    buffer = (char *)malloc(sizeof(char)*fsize + 1);
    if (buffer == NULL)
    {
        perror("malloc() failed:");
        fclose(fp);
        return NULL;
    }

    memset(buffer, 0, sizeof(char)*fsize + 1);

    bytes = fread(buffer, sizeof(char), fsize, fp);
    if (bytes != fsize)
    {
        perror("fread() failed:");
        free(buffer);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return buffer;
}

// JSON C usage part 
static const char * get_string_value_from_json_obj(struct json_object * obj, char * key)
{
    if ((obj == NULL) || (key == NULL))
    {
        CcspTraceError(("%s:%d Invalid args\n", __FUNCTION__, __LINE__));
        return NULL;
    }

    struct json_object * str_json_obj;

    if (FALSE == json_object_object_get_ex(obj, key, &str_json_obj))
    {
        CcspTraceError(("%s:%d No key:%s found\n", __FUNCTION__, __LINE__, key));
        return NULL;
    }

    int val_type = 0;

    val_type = json_object_get_type(str_json_obj);
    if (val_type != json_type_string)
    {
        CcspTraceError(("State name not found in config file\n"));
        return NULL;
    }

    return json_object_get_string(str_json_obj);

}

static led_state_t * get_state_from_state_list (led_state_t * state_list, int no_of_states, const char * name)
{
    if ((state_list == NULL) || (name == NULL))
    {
        CcspTraceError(("%s %d: Invalid args\n", __FUNCTION__, __LINE__));
        return NULL;
    }

    int i = 0;
    
    led_state_t * tmp_state_obj = NULL;
    led_state_t * state_obj = NULL;

    for(i = 0; i < no_of_states; i++)
    {
        tmp_state_obj = &state_list[i];
        if (tmp_state_obj == NULL)
            continue;
        if (strncmp(tmp_state_obj->name, name, strlen(name)) == 0)
        {
            state_obj = tmp_state_obj;
            break;
        }
    }

    return state_obj;
    
}

static struct json_object * get_json_array_object(struct json_object * parsed_json_obj, const char * key)
{
    if ((key == NULL) || (parsed_json_obj == NULL))
    {
        CcspTraceError(("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return NULL;
    }

    struct json_object * tmp_array_json_obj = NULL;

    if (FALSE == json_object_object_get_ex(parsed_json_obj, key, &tmp_array_json_obj))
    {
        CcspTraceError(("%s %d: config file does not contain key: \"%s\".\n", __FUNCTION__, __LINE__, key));
        return NULL;
    }
    if (json_object_get_type(tmp_array_json_obj) != json_type_array)
    {
        CcspTraceError(("%s %d: config file does not contain array object in key: \"%s\".\n", __FUNCTION__, __LINE__, key));
        return NULL;
    }

    return tmp_array_json_obj;
}

static led_transition_t * get_transition_list_from_json_obj (struct json_object * transition_list_json_obj, led_state_t * state_list_data, int no_of_states)
{
    if ((transition_list_json_obj == NULL) || (state_list_data == NULL) || (no_of_states <= 0))
    {
        CcspTraceError(("%s %d: Invalid args..\n", __FUNCTION__, __LINE__));
        return NULL;
    }

    struct json_object * transition_json_obj = NULL;

    int no_of_transition = json_object_array_length(transition_list_json_obj);
    if (no_of_transition <= 0)
    {
        CcspTraceError(("%s %d: No transiton array found for state\n" , __FUNCTION__, __LINE__));
        return NULL;
    }

    led_transition_t * transitions_list_data = malloc(sizeof(led_transition_t) * no_of_transition);
    if (transitions_list_data == NULL)
    {
        CcspTraceError(("%s %d: malloc failed\n" , __FUNCTION__, __LINE__));
        return NULL;
    }

    int i = 0;
    const char * event = NULL;
    const char * next_state = NULL;

    for ( i = 0; i < no_of_transition; i++ )
    {
        memset(&transitions_list_data[i], 0, sizeof(led_transition_t));

        transition_json_obj = json_object_array_get_idx(transition_list_json_obj, i); 
        // for each Transition

        // get "event" from the "transition" object and create LED state transition data
        event = get_string_value_from_json_obj(transition_json_obj, EVENT_JSON_KEY);
        if (event == NULL)
        {
            CcspTraceError(("%s %d cannot fetch event type from transition.\n", __FUNCTION__, __LINE__));
            return transitions_list_data;
        }
        // Set event for transition data
        transitions_list_data[i].event = ledmgr_get_event_from_str((char *)event);

        // get "next_state" from the "transition" object and create LED state transition data
        next_state = get_string_value_from_json_obj(transition_json_obj, NEXT_STATE_JSON_KEY);
        if (next_state == NULL)
        {
            CcspTraceError(("%s %d cannot fetch next_state type from transition.\n", __FUNCTION__, __LINE__));
            return transitions_list_data;
        }
        if ((transitions_list_data[i].next_state = get_state_from_state_list(state_list_data, no_of_states, next_state)) == NULL)
        {
            CcspTraceError(("%s %d cannot find state %s from state list.\n", __FUNCTION__, __LINE__, next_state));
            return transitions_list_data;
        }
    }
    return transitions_list_data;
}


static int set_state_to_transition(led_state_t * states, int no_of_states, struct json_object * state_array_json_obj)
{
    if ((state_array_json_obj == NULL) || (json_object_get_type(state_array_json_obj) != json_type_array)
            || (states == NULL) || (no_of_states <= 0))
    {
        CcspTraceError(("%s %d: Invalid args\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    int i = 0;
    int no_of_transition = 0;
    const char * state_name = NULL;
    struct json_object * state_json_obj = NULL;
    led_state_t * state_data = NULL;
    struct json_object * transition_list_json_obj = NULL;

    for ( i = 0; i < json_object_array_length(state_array_json_obj); i++ )
    {
        state_json_obj = json_object_array_get_idx(state_array_json_obj, i); 

        state_name = get_string_value_from_json_obj(state_json_obj, NAME_JSON_KEY);
        if (state_name == NULL)
        {
            CcspTraceError(("%s %d: Unable to fetch state name\n", __FUNCTION__, __LINE__));
            return FAILURE;
        }

        // for every State in json array list, check if there is a state saved in local data structure
        state_data = get_state_from_state_list(states, no_of_states, state_name);
        if (state_data == NULL)
        {
            CcspTraceError(("%s %d: Unable to fetch state data with name %s\n", __FUNCTION__, __LINE__, state_name));
            return FAILURE;
        }

        // get "transition" object from "states" 
        transition_list_json_obj = get_json_array_object(state_json_obj, TRANSITIONS_JSON_KEY);
        if (transition_list_json_obj == NULL)
        {
            CcspTraceError(("%s:%d key %s not found in config file\n" , __FUNCTION__, __LINE__, TRANSITIONS_JSON_KEY));
            return FAILURE;
        }   

        no_of_transition = json_object_array_length(transition_list_json_obj);
        if (no_of_transition > 0)
        {
            // Set transition list for each state data
            state_data->transitions_list = get_transition_list_from_json_obj(transition_list_json_obj, states, no_of_states);

            // Set no_of_transitions for each state data
            state_data->no_of_transitions = no_of_transition;
        }

        transition_list_json_obj = NULL;

    }

    return SUCCESS;
}

#ifdef LEDMGR_WEBCONFIG
// converts to a single command string from struct/json formatted [char *buffer]
char *get_command_string(char *buffer, const char *current_mode_name)
{
    const cJSON *led_modes = NULL;
    const cJSON *led_mode = NULL;
    const cJSON *led_mode_name = NULL;
    const cJSON *led_mode_command = NULL;
    char* commandString = NULL;

    if (buffer == NULL)
    {
        CcspTraceError(("%s %d: Error finding the contents of the buffer holding the json data.\n", __FUNCTION__, __LINE__));
        return NULL;
    }
    if (current_mode_name == NULL)
    {
        CcspTraceError(("%s %d: Error finding the selected led mode name.\n", __FUNCTION__, __LINE__));
        return NULL;
    }
    cJSON *json_obj = cJSON_ParseWithLength(buffer, strlen(buffer));
    if (json_obj == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            CcspTraceError(("%s %d: cJSON parsing Error before: %s\n", __FUNCTION__, __LINE__, error_ptr));
            return NULL;
        }
    }
    led_modes = cJSON_GetObjectItemCaseSensitive(json_obj, "led_modes");
    if (led_modes == NULL)
    {
        CcspTraceError(("%s %d: Error finding led modes object in JSON.\n", __FUNCTION__, __LINE__));
        cJSON_Delete(json_obj);
        return NULL;
    }
    //check for match, print matching mode to commandString (unformatted)
    cJSON_ArrayForEach(led_mode, led_modes)
    {
        led_mode_name = cJSON_GetObjectItemCaseSensitive(led_mode, "name");

        if (led_mode_name == NULL)
        {
            CcspTraceError(("%s %d: Error finding name in led mode object.\n", __FUNCTION__, __LINE__));
            cJSON_Delete(json_obj);
            return NULL;
        }
        if (strncmp(led_mode_name->valuestring, current_mode_name, strlen(current_mode_name)) == 0)
        {
            led_mode_command = cJSON_GetObjectItemCaseSensitive(led_mode, "command");
            if (led_mode_command == NULL)
            {
                CcspTraceError(("%s %d: Error finding command in led mode object.\n", __FUNCTION__, __LINE__));
                cJSON_Delete(json_obj);
                return NULL;
            }
            commandString = cJSON_PrintUnformatted(led_mode_command);
            if(commandString == NULL) {
                CcspTraceError(("%s %d: Error allocating/printing to commandString\n", __FUNCTION__, __LINE__));
                cJSON_Delete(json_obj);
                return NULL;
            }
            break;
        }
    }
    cJSON_Delete(json_obj);
    return commandString;
}
#endif

#ifdef LEDMGR_WEBCONFIG
static int get_led_mode_data_from_json_obj (struct json_object * parsed_json_obj, char* buffer)
#else
static int get_led_mode_data_from_json_obj (struct json_object * parsed_json_obj)
#endif
{
    if (parsed_json_obj == NULL)
    {
        CcspTraceError(("%s %d: Failed to parse JSON file\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    struct json_object * mode_array_json_obj = get_json_array_object(parsed_json_obj, LED_MODES_JSON_KEY);

    if (mode_array_json_obj == NULL)
    {
        CcspTraceError(("%s %d: failed to get json object.\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    int no_of_modes =  json_object_array_length(mode_array_json_obj);
    CcspTraceInfo(("%s %d: no_of_modes = %d\n", __FUNCTION__, __LINE__, no_of_modes));

    led_mode_t * mode_data_arr = malloc(sizeof(led_mode_t) * no_of_modes);
    if (mode_data_arr == NULL)
    {
        CcspTraceError(("%s %d: malloc() failed\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    g_mode_data.mode_obj_head = mode_data_arr;
    g_mode_data.no_of_modes = no_of_modes;
    CcspTraceInfo(("%s %d: g_mode_data.mode_obj_head = %p and g_mode_data.no_of_modes =%d\n", __FUNCTION__, __LINE__, g_mode_data.mode_obj_head, g_mode_data.no_of_modes));

    int i = 0;
    const char * mode_name = NULL;
    const char * name = NULL;
#ifdef LEDMGR_WEBCONFIG
    char * cmd_name = NULL;
#else
    const char * cmd_name = NULL;
#endif
    struct json_object * mode_json_obj = NULL;
    led_hal_command_t * cmd = NULL;

    for ( i = 0; i < no_of_modes; i++ )
    {
        CcspTraceInfo(("%s %d: i = %d\n", __FUNCTION__, __LINE__, i));
        memset(&mode_data_arr[i], 0, sizeof(led_mode_t));

        mode_json_obj = json_object_array_get_idx(mode_array_json_obj, i); 

        // for each MODE object
        // Set name for Mode
        mode_name = get_string_value_from_json_obj(mode_json_obj, NAME_JSON_KEY);
        if ((mode_name == NULL) || (strlen(mode_name) > (OBJ_NAME_LEN -1)))
        {
            CcspTraceError(("%s %d: Invalid LED Mode in config file.\n", __FUNCTION__, __LINE__));
            return FAILURE;
        }

        strncpy(mode_data_arr[i].name, mode_name, strlen(mode_name));

#ifdef LEDMGR_WEBCONFIG
        // Set command for Mode but first check the type of json
        cmd_name = get_command_string(buffer, mode_name); // get command string from new json structure
#else
        cmd_name = get_string_value_from_json_obj(mode_json_obj, MODE_CMD_JSON_KEY);
#endif
        if ((cmd_name == NULL) || (strlen(cmd_name) > (OBJ_NAME_LEN -1)))
        {
            CcspTraceError(("%s %d: Invalid CMD Mode for Mode %s.\n", __FUNCTION__, __LINE__, mode_name));
            return FAILURE;
        }
        cmd = malloc (sizeof(led_hal_command_t));
        if (cmd == NULL)
        {
            CcspTraceError(("%s %d: malloc failed.\n", __FUNCTION__, __LINE__));
            return FAILURE;
        }
        memset (cmd, 0, sizeof(led_hal_command_t));
        strncpy(cmd->cmd, cmd_name, strlen(cmd_name));

#ifdef LEDMGR_WEBCONFIG
        free(cmd_name);
#endif
        mode_data_arr[i].hal_command = cmd;
    }

    return SUCCESS;
}

static led_state_t * get_state_from_led_json_obj (struct json_object * state_array_json_obj)
{
    if (state_array_json_obj == NULL)
    {
        CcspTraceError(("%s %d: Invalid args..\n",__FUNCTION__, __LINE__));
        return NULL;
    }

    int no_of_states = json_object_array_length(state_array_json_obj);
    if (no_of_states <= 0)
    {
        CcspTraceError(("%s %d: State array empty\n",__FUNCTION__, __LINE__));
        return NULL;
    }

    led_state_t * states = malloc(sizeof(led_state_t) * no_of_states);
    if (states == NULL)
    {
        CcspTraceError(("%s:%d malloc() failed\n", __FUNCTION__, __LINE__));
        return NULL;
    }

    int i = 0;
    const char * state_name = NULL;
    const char * state_mode = NULL;
    struct json_object * state_json_obj = NULL;

    for (i = 0; i < no_of_states; i++)
    {
        memset(&states[i], 0, sizeof(led_state_t));
        state_json_obj = json_object_array_get_idx(state_array_json_obj, i);

        // For every State
        // get "name" from the "state" object and create LED data
        state_name = get_string_value_from_json_obj(state_json_obj, NAME_JSON_KEY);
        if (state_name == NULL)
        {
            CcspTraceError(("%s %d Invalid LED Name in config file.\n", __FUNCTION__, __LINE__));
            return NULL;
        }
        strncpy(states[i].name, state_name, strlen(state_name));

        // get "led_mode" from the object "state" and store it in LED state data
        state_mode = get_string_value_from_json_obj(state_json_obj, LED_MODE_JSON_KEY);
        if (state_mode == NULL)
        {
            CcspTraceError(("%s %d: transition for state %s not found in config file\n", __FUNCTION__, __LINE__, state_name));
            return NULL;
        }
        states[i].led_mode = get_mode_data_for_mode_name(state_mode);
    }
    return states;
}

static int geled_data_t_from_json_obj (struct json_object * parsed_json_obj)
{
    if (parsed_json_obj == NULL)
    {
        CcspTraceError(("%s %d: Failed to parse JSON file\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    struct json_object * led_array_json_obj = get_json_array_object(parsed_json_obj, LED_JSON_KEY);

    if (led_array_json_obj == NULL)
    {
        CcspTraceError(("%s %d: failed to get json object.\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    int no_of_leds =  json_object_array_length(led_array_json_obj);

    led_t * led_data_arr = malloc (sizeof(led_t) * no_of_leds);

    if (led_data_arr == NULL)
    {
        CcspTraceError(("%s %d: malloc() failed\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    g_led_data.led_obj_head = led_data_arr; 
    g_led_data.no_of_leds = no_of_leds; 

    int i = 0;
    int no_of_states = 0;
    const char * led_name = NULL;
    struct json_object * led_json_obj = NULL;
    struct json_object * state_array_json_obj = NULL;

    for (i = 0; i < no_of_leds; i++)
    {
        memset (&led_data_arr[i], 0, sizeof(led_t));

        led_json_obj =  json_object_array_get_idx(led_array_json_obj, i);

        // for each LED object
        // Set name for LED
        led_name = get_string_value_from_json_obj(led_json_obj, NAME_JSON_KEY);
        if ((led_name == NULL) && (strlen(led_name) > (OBJ_NAME_LEN -1)))
        {
            CcspTraceError(("%s %d: Invalid LED Mode in config file.\n", __FUNCTION__, __LINE__));
            return FAILURE;
        }
        strncpy(led_data_arr[i].name, led_name, strlen(led_name));

        // Set no_of_states for LED
        state_array_json_obj = get_json_array_object (led_json_obj, STATES_JSON_KEY);
        if (state_array_json_obj == NULL)
        {       
            CcspTraceError(("%s %d Cannot fetch states from LED.\n", __FUNCTION__, __LINE__));
            return FAILURE;
        }
        no_of_states = json_object_array_length(state_array_json_obj);
        led_data_arr[i].no_of_states = no_of_states;

        if (no_of_states > 0)
        {
            // Set state_list for LED
            led_data_arr[i].states = get_state_from_led_json_obj(state_array_json_obj);

            // Set current state for LED as first state in statelist
            led_data_arr[i].current_state = led_data_arr[i].states;

            // Set transitions and point them to correct states
            set_state_to_transition(led_data_arr[i].states, no_of_states, state_array_json_obj);
        }
        state_array_json_obj = NULL;
        
    }
    return SUCCESS;
}


int ledmgr_parse_config_file (char * buffer)
{
    if (buffer == NULL)
    {
        CcspTraceError(("%s %d: Invalid args\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }
    
    struct json_object * parsed_json_obj = NULL;
    parsed_json_obj = json_tokener_parse(buffer);

    if (parsed_json_obj == NULL)
    {
        CcspTraceError(("%s %d: Failed to parse JSON file.\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    // create LED_MODE data
#ifdef LEDMGR_WEBCONFIG
    if (get_led_mode_data_from_json_obj(parsed_json_obj, buffer) == FAILURE)
#else
    if (get_led_mode_data_from_json_obj(parsed_json_obj) == FAILURE)
#endif
    {
        CcspTraceError(("%s %d: Failed to get LED Mode data from config file\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }
    CcspTraceInfo(("%s %d: Reading of LED Mode data - OK \n", __FUNCTION__, __LINE__));

    // create LED data
    if (geled_data_t_from_json_obj(parsed_json_obj) == FAILURE)
    {
        CcspTraceError(("%s %d: Failed to get LED Mode data from config file.\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }
    CcspTraceInfo(("%s %d: Reading of LED data - OK \n", __FUNCTION__, __LINE__));

    json_object_put(parsed_json_obj);

    return SUCCESS;
}

// debug call to check if parsing of json file is ok
int ledmgr_print_data_to_logfile()
{
    int i = 0, j = 0, k = 0;

    led_t * led_data_arr = g_led_data.led_obj_head;
    led_mode_t * mode_data_arr = g_mode_data.mode_obj_head;
    int no_of_modes = g_mode_data.no_of_modes;
    int no_of_leds = g_led_data.no_of_leds;

    if (mode_data_arr)
    {
        led_hal_command_t * cmd = NULL;

        for (i=0; i< no_of_modes; i++)
        {
            CcspTraceInfo(("%d.Mode Name: %s\n",(i+1) , mode_data_arr[i].name));
            cmd = mode_data_arr[i].hal_command;
            CcspTraceInfo(("\tCommand: %s.\n", cmd->cmd));
        }

    }

    if (led_data_arr)
    {
        int no_of_states = 0;
        int no_of_transitions = 0;

        led_state_t * states = NULL;
        led_transition_t * transitions = NULL;

        for (i = 0; i < no_of_leds; i++)
        {
            CcspTraceInfo(("%d.Led Name: %s\n", (i+1), led_data_arr[i].name));
            CcspTraceInfo(("   Led current State: %s\n", led_data_arr[i].current_state->name));
            states = led_data_arr[i].states;
            no_of_states = led_data_arr[i].no_of_states;
            for (j = 0; j < no_of_states; j++)
            {
                CcspTraceInfo(("%d.State: %s.\n",(j+1), states[j].name)); 
                CcspTraceInfo(("\tState Mode: %s.\n", states[j].led_mode->name)); 
                transitions = states[j].transitions_list;
                no_of_transitions = states[j].no_of_transitions;
                for (k = 0; k < no_of_transitions; k++)
                {
                    CcspTraceInfo(("%d.Transition Event: %s.\n",(k+1), ledmgr_get_str_from_event(transitions[k].event))); 
                    CcspTraceInfo(("\tNext State: %s.\n", transitions[k].next_state->name)); 
                }
            }
        }
    }

    return SUCCESS;
}


void ledmgr_free_data()
{
    int i = 0, j = 0;

    led_t * led_data_arr = g_led_data.led_obj_head;
    led_mode_t * mode_data_arr = g_mode_data.mode_obj_head;
    int no_of_modes = g_mode_data.no_of_modes;
    int no_of_leds = g_led_data.no_of_leds;

    // free mode data
    if (mode_data_arr != NULL)
    {
        for (i = 0; i < no_of_modes; i++)
        {
            if (mode_data_arr[i].hal_command) 
                free(mode_data_arr[i].hal_command);
        }
        free(mode_data_arr);

        g_mode_data.mode_obj_head = NULL;
        g_mode_data.no_of_modes = 0;
    }

    // free led data
    if (led_data_arr != NULL)
    {
        led_state_t * states = NULL;
        int no_of_states = 0;

        for(i = 0; i < no_of_leds; i++)
        {
            states = led_data_arr[i].states;
            no_of_states = led_data_arr[i].no_of_states;
            for (j = 0; j < no_of_states; j++)
            {
                free(states[j].transitions_list);
            }
            free(states);
        }
        free(led_data_arr);
        g_led_data.led_obj_head = NULL;
        g_led_data.no_of_leds = 0;
    }

}
