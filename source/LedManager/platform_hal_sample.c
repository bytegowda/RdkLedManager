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


/* This file contains the sample hal functions
 * to be implemented for RdkLedManager
 * Sample json file - led_config_sample.json
*/

#include "platform_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SAMPLE_LED_CONF_FILE_LOC "/usr/rdk/rdkledmanager/led_config_sample.json"
#define MAX_CONFIG_PATH_LEN 128

/* platfom_hal_initLed() function */
/**
* @description Initialises HAL layer and return file pointer to config file.
*
* @param[out] config_file_name - Vendor specific value.
*                             \n Buffer to hold the config file name including the full path. eg: /usr/rdk/rdkledmanager/led_config_gb.json.
*                             \n The buffer size should be at least 128 bytes long.
*
* @return The status of the operation.
* @retval RETURN_OK if successful.
* @retval RETURN_ERR if any error is detected.
*/
int platform_hal_initLed(char * config_file_name)
{
    if(config_file_name != NULL)
    {
        strncpy(config_file_name, SAMPLE_LED_CONF_FILE_LOC, MAX_CONFIG_PATH_LEN);
    }
    else
    {
        return -1;
    }
    return 0;
}

/* platform_hal_setLed() function */
/**
* @description Set Led behavior of the device.
*
* @param[in] pValue - The Current Led Buffer to be populated.
* <pre>
*                _LEDMGMT_PARAMS is a structure with following members :
*
*                LedColor                    - LedColor is platform dependent.
*                                              LedColor can hold  any values from enum LED_COLOR.
*
*                State                       - 0 for Solid, 1 for Blink.
*
*                Interval                    - In seconds. Range is from 0 to n.
* </pre>
*
* @return The status of the operation.
* @retval RETURN_OK if successful.
* @retval RETURN_ERR if any error is detected.
*
* @sideeffect None.
*/
int platform_hal_setLed(PLEDMGMT_PARAMS pValue)
{
    return 0;
}

/* platform_hal_getLed() function */
/**
* @description Get Led behavior of the device at time of call.
*
* @param[out] pValue - The Current Led Buffer to be populated.
* <pre>
*                _LEDMGMT_PARAMS is a structure with following members :
*
*                LedColor                    - LedColor is platform dependent.
*                                              LedColor can hold  any values from enum LED_COLOR.
*
*                State                       - 0 for Solid, 1 for Blink.
*
*                Interval                    - In seconds. Range is from 0 to n.
* </pre>
*
* @return The status of the operation.
* @retval RETURN_OK if successful.
* @retval RETURN_ERR if any error is detected.
*
* @sideeffect None
*/
int platform_hal_getLed(PLEDMGMT_PARAMS pValue)
{
    return 0;
}
