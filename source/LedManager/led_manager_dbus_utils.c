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
#include "led_manager_global.h"
 
ANSC_HANDLE bus_handle = NULL; 
char g_Subsystem[32] = {0};
PCOMPONENT_COMMON_LED_MANAGER       g_pComponentCommonLedMgr = NULL;  
PCCSP_CCD_INTERFACE pSsdCcdIf = (PCCSP_CCD_INTERFACE) NULL;
PDSLH_LCB_INTERFACE pDslhLcbIf = (PDSLH_LCB_INTERFACE) NULL;
PDSLH_CPE_CONTROLLER_OBJECT pDslhCpeController = NULL;
extern ANSC_HANDLE g_MessageBusHandle_Irep;
extern char g_SubSysPrefix_Irep[32];

ANSC_STATUS ssp_create()
{
    int rc = ANSC_STATUS_FAILURE;

    g_pComponentCommonLedMgr = (PCOMPONENT_COMMON_LED_MANAGER) AnscAllocateMemory(sizeof(COMPONENT_COMMON_LED_MANAGER));

    if(!g_pComponentCommonLedMgr)
    {
        return ANSC_STATUS_RESOURCES;
    }

    ComponentCommonDmInit(g_pComponentCommonLedMgr);

    g_pComponentCommonLedMgr->Name    = AnscCloneString(COMPONENT_NAME_LEDMANAGER);
    g_pComponentCommonLedMgr->Version = 1;
    g_pComponentCommonLedMgr->Author  = AnscCloneString("Your Name");

    /* Create ComponentCommonDatamodel interface*/
    if (!pSsdCcdIf)
    {
        pSsdCcdIf = (PCCSP_CCD_INTERFACE) AnscAllocateMemory(sizeof(CCSP_CCD_INTERFACE));

        if ( !pSsdCcdIf )
        {
            return ANSC_STATUS_RESOURCES;
        }
        else
        {
            AnscCopyString(pSsdCcdIf->Name, CCSP_CCD_INTERFACE_NAME);

            pSsdCcdIf->InterfaceId              = CCSP_CCD_INTERFACE_ID;
            pSsdCcdIf->hOwnerContext            = NULL;
            pSsdCcdIf->Size                     = sizeof(CCSP_CCD_INTERFACE);

            pSsdCcdIf->GetComponentName         = ssp_CcdIfGetComponentName;
            pSsdCcdIf->GetComponentVersion      = ssp_CcdIfGetComponentVersion;
            pSsdCcdIf->GetComponentAuthor       = ssp_CcdIfGetComponentAuthor;
            pSsdCcdIf->GetComponentHealth       = ssp_CcdIfGetComponentHealth;
            pSsdCcdIf->GetComponentState        = ssp_CcdIfGetComponentState;
            pSsdCcdIf->GetLoggingEnabled        = ssp_CcdIfGetLoggingEnabled;
            pSsdCcdIf->SetLoggingEnabled        = ssp_CcdIfSetLoggingEnabled;
            pSsdCcdIf->GetLoggingLevel          = ssp_CcdIfGetLoggingLevel;
            pSsdCcdIf->SetLoggingLevel          = ssp_CcdIfSetLoggingLevel;
            pSsdCcdIf->GetMemMaxUsage           = ssp_CcdIfGetMemMaxUsage;
            pSsdCcdIf->GetMemMinUsage           = ssp_CcdIfGetMemMinUsage;
            pSsdCcdIf->GetMemConsumed           = ssp_CcdIfGetMemConsumed;
            pSsdCcdIf->ApplyChanges             = ssp_CcdIfApplyChanges;
        }
    }
    /* Create ComponentCommonDatamodel interface*/
    if (!pDslhLcbIf)
    {
        pDslhLcbIf = (PDSLH_LCB_INTERFACE) AnscAllocateMemory(sizeof(DSLH_LCB_INTERFACE));

        if (!pDslhLcbIf)
        {
            return ANSC_STATUS_RESOURCES;
        }
        else
        {
            AnscCopyString(pDslhLcbIf->Name, CCSP_LIBCBK_INTERFACE_NAME);

            pDslhLcbIf->InterfaceId              = CCSP_LIBCBK_INTERFACE_ID;
            pDslhLcbIf->hOwnerContext            = NULL;
            pDslhLcbIf->Size                     = sizeof(DSLH_LCB_INTERFACE);
            pDslhLcbIf->InitLibrary              = NULL;
        }
    }

    pDslhCpeController = DslhCreateCpeController(NULL, NULL, NULL);

    if (!pDslhCpeController)
    {
        CcspTraceWarning(("CANNOT Create pDslhCpeController... Exit!\n"));
        return ANSC_STATUS_RESOURCES;
    }

    CcspTraceInfo(("%s %d: DslhCreateCpeController() returned SUCCESS\n", __FUNCTION__, __LINE__));

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
ssp_engage
    (
    )
{
    ANSC_STATUS             returnStatus                = ANSC_STATUS_SUCCESS;
    PCCC_MBI_INTERFACE              pSsdMbiIf                   = (PCCC_MBI_INTERFACE)MsgHelper_CreateCcdMbiIf((void*)bus_handle, g_Subsystem);
    char                            CrName[256];

    g_pComponentCommonLedMgr->Health = RDK_COMMON_COMPONENT_HEALTH_Yellow;

    /* data model configuration */
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pDslhLcbIf);
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pSsdMbiIf);
    pDslhCpeController->AddInterface((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)pSsdCcdIf);
    pDslhCpeController->SetDbusHandle((ANSC_HANDLE)pDslhCpeController, (ANSC_HANDLE)bus_handle);
    pDslhCpeController->Engage((ANSC_HANDLE)pDslhCpeController);

    if ( g_Subsystem[0] != 0 )
    {
        sprintf(CrName, "%s%s", g_Subsystem, CCSP_DBUS_INTERFACE_CR);
    }
    else
    {
        sprintf(CrName, "%s", CCSP_DBUS_INTERFACE_CR);
    }

    returnStatus =
        pDslhCpeController->RegisterCcspDataModel
        (
         (ANSC_HANDLE)pDslhCpeController,
         CrName, /* CCSP_DBUS_INTERFACE_CR,*/              /* CCSP CR ID */
         NULL,             /* Data Model XML file. Can be empty if only base data model supported. */
         COMPONENT_NAME_LEDMANAGER,            /* Component Name    */
         COMPONENT_VERSION_LEDMANAGER,         /* Component Version */
         COMPONENT_PATH_LEDMANAGER,            /* Component Path    */
         g_Subsystem /* Component Prefix  */
        );

    if ( returnStatus == ANSC_STATUS_SUCCESS || returnStatus == CCSP_SUCCESS)
    {
        /* System is fully initialized */
        g_pComponentCommonLedMgr->Health = RDK_COMMON_COMPONENT_HEALTH_Green;
    }

    CcspTraceInfo(("%s %d: XML registration SUCCESS\n", __FUNCTION__, __LINE__));

    return ANSC_STATUS_SUCCESS;
}

