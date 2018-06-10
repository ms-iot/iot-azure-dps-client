/*
Copyright 2017 Microsoft
Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "stdafx.h"
#include <filesystem>
#include <fstream>
#include <assert.h>
#include "DPSService.h"
#include "TpmSupport.h"


// #include "iothub_client.h"
// #include "iothub_message.h"
// #include "iothub_client_version.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/http_proxy_io.h"
#include "azure_c_shared_utility/xlogging.h"

// #include "iothub_client_ll.h"
#include "azure_prov_client/prov_device_ll_client.h"
#include "azure_prov_client/prov_security_factory.h"

// #include "iothubtransportmqtt.h"
#include "azure_prov_client/prov_transport_http_client.h"

// 
// This tpm_slot registry key/value pair needs to stay in sync with the one found here: 
//    https://github.com/ms-iot/iot-core-azure-dm-client/blob/develop/src/SystemConfigurator/CommandProcessor.cpp
//

#define IOTDPSCLIENT_PARAMETERS_REGPATH             L"system\\currentcontrolset\\services\\iotdpsclient\\parameters"
#define IOTDPSCLIENT_PARAMETERS_REGNAME_DPSURI      L"dps_uri"
#define IOTDPSCLIENT_PARAMETERS_REGNAME_DPSSCOPE    L"dps_scope"
#define IOTDPSCLIENT_PARAMETERS_REGNAME_TPMSLOT     L"tpm_slot"

using namespace std;

extern void xlogging_set_log_function(LOGGER_LOG log_function);
void LoggingForDpsSdk(LOG_CATEGORY log_category, const char* file, const char* func, const int line, unsigned int /*options*/, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    string fmt("");
    switch (log_category)
    {
    case AZ_LOG_INFO:
        fmt = "Info: ";
        break;
    case AZ_LOG_ERROR:
    {
        int size = _scprintf("Error: File:%s Func:%s Line:%d ", file, func, line);
        vector<char> message(size + 1, '\0');
        sprintf_s(message.data(), message.size(), "Error: File:%s Func:%s Line:%d ", file, func, line);
        fmt = message.data();
    }
    break;
    default:
        break;
    }
    fmt += format;

    int size = _vscprintf(fmt.data(), args);
    vector<char> message(size + 1, '\0');
    vsprintf_s(message.data(), message.size(), fmt.data(), args);
    TRACE(message.data());

    va_end(args);
}


#define DPS_SUCCESS 1
#define DPS_FAILURE -1
#define DPS_RUNNING 0

typedef struct CLIENT_SAMPLE_INFO_TAG
{
    unsigned int sleep_time;
    char* iothub_uri;
    char* access_key_name;
    char* device_key;
    char* device_id;
    int registration_complete;
    int slot;
} CLIENT_SAMPLE_INFO;

CLIENT_SAMPLE_INFO dps_info;

// DEFINE_ENUM_STRINGS(DPS_ERROR, DPS_ERROR_VALUES);
DEFINE_ENUM_STRINGS(PROV_DEVICE_RESULT, PROV_DEVICE_RESULT_VALUE);
DEFINE_ENUM_STRINGS(PROV_DEVICE_REG_STATUS, PROV_DEVICE_REG_STATUS_VALUES);

int GetDpsTpmSlot()
{
    TRACE(__FUNCTION__);

    wstring wdps_tpm_slot;
    if (ERROR_SUCCESS != Utils::TryReadRegistryValue(IOTDPSCLIENT_PARAMETERS_REGPATH, IOTDPSCLIENT_PARAMETERS_REGNAME_TPMSLOT, wdps_tpm_slot))
    {
        TRACE("tpm slot not found in registry.");
        wdps_tpm_slot = L"0";
    }
    TRACEP(L"tpm slot: ", wdps_tpm_slot.c_str());

    return _wtoi(wdps_tpm_slot.c_str());
}

void ResetDps()
{
    TRACE(__FUNCTION__);

    int dps_tpm_slot = GetDpsTpmSlot();

    //   limpet <slot> -EHK
    TRACE("call limpet <slot> -EHK");
    EvictHmacKey(dps_tpm_slot);
    //   limpet <slot> -DUR
    TRACE("call limpet <slot> -DUR");
    DestroyServiceUrl(dps_tpm_slot);
}

static void registation_status_callback(PROV_DEVICE_REG_STATUS reg_status, void* user_context)
{
    if (user_context == NULL)
    {
        TRACE("user_context is NULL");
    }
    else
    {
        CLIENT_SAMPLE_INFO* local_dps_info = (CLIENT_SAMPLE_INFO*)user_context;

        TRACEP("Provisioning Status: ", ENUM_TO_STRING(PROV_DEVICE_REG_STATUS, reg_status));
        if (reg_status == PROV_DEVICE_REG_STATUS_CONNECTED)
        {
            local_dps_info->sleep_time = 600;
        }
        else if (reg_status == PROV_DEVICE_REG_STATUS_REGISTERING)
        {
            local_dps_info->sleep_time = 900;
        }
        else if (reg_status == PROV_DEVICE_REG_STATUS_ASSIGNING)
        {
            local_dps_info->sleep_time = 1200;
        }
    }
}

