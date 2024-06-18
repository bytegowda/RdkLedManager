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

// ipc headers
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sysevent/sysevent.h>

#define LOCALHOST         "127.0.0.1"
#define MAX_RETRIES       6
#define LED_SYSEVENT_NAME "led_evt_handler"
#define LED_SYSEVENT_KEY  "led_event"

#ifdef WAN_STATUS_LED_EVENT
#define LED_WAN_SYSEVENT_KEY  "rdkb_wan_status"
#endif

cpe_led_events_t led_events[] = 
{
{    ePowerOn,          "power_on"               },
{    ePowerOff,         "power_off"              },
{    eProximity,        "proximity"              },
{    eDslTraining,      "rdkb_dsl_training"      },
{    eWanLinkUp,        "rdkb_wan_link_up"       },
{    eWanLinkDown,      "rdkb_wan_link_down"     },
#ifndef WAN_STATUS_LED_EVENT
{    eIPv4Up,           "rdkb_ipv4_up"           },
{    eIPv4Down,         "rdkb_ipv4_down"         },
{    eIPv6Up,           "rdkb_ipv6_up"           },
{    eIPv6Down,         "rdkb_ipv6_down"         },
{    eMaptUp,           "rdkb_mapt_up"           },
{    eMaptDown,         "rdkb_mapt_down"         },
#else
{    eIPv4Only,         "rdkb_ipv4_only"         },
{    eIPv6Only,         "rdkb_ipv6_only"         },
{    eDualStackUp,      "rdkb_dualstack"         },
{    eMaptUp,           "rdkb_mapt"              },
#endif
{    eFwUpdateStart,    "rdkb_fwupdate_start"    },
{    eFwUpdateStop,     "rdkb_fwupdate_stop"     },
{    eFwUpdateComplete, "rdkb_fwupdate_complete" },
{    eFwUpdateReset,    "rdkb_fwupdate_reset" },
{    eFwUpdateStartManual, "rdkb_fwupdate_manual_start" },
{    eVoiceProvisioned, "rdkb_voice_provisioned" },
{    eVoiceUp,          "rdkb_voice_up"          },
{    eVoiceRinging,     "rdkb_voice_ringing"     },
{    eVoiceError,       "rdkb_voice_error"       },
{    eVoiceDown,        "rdkb_voice_down"        },
{    eWiFiUp,           "rdkb_wifi_up"           },
{    eWiFiDown,         "rdkb_wifi_down"         },
{    eWpsStart,         "rdkb_wps_start"         },
{    eWpsOverlap,       "rdkb_wps_overlap"       },
{    eWpsTimeout,       "rdkb_wps_timeout"       },
{    eWpsStop,          "rdkb_wps_stop"          },
{    eWpsClientConnected, "rdkb_wps_client_connected" },
{    eWpsWlanHoldStart, "rdkb_wps_wlan_hold_start" },
{    eWpsWlanHoldStop,  "rdkb_wps_wlan_hold_stop" },
{    eWpsFrHoldStart,   "rdkb_wps_fr_hold_start" },
{    eWpsFrHoldStop,    "rdkb_wps_fr_hold_stop"  },
{    eFwDownloadStart,  "rdkb_fwdownload_start"    },
{    eFwDownloadStop,   "rdkb_fwdownload_stop"    },
{    eWanEstablish,   "rdkb_wan_establish"    },
{    eFactoryReset,   "rdkb_factory_reset"    },
{    eExtConnected,      "ext_connected"         },
{    eExtConnecting,     "ext_connecting"        },
{    eExtFail,           "ext_fail"              },
{    eExtDisconnected,   "ext_disconnected"      },
{    eGfoEnabled,        "gfo_enabled"           },
{    eGfoDisabled,       "gfo_disabled"          },
{    eWfoEnabled,        "wfo_enabled"           },
{    eWfoDisabled,       "wfo_disabled"          },
#if defined(FEATURE_RDKB_LED_MANAGER_CAPTIVE_PORTAL)
{    eLimitedOperational, "rdkb_limited_operational" },
{    eFwDownloadStopCaptive, "rdkb_fwdownload_stop_captivemode" },
#endif
{    MAX_EVENTS,        ""                       }
};

extern led_data_t g_led_data;
extern led_mode_data_t g_mode_data;

