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
/*
 * Copyright [2014] [Cisco Systems, Inc.]
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


#ifndef _LEDMGR_DEFN_H_
#define _LEDMGR_DEFN_H_

#define OBJ_NAME_LEN                 128

typedef struct led_hal_command_t
{
    char cmd[OBJ_NAME_LEN];             // command to be sent to HAL
} led_hal_command_t;

typedef struct led_mode_t
{
    char name[OBJ_NAME_LEN];
    led_hal_command_t * hal_command; // linked list of transitions that this state can take
} led_mode_t;

typedef struct led_transition_t
{
    cpe_event_t event;               
    struct led_state_t *next_state; // pointer to next state. this pointer is fixed when config data is loaded.
} led_transition_t;

typedef struct led_state_t
{
    char name[OBJ_NAME_LEN];
    led_transition_t * transitions_list; // linked list of transitions that this state can take
    led_mode_t * led_mode;               // pointer for the structure with the command list
    int no_of_transitions;              // holds number of possible transition this state can support
} led_state_t;

typedef struct led_t
{
    char name[OBJ_NAME_LEN];
    led_state_t* states;           // linked list of states that this led can take
    led_state_t* current_state;    // current state of the led
    int no_of_states;                     // holds number of possible states this LED can support
} led_t;

typedef struct 
{
    led_t * led_obj_head; 
    int no_of_leds;
} led_data_t;

typedef struct 
{
    led_mode_t * mode_obj_head;
    int no_of_modes;
} led_mode_data_t;

typedef struct {
   cpe_event_t event;
   char * event_str;
} cpe_led_events_t;

#ifdef WAN_STATUS_LED_EVENT
typedef struct {
   char  ipv4_event_value[BUFLEN_64];
   char  ipv6_event_value[BUFLEN_64];
   char  mapt_event_value[BUFLEN_64];
   char  wan_event[BUFLEN_64];
} cpe_wan_led_events_t;
#endif
int ledmgr_catch_events ();

#endif /* _LEDMGR_DEFN_H_ */ 
