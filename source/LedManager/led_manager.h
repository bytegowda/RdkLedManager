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


#ifndef _LEDMGR_H_
#define _LEDMGR_H_

#ifdef LEDMGR_WEBCONFIG
#include "ledmgr_webconfig.h"
#else
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <pthread.h>
#include "ccsp_trace.h"
#include "platform_hal.h"
#endif

#define COMPONENT_NAME_LEDMANAGER    "com.cisco.spvtg.ccsp.ledmanager"
#define COMPONENT_PATH_LEDMANAGER    "/com/cisco/spvtg/ccsp/ledmanager"
#define COMPONENT_VERSION_LEDMANAGER 1

#define SUCCESS                      0
#define FAILURE                      -1

#define BUFLEN_4                     4
#define BUFLEN_8                     8
#define BUFLEN_16                    16
#define BUFLEN_18                    18
#define BUFLEN_24                    24
#define BUFLEN_32                    32
#define BUFLEN_40                    40
#define BUFLEN_48                    48
#define BUFLEN_64                    64
#define BUFLEN_80                    80
#define BUFLEN_128                   128
#define BUFLEN_256                   256
#define BUFLEN_264                   264
#define BUFLEN_512                   512
#define BUFLEN_1024                  1024

typedef enum {
ePowerOn = 0,
ePowerOff,
eProximity,
eDslTraining,
eWanLinkUp,
eWanLinkDown,
#ifndef WAN_STATUS_LED_EVENT
eIPv4Up,
eIPv4Down,
eIPv6Up,
eIPv6Down,
eMaptUp,
eMaptDown,
#else
eIPv4Only,
eIPv6Only,
eDualStackUp,
eMaptUp,
#endif
eFwUpdateStart,
eFwUpdateStop,
eFwUpdateComplete,
eFwUpdateReset,
eFwUpdateStartManual,
eVoiceProvisioned,
eVoiceUp,
eVoiceRinging,
eVoiceError,
eVoiceDown,
eWiFiUp,
eWiFiDown,
eWpsStart,
eWpsOverlap,
eWpsTimeout,
eWpsStop,
eWpsClientConnected,
eWpsWlanHoldStart,
eWpsWlanHoldStop,
eWpsFrHoldStart,
eWpsFrHoldStop,
eFwDownloadStart,
eFwDownloadStop,
eWanEstablish,
eFactoryReset,
eTimeOut,
eExtConnected,
eExtConnecting,
eExtFail,
eExtDisconnected,
eGfoEnabled,
eGfoDisabled,
eWfoEnabled,
eWfoDisabled,
#if defined(FEATURE_RDKB_LED_MANAGER_CAPTIVE_PORTAL)
eLimitedOperational,
eFwDownloadStopCaptive,
#endif
MAX_EVENTS
}cpe_event_t;

#endif /* _LEDMGR_H_ */ 