ANSC_STATUS
ssp_cancel
    (
    )
{
    int                             nRet  = 0;
    char                            CrName[256];
    char                            CpName[256];

    if(  g_pComponentCommonLedMgr == NULL)
    {
        return ANSC_STATUS_SUCCESS;
    }

    if ( g_Subsystem[0] != 0 )
    {
        sprintf(CrName, "%s%s", g_Subsystem, CCSP_DBUS_INTERFACE_CR);
        sprintf(CpName, "%s%s", g_Subsystem, COMPONENT_NAME_LEDMANAGER);
    }
    else
    {
        sprintf(CrName, "%s", CCSP_DBUS_INTERFACE_CR);
        sprintf(CpName, "%s", COMPONENT_NAME_LEDMANAGER);
    }
    /* unregister component */
    nRet = CcspBaseIf_unregisterComponent(bus_handle, CrName, CpName );
    AnscTrace("unregisterComponent returns %d\n", nRet);

    pDslhCpeController->Cancel((ANSC_HANDLE)pDslhCpeController);
    AnscFreeMemory(pDslhCpeController);

    if ( pSsdCcdIf ) AnscFreeMemory(pSsdCcdIf);
    if (  g_pComponentCommonLedMgr ) AnscFreeMemory( g_pComponentCommonLedMgr);

     g_pComponentCommonLedMgr = NULL;
    pSsdCcdIf                = NULL;
    pDslhCpeController       = NULL;

    return ANSC_STATUS_SUCCESS;
}

DBusHandlerResult
CcspComp_path_message_func(DBusConnection  *conn, DBusMessage *message, void *user_data)
{
    CCSP_MESSAGE_BUS_INFO *bus_info =(CCSP_MESSAGE_BUS_INFO *) user_data;
    const char *interface = dbus_message_get_interface(message);
    const char *method   = dbus_message_get_member(message);
    DBusMessage *reply;

    reply = dbus_message_new_method_return (message);
    if (reply == NULL)
    {
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return CcspBaseIf_base_path_message_func
               (
                   conn,
                   message,
                   reply,
                   interface,
                   method,
                   bus_info
               );
}

static void* AllocateMemory_Callback
    (
        unsigned int ulMemorySize
    )
{
    return AnscAllocateMemory(ulMemorySize);
}

int
ssp_Mbi_Initialize
    (
        void * user_data
    )
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}

int
ssp_Mbi_Finalize
    (
        void*               user_data
    )
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    returnStatus = ssp_cancel();

    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}

