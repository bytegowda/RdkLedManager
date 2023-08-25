// /*
//  * If not stated otherwise in this file or this component's Licenses.txt file the
//  * following copyright and licenses apply:
//  *
//  * Copyright 2018 RDK Management
//  *
//  * Licensed under the Apache License, Version 2.0 (the "License");
//  * you may not use this file except in compliance with the License.
//  * You may obtain a copy of the License at
//  *
//  * http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
// */

// //local includes
// #include "ledmgr_dml_apis.h"
// #include "ledmgr_dml.h"

// //global includes
// // #include "ansc_platform.h"
// // #include "syslog.h"
// // #include "ccsp_trace.h"
// // // #include "safec_lib_common.h"
// // #include <msgpack.h>
// // #include "helpers.h"
// // #include "base64.h"
// #include <syscfg/syscfg.h>

// #define DEBUG_INI_NAME "/etc/debug.ini"

// extern LED_DATA* g_pLedMgrBE;

// /***********************************************************************

//  APIs for Object:

//     X_RDK_LedManager.

//     *  LedManager_GetParamStringValue
//     *  LedManager_SetParamStringValue

// ***********************************************************************/
// LONG LedManager_GetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pValue, ULONG* pulSize)
// {
//     LONG ret = -1;

//     if( AnscEqualString(ParamName, "Data", TRUE) )
//     {
//         /* Data value should be empty for all get */
//         snprintf(pValue, pulSize, "%s", "");
//         ret = 0;
//     }

//     return ret;
// }

// BOOL LedManager_SetParamStringValue(ANSC_HANDLE hInsContext, char* ParamName, char* pString)
// {
//     BOOL ret = FALSE;

//     if(pLedConfigData != NULL)
//     {
//         DML_LEDMGR_CONFIG* pLedDmlData = &(pLedConfigData->data);

//         if (strcmp(ParamName, "Data") == 0)
//         {
//             char *webConf = NULL;
//             int webSize = 0;

//             webConf = AnscBase64Decode(pString, &webSize);
//             if(!webConf)
//             {
//                 CcspTraceError(("%s: Failed to decode webconfig blob..\n",__FUNCTION__));
//                 LedMgrDml_GetConfigData_release(pLedConfigData);
//                 return ret;
//             }
//             if ( ANSC_STATUS_SUCCESS == LedMgrDmlLedDataSet(webConf,webSize) )
//             {
//                 CcspTraceInfo(("%s Success in parsing web config blob..\n",__FUNCTION__));
//                 ret = TRUE;
//             }
//             else
//             {
//                 CcspTraceError(("%s Failed to parse webconfig blob..\n",__FUNCTION__));
//             }
//             AnscFreeMemory(webConf);
//         }

//         else if( AnscEqualString(ParamName, "LedFailoverData", TRUE) )
//         {
//             char *webConf = NULL;
//             int webSize = 0;

//             webConf = AnscBase64Decode(pString, &webSize);
//             if(!webConf)
//             {
//                 CcspTraceError(("%s: Failed to decode webconfig blob..\n",__FUNCTION__));
//                 LedMgrDml_GetConfigData_release(pLedConfigData);
//                 return ret;
//             }
//             if ( ANSC_STATUS_SUCCESS == LedMgrDmlLedFailOverDataSet(webConf,webSize) )
//             {
//                 CcspTraceInfo(("%s Success in parsing web config blob..\n",__FUNCTION__));
//                 ret = TRUE;
//             }
//             else
//             {
//                 CcspTraceError(("%s Failed to parse webconfig blob..\n",__FUNCTION__));
//             }
//             AnscFreeMemory(webConf);

//         }
//     }

//     return ret;
// }