cpe_event_t ledmgr_get_event_from_str (char * event_str);
#ifdef WAN_STATUS_LED_EVENT
cpe_event_t ledmgr_get_wan_event_from_str (char * event_str);
#endif
static int handle_event(cpe_event_t event)
{

    led_t * led_data_arr = g_led_data.led_obj_head;
    led_mode_t * mode_data_arr = g_mode_data.mode_obj_head;

    if ((mode_data_arr == NULL) || (led_data_arr == NULL))
    {
        CcspTraceError(("%s %d: invalid event or invalid config\n", __func__, __LINE__));
        return FAILURE;
    }

    bool bupdate = FALSE;
    int i = 0, j = 0;
    int no_of_transitions = 0;
    led_state_t * state = NULL;
    led_transition_t * transitions = NULL;
    led_mode_t * mode = NULL;
    led_hal_command_t * cmd = NULL;
    LEDMGMT_PARAMS led_data;

    for (i = 0; i < g_led_data.no_of_leds; i++) 
    {
        state = led_data_arr[i].current_state;
        if (state == NULL)
            continue;   // check remaining LEDs

        CcspTraceInfo(("\n\nfor LED %s with current state %s..\n", led_data_arr[i].name, state->name));
        transitions = state->transitions_list;
        no_of_transitions = state->no_of_transitions;
        for (j = 0; j < no_of_transitions; j++)
        {
            if (transitions[j].event == event)
            {
                led_data_arr[i].current_state = transitions[j].next_state;
                CcspTraceInfo(("\n\nLED %s set to  state %s..\n", led_data_arr[i].name, led_data_arr[i].current_state->name));
                mode = led_data_arr[i].current_state->led_mode;
                if (mode == NULL)
                {
                    CcspTraceError(("%s %d: Unable to get mode for current state\n", __FUNCTION__, __LINE__));
                    break; //break from transition and check next LED
                }

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
                bupdate = TRUE;
                break; //break from transition and check next LED
            }
        }
    }

    if (bupdate)
        return SUCCESS;
    else
        return FAILURE;
}

int ledmgr_catch_events()
{

    cpe_event_t t_event;
    token_t sysevent_token;
    int sysevent_fd;
    async_id_t led_state_asyncid;
    char * sysevent_name = LED_SYSEVENT_NAME;
    char * sysevent_key = LED_SYSEVENT_KEY;
#ifdef WAN_STATUS_LED_EVENT
    char * sysevent_wan_key = LED_WAN_SYSEVENT_KEY;
    async_id_t led_wan_state_asyncid;
#endif
    bool status = FALSE;
    int retry = 0;

    do
    {
        sysevent_fd =  sysevent_open(LOCALHOST, SE_SERVER_WELL_KNOWN_PORT, SE_VERSION, sysevent_name, &sysevent_token);
        if (sysevent_fd < 0)
            status = FALSE;
        else
        {
            CcspTraceInfo(("%s %d: sysevent_open ok\n", __FUNCTION__, __LINE__));
            status = TRUE;
        }

    }while((status == FALSE) && (retry++ < MAX_RETRIES));

    if (status == FALSE)
    {
        CcspTraceError(("%s %d: opening sysevent socket failure with error\n", __FUNCTION__, __LINE__));
        return FAILURE;
    }

    sysevent_set_options(sysevent_fd, sysevent_token, sysevent_key, TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd, sysevent_token, sysevent_key, &led_state_asyncid);
#ifdef WAN_STATUS_LED_EVENT
    sysevent_set_options(sysevent_fd, sysevent_token, sysevent_wan_key, TUPLE_FLAG_EVENT);
    sysevent_setnotification(sysevent_fd, sysevent_token, sysevent_wan_key, &led_wan_state_asyncid);
#endif
    char name[BUFLEN_32] = {0};
    char val[BUFLEN_64] = {0};
    int namelen = 0;
    int vallen  = 0;
    int err;
    async_id_t getnotification_asyncid;

    while(1)
    {
        memset(name, 0, BUFLEN_32);
        memset(val, 0, BUFLEN_64);
        namelen = sizeof(name);
        vallen  = sizeof(val);

        err = sysevent_getnotification(sysevent_fd, sysevent_token, name, &namelen,  val, &vallen, &getnotification_asyncid);
        if (err)
        {
            CcspTraceError(("%s %d: sysevent_getnotification failed with error: %d\n", __FUNCTION__, __LINE__, err));
            continue;
        }
        else
        {
            if (strcmp(name, sysevent_key) == 0)
            {
                CcspTraceInfo(("%s %d: received notification event %s with value = %s\n", __FUNCTION__, __LINE__, name, val));
                t_event = ledmgr_get_event_from_str (val);
                if (t_event >= MAX_EVENTS)
                {
                    CcspTraceError(("%s %d: unsupported event %s \n", __FUNCTION__, __LINE__, val));
                    continue;
                }
                if (handle_event(t_event) != SUCCESS)
                {
                    CcspTraceInfo(("%s %d: failed to handle event\n" ,__FUNCTION__, __LINE__));
                }
            }
#ifdef WAN_STATUS_LED_EVENT
            else if(strcmp(name, sysevent_wan_key) == 0)
            {
                CcspTraceInfo(("%s %d: received notification event %s with value = %s\n", __FUNCTION__, __LINE__, name, val));
                //parse the value and map to right wan event
                t_event = ledmgr_get_wan_event_from_str (val);
                if (t_event >= MAX_EVENTS)
                {
                    CcspTraceError(("%s %d: unsupported event %s \n", __FUNCTION__, __LINE__, val));
                    continue;
                }
                if (handle_event(t_event) != SUCCESS)
                {
                    CcspTraceInfo(("%s %d: failed to handle event\n" ,__FUNCTION__, __LINE__));
                }
            }
#endif
            else
            {
                CcspTraceError(("%s %d: undefined event %s \n", __FUNCTION__, __LINE__, name));
            }
        }
    }

    return SUCCESS;;
}