int
ssp_Mbi_Buscheck
    (
        void*               user_data
    )
{
    return 0;
}

int
ssp_Mbi_FreeResources
    (
        int                 priority,
        void                * user_data
    )
{
    ANSC_STATUS             returnStatus    = ANSC_STATUS_SUCCESS;

    if ( priority == COMMON_COMPONENT_FREERESOURCES_PRIORITY_Low )
    {
        /* Currently do nothing */
    }
    else if ( priority == COMMON_COMPONENT_FREERESOURCES_PRIORITY_High )
    {
        returnStatus = ssp_cancel();
    }

    return ( returnStatus == ANSC_STATUS_SUCCESS ) ? 0 : 1;
}

ANSC_STATUS
ssp_Mbi_MessageBusEngage(char* component_id, char* config_file, char* path)
{
    ANSC_STATUS                 returnStatus       = ANSC_STATUS_SUCCESS;
    CCSP_Base_Func_CB           cb                 = {0};

    if ( ! component_id || ! path )
    {
        CcspTraceError((" !!! ssp_Mbi_MessageBusEngage: component_id or path is NULL !!!\n"));
    }

    /* Connect to message bus */
    returnStatus =
        CCSP_Message_Bus_Init
        (        component_id,
                 config_file,
                 &bus_handle,
                 (CCSP_MESSAGE_BUS_MALLOC)AllocateMemory_Callback,           /* mallocfc, use default */
                 Ansc_FreeMemory_Callback                /* freefc,   use default */
        );

    if ( returnStatus != ANSC_STATUS_SUCCESS )
    {
        CcspTraceError((" !!! SSD Message Bus Init ERROR !!!\n"));

        return returnStatus;
    }

    CcspTraceInfo(("INFO: bus_handle: 0x%8x \n", bus_handle));
    g_MessageBusHandle_Irep = bus_handle;
    AnscCopyString(g_SubSysPrefix_Irep, g_Subsystem);

    CCSP_Msg_SleepInMilliSeconds(1000);

    /* Base interface implementation that will be used cross components */
    cb.getParameterValues     = CcspCcMbi_GetParameterValues;
    cb.setParameterValues     = CcspCcMbi_SetParameterValues;
    cb.setCommit              = CcspCcMbi_SetCommit;
    cb.setParameterAttributes = CcspCcMbi_SetParameterAttributes;
    cb.getParameterAttributes = CcspCcMbi_GetParameterAttributes;
    cb.AddTblRow              = CcspCcMbi_AddTblRow;
    cb.DeleteTblRow           = CcspCcMbi_DeleteTblRow;
    cb.getParameterNames      = CcspCcMbi_GetParameterNames;
    cb.currentSessionIDSignal = CcspCcMbi_CurrentSessionIdSignal;

    /* Base interface implementation that will only be used by ssd */
    cb.initialize             = ssp_Mbi_Initialize;
    cb.finalize               = ssp_Mbi_Finalize;
    cb.freeResources          = ssp_Mbi_FreeResources;
    cb.busCheck               = ssp_Mbi_Buscheck;

    CcspBaseIf_SetCallback(bus_handle, &cb);

    CcspTraceInfo(("%s %d: calling CCSP_Message_Bus_Register_Path()\n", __FUNCTION__, __LINE__));

    /* Register service callback functions */
    returnStatus =
        CCSP_Message_Bus_Register_Path
        (
         bus_handle,
         path,
         CcspComp_path_message_func,
         bus_handle
        );

    if ( returnStatus != CCSP_Message_Bus_OK )
    {
        CcspTraceError((" !!! CCSP_Message_Bus_Register_Path ERROR returnStatus: %d\n!!!\n", returnStatus));

        return returnStatus;
    }

    CcspTraceInfo(("%s %d: CCSP_Message_Bus_Register_Path() returned SUCCESS\n", __FUNCTION__, __LINE__));

    /* Register event/signal */
    returnStatus =
        CcspBaseIf_Register_Event
        (
         bus_handle,
         0,
         "currentSessionIDSignal"
        );

    if ( returnStatus != CCSP_Message_Bus_OK )
    {
        CcspTraceError((" !!! CCSP_Message_Bus_Register_Event: CurrentSessionIDSignal ERROR returnStatus: %d!!!\n", returnStatus));

        return returnStatus;
    }

    CcspTraceInfo(("%s %d: CcspBaseIf_Register_Event() returned SUCCESS\n", __FUNCTION__, __LINE__));

    return ANSC_STATUS_SUCCESS;

}