static void register_device_callback(PROV_DEVICE_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
    TRACE(__FUNCTION__);

    if (user_context == NULL)
    {
        printf("user_context is NULL\r\n");
    }
    else
    {
        CLIENT_SAMPLE_INFO* reg_dps_info = (CLIENT_SAMPLE_INFO*)user_context;
        if (register_result == PROV_DEVICE_RESULT_OK)
        {
            string fullUri = iothub_uri;
            fullUri += "/";
            fullUri += device_id;

            // Store connection string in TPM:
            //   limpet <slot> -SUR <uri>/<deviceId>
            TRACEP("call limpet <slot> -SUR: ", fullUri.c_str());
            StoreServiceUrl(reg_dps_info->slot, fullUri.c_str());
            reg_dps_info->registration_complete = DPS_SUCCESS;
        }
        else
        {
            TRACEP("Failure encountered on registration: %s\r\n", ENUM_TO_STRING(PROV_DEVICE_RESULT, register_result));
            reg_dps_info->registration_complete = DPS_FAILURE;
        }
    }
}

void DoDpsWork()
{
    TRACE(__FUNCTION__);

    xlogging_set_log_function(&LoggingForDpsSdk);

    // Query the service url
    string serviceUrl = "";

    try {
        serviceUrl = GetServiceUrl(GetDpsTpmSlot());
    }
    catch (const IotException& dme)
    {
        TRACEP("Failed to get ServiceUrl: ", dme.what());
        TRACEP("Setting ServiceUrl: ", serviceUrl.c_str());
    }

    if (serviceUrl.empty())
    {
        // If connection string is not present, query 
        // azure device provisioning service for it
        TRACE("Service URL not found in TPM, query DPS");

        memset(&dps_info, 0, sizeof(CLIENT_SAMPLE_INFO));

        dps_info.slot = GetDpsTpmSlot();

        wstring wdps_uri =
            Utils::ReadRegistryValue(IOTDPSCLIENT_PARAMETERS_REGPATH, IOTDPSCLIENT_PARAMETERS_REGNAME_DPSURI);
        TRACEP(L"uri from registry: ", wdps_uri.c_str());

        string dps_uri = Utils::WideToMultibyte(wdps_uri.c_str());
        TRACEP("uri to char: ", dps_uri.data());

        wstring wdps_scope_id =
            Utils::ReadRegistryValue(IOTDPSCLIENT_PARAMETERS_REGPATH, IOTDPSCLIENT_PARAMETERS_REGNAME_DPSSCOPE);
        TRACEP(L"scope from registry: ", wdps_scope_id.c_str());

        string dps_scope_id = Utils::WideToMultibyte(wdps_scope_id.c_str());
        TRACEP("scope to char: ", dps_scope_id.data());

        SECURE_DEVICE_TYPE hsm_type = SECURE_DEVICE_TYPE_TPM;

        if (platform_init() != 0 && prov_dev_security_init(hsm_type) != 0)
        {
            TRACE("Failed calling platform_init");
        }

        

        ResetDps();

        do
        {
            TRACE("Start registration process");
            dps_info.registration_complete = DPS_RUNNING;
            dps_info.sleep_time = 10;

            // Wait for an internet connection to be established
            DWORD result;
            while (!InternetGetConnectedState(&result, 0))
            {
                TRACE("No internet connection found ... wait 5 seconds and check again.");
                ThreadAPI_Sleep(5000);
            }

            PROV_DEVICE_LL_HANDLE handle;
            if ((handle = Prov_Device_LL_Create(dps_uri.data(), dps_scope_id.data(), Prov_Device_HTTP_Protocol)) == NULL)
            {
                TRACE("failed calling DPS_LL_Create");
                return;
            }

            if (Prov_Device_LL_Register_Device(handle, register_device_callback, &dps_info, registation_status_callback, &dps_info) != PROV_DEVICE_RESULT_OK)
            {
                TRACE("failed calling Prov_Device_LL_Register_Device");
                return;
            }

            do
            {
                Prov_Device_LL_DoWork(handle);
                ThreadAPI_Sleep(dps_info.sleep_time);
            } while (DPS_RUNNING == dps_info.registration_complete);

            Prov_Device_LL_Destroy(handle);

            if (DPS_SUCCESS == dps_info.registration_complete)
            {
                break;
            }
            else
            {
                ThreadAPI_Sleep(5000);
            }

            TRACE("Registration failed, retry");

        } while (true);
    }

    TRACE("Exiting DoDpsWork");
}
