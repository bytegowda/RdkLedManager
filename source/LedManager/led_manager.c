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

#include "led_manager.h"
#include "led_manager_utils.h"
#include "led_manager_events.h"
#include "led_manager_dbus_utils.h"
#include "breakpad_wrapper.h"
#include "cap.h"
#ifdef LEDMGR_WEBCONFIG
#include "ledmgr_webconfig.h"
#include "ledmgr_rbus_handler_apis.h"
#else
#include "led_manager_global.h"
#endif

#define DEBUG_INI_NAME "/etc/debug.ini"
extern char * pComponentName;

#ifndef LEDMGR_WEBCONFIG
extern ANSC_HANDLE bus_handle;
extern char g_Subsystem[32];
#endif

extern PCOMPONENT_COMMON_LED_MANAGER g_pComponentCommonLedMgr;
cap_user appcaps;

#ifdef LEDMGR_WEBCONFIG
char conf_filepath[BUFLEN_128];
ANSC_STATUS retStatus = ANSC_STATUS_FAILURE;
#endif

static void ledmgr_start ()
{
    CcspTraceInfo(("%s %d: LedManager Start\n", __FUNCTION__, __LINE__));
    FILE * fd = NULL;

    fd = fopen("/var/tmp/ledmanager.pid", "w+");
    if ( !fd )
    {
        CcspTraceWarning(("Create /var/tmp/ledmanager.pid error. \n"));
        return;
    }
    else
    {
        char cmd[BUFLEN_8] = {0};  
        sprintf(cmd, "%d", getpid());
        fputs(cmd, fd);
        fclose(fd);
    }

    FILE *config_file = NULL;
#ifdef LEDMGR_WEBCONFIG
    if (platform_hal_initLed(conf_filepath) != SUCCESS)
    {
        CcspTraceError(("%s %d: Cannot get config file. Exiting..\n", __FUNCTION__, __LINE__));
        goto FAIL;
    }

    config_file = fopen(conf_filepath, "r");
#else 
    char filepath[BUFLEN_128] = {0};

    if (platform_hal_initLed(filepath) != SUCCESS)
    {
        CcspTraceError(("%s %d: Cannot get config file. Exiting..\n", __FUNCTION__, __LINE__));
        goto FAIL;
    }

    config_file = fopen(filepath, "r");
#endif

    if (config_file == NULL)
    {
        CcspTraceError(("%s %d: Cannot read config file. Exiting..\n", __FUNCTION__, __LINE__));
        goto FAIL;
    }

    char * buffer = NULL;
    buffer = ledmgr_read_config_file(config_file);

    if (buffer == NULL)
    {
        CcspTraceError(("%s %d: Cannot read config file. Exiting..\n", __FUNCTION__, __LINE__));
        goto FAIL;
    }

    CcspTraceInfo(("%s %d: Reading config file successful\n", __FUNCTION__, __LINE__));

    if (ledmgr_parse_config_file(buffer) != SUCCESS)
    {
        CcspTraceError(("%s %d: Cannot parse config file. Exiting..\n", __FUNCTION__, __LINE__));
        free (buffer);
        goto FAIL;
    }

    free (buffer); // buffer can be freed, config file is parsed and program has saved the data

    ledmgr_print_data_to_logfile();

#ifdef LEDMGR_WEBCONFIG 
    web_config_init();
    CcspTraceInfo(("%s %d: Webconfig initialisation complete\n", __FUNCTION__, __LINE__));
#endif

    if (ledmgr_catch_events() != SUCCESS)
    {
        CcspTraceError(("%s %d: Event listener init failed.\n", __FUNCTION__, __LINE__));
        goto FAIL;
    }

    ledmgr_free_data();

FAIL:
    ledmgr_free_data();
}

int main(int argc, char* argv[])
{

    int   cmdChar = 0;
    pComponentName = COMPONENT_NAME_LEDMANAGER;

#ifdef FEATURE_SUPPORT_RDKLOG
    RDK_LOGGER_INIT();
#endif

    BOOL                bRunAsDaemon = TRUE;
    int                 idx = 0;

    appcaps.caps = NULL;
    appcaps.user_name = NULL;

    bool blocklist_ret = false;
    blocklist_ret = isBlocklisted();
    if(blocklist_ret)
    {
        CcspTraceInfo(("NonRoot feature is disabled\n"));
    }
    else
    {
        CcspTraceInfo(("NonRoot feature is enabled, dropping root privileges for RdkInterDeviceManager Process\n"));
        init_capability();
        drop_root_caps(&appcaps);
        update_process_caps(&appcaps);
        read_capability(&appcaps);
    }

    for(idx = 1; idx < argc; idx++)
    {
        if((strcmp(argv[idx], "-subsys") == 0))
        {
            if((idx + 1) < argc)
            {
                AnscCopyString(g_Subsystem, argv[idx+1]);
            }
            else
            {
                CcspTraceError(("parameter after -subsys is missing"));
            }
        }
        else if ( strcmp(argv[idx], "-c") == 0 )
        {
            bRunAsDaemon = FALSE;
        }
    }

#if defined(_ANSC_WINDOWSNT)
    AnscStartupSocketWrapper(NULL);

    cmd_dispatch('e');

    RDKLogEnable = GetLogInfo(bus_handle,"eRT.","Device.LogAgent.X_RDKCENTRAL-COM_LoggerEnable");
    RDKLogLevel = (char)GetLogInfo(bus_handle,"eRT.","Device.LogAgent.X_RDKCENTRAL-COM_LogLevel");
    LEDMANAGER_RDKLogEnable = GetLogInfo(bus_handle,"eRT.","Device.LogAgent.X_RDKCENTRAL-COM_LoggerEnable");
    LEDMANAGER_RDKLogLevel = (char)GetLogInfo(bus_handle,"eRT.","Device.LogAgent.X_RDKCENTRAL-COM_LogLevel");

    while ( cmdChar != 'q' )
    {
        cmdChar = getchar();

        cmd_dispatch(cmdChar);
    }

#elif defined(_ANSC_LINUX)
    if ( bRunAsDaemon )
        daemonize();

    CcspTraceInfo(("\nAfter daemonize before signal\n"));

#ifdef INCLUDE_BREAKPAD
    breakpad_ExceptionHandler();
#else
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    /*signal(SIGCHLD, sig_handler);*/
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    signal(SIGSEGV, sig_handler);
    signal(SIGBUS, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGFPE, sig_handler);
    signal(SIGILL, sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGHUP, sig_handler);
#endif

    cmd_dispatch('e');

    char *subSys = NULL;
#ifdef _COSA_SIM_
    subSys = "";        /* PC simu use empty string as subsystem */
#else
    subSys = NULL;      /* use default sub-system */
#endif

    int err;
    err = Cdm_Init(bus_handle, subSys, NULL, NULL, pComponentName);
    if (err != CCSP_SUCCESS)
    {
        fprintf(stderr, "Cdm_Init: %s\n", Cdm_StrError(err));
        exit(1);
    }

#ifdef LEDMGR_WEBCONFIG
    retStatus = LedMgr_Rbus_Init();
    if(retStatus != ANSC_STATUS_SUCCESS)
    {
        CcspTraceError(("%s %d - Rbus Init failed !\n", __FUNCTION__, __LINE__ ));
        return retStatus;
    }
#endif

    ledmgr_start();

    if ( bRunAsDaemon )
    {
        while(1)
        {
            sleep(30);
        }
    }
    else
    {
        while ( cmdChar != 'q' )
        {
            cmdChar = getchar();

            cmd_dispatch(cmdChar);
        }
    }
    
#endif
    return SUCCESS;
}