int cmd_dispatch(int  command)
{
    switch ( command )
    {
        case    'e' :
#ifdef _ANSC_LINUX
            CcspTraceInfo(("Connect to bus daemon...\n"));
            {
                char                            CName[256];

                if ( g_Subsystem[0] != 0 )
                {
                    sprintf(CName, "%s%s", g_Subsystem, COMPONENT_NAME_LEDMANAGER);
                }
                else
                {
                    sprintf(CName, "%s", COMPONENT_NAME_LEDMANAGER);
                }

                ssp_Mbi_MessageBusEngage
                    (
                     CName,
                     CCSP_MSG_BUS_CFG,
                     COMPONENT_PATH_LEDMANAGER
                    );
            }
#endif
            ssp_create();
            ssp_engage();
            break;

        case    'm':
            AnscPrintComponentMemoryTable(pComponentName);
            break;

        case    't':
            AnscTraceMemoryTable();
            break;

        case    'c':
            ssp_cancel();
            break;

        default:
            break;
    }

    return 0;
}

#if defined(_ANSC_LINUX)
void daemonize(void)
{
    int fd;
    switch (fork()) {
        case 0:
            break;
        case -1:
            CcspTraceInfo(("Error daemonizing (fork)! %d - %s\n", errno, strerror(
                            errno)));
            exit(0);
            break;
        default:
            _exit(0);
    }

    if (setsid() < 0)
    {
        CcspTraceInfo(("Error demonizing (setsid)! %d - %s\n", errno, strerror(errno)));
        exit(0);
    }

#ifndef  _DEBUG
    fd = open("/dev/null", O_RDONLY);
    if (fd != 0) {
        dup2(fd, 0);
        close(fd);
    }
    fd = open("/dev/null", O_WRONLY);
    if (fd != 1) {
        dup2(fd, 1);
        close(fd);
    }
    fd = open("/dev/null", O_WRONLY);
    if (fd != 2) {
        dup2(fd, 2);
        close(fd);
    }
#endif
}

#endif

char*
ssp_CcdIfGetComponentName
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonLedMgr->Name;
}


ULONG
ssp_CcdIfGetComponentVersion
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonLedMgr->Version;
}


char*
ssp_CcdIfGetComponentAuthor
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonLedMgr->Author;
}


ULONG
ssp_CcdIfGetComponentHealth
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonLedMgr->Health;
}

ULONG
ssp_CcdIfGetComponentState
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonLedMgr->State;
}


BOOL
ssp_CcdIfGetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonLedMgr->LogEnable;
}



ANSC_STATUS
ssp_CcdIfSetLoggingEnabled
    (
        ANSC_HANDLE                     hThisObject,
        BOOL                            bEnabled
    )
{
    if(g_pComponentCommonLedMgr->LogEnable == bEnabled)
    {
        return ANSC_STATUS_SUCCESS;
    }

    g_pComponentCommonLedMgr->LogEnable = bEnabled;

    if(bEnabled)
    {
        g_iTraceLevel = (INT) g_pComponentCommonLedMgr->LogLevel;
    }
    else
    {
        g_iTraceLevel = CCSP_TRACE_INVALID_LEVEL;
    }
    return ANSC_STATUS_SUCCESS;
}


ULONG
ssp_CcdIfGetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonLedMgr->LogLevel;
}

ANSC_STATUS
ssp_CcdIfSetLoggingLevel
    (
        ANSC_HANDLE                     hThisObject,
        ULONG                           LogLevel
    )
{
    if(g_pComponentCommonLedMgr->LogLevel == LogLevel)
    {
        return ANSC_STATUS_SUCCESS;
    }

    g_pComponentCommonLedMgr->LogLevel = LogLevel;

    if(g_pComponentCommonLedMgr->LogEnable)
    {
        g_iTraceLevel = (INT) g_pComponentCommonLedMgr->LogLevel;
    }

    return ANSC_STATUS_SUCCESS;
}

ULONG
ssp_CcdIfGetMemMaxUsage
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return g_ulAllocatedSizePeak;
}


ULONG
ssp_CcdIfGetMemMinUsage
    (
        ANSC_HANDLE                     hThisObject
    )
{
    return  g_pComponentCommonLedMgr->MemMinUsage;
}

ULONG
ssp_CcdIfGetMemConsumed
    (
        ANSC_HANDLE                     hThisObject
    )
{
    LONG             size = 0;

    size = AnscGetComponentMemorySize(COMPONENT_NAME_LEDMANAGER);
    if (size == -1 )
        size = 0;

    return size;
}


ANSC_STATUS
ssp_CcdIfApplyChanges
    (
        ANSC_HANDLE                     hThisObject
    )
{
    ANSC_STATUS                         returnStatus    = ANSC_STATUS_SUCCESS;
    /* Assume the parameter settings are committed immediately. */
    /* AnscSetTraceLevel((INT) g_pComponentCommonLedMgr->LogLevel); */
    return returnStatus;
}

