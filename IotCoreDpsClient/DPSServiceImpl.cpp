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
#include "..\SharedUtilities\DMException.h"
#include "..\SharedUtilities\Logger.h"


#include "azure_c_shared_utility\umock_c_prod.h"
#include "azure_c_shared_utility\threadapi.h"
#include "azure_hub_modules\iothub_drs_client.h"
#include "azure_hub_modules\iothub_device_auth.h"

#include "azure_c_shared_utility\macro_utils.h"
#include "azure_c_shared_utility\buffer_.h"
#include "azure_hub_modules\secure_device_factory.h"
#include "azure_c_shared_utility/platform.h"


extern void xlogging_set_log_function(LOGGER_LOG log_function);
void LoggingForDpsSdk(LOG_CATEGORY log_category, const char* file, const char* func, const int line, unsigned int /*options*/, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	std::string fmt("");
	switch (log_category)
	{
	case AZ_LOG_INFO:
		fmt = "Info: ";
		break;
	case AZ_LOG_ERROR:
		{
			int size = _scprintf("Error: File:%s Func:%s Line:%d ", file, func, line);
			std::vector<char> message(size + 1, '\0');
			sprintf_s(message.data(), message.size(), "Error: File:%s Func:%s Line:%d ", file, func, line);
			fmt = message.data();
		}
		break;
	default:
		break;
	}
	fmt += format;

	int size = _vscprintf(fmt.data(), args);
	std::vector<char> message(size + 1, '\0');
	vsprintf_s(message.data(), message.size(), fmt.data(), args);
	TRACE(message.data());

	va_end(args);
}


#define DPS_SUCCESS 1
#define DPS_FAILURE -1
#define DPS_RUNNING 0

typedef struct DPS_CLIENT_SAMPLE_INFO_TAG
{
    unsigned int sleep_time;
    char* iothub_uri;
	char* access_key_name;
	char* device_key;
	char* device_id;
	unsigned int slot;
	int registration_complete;
} DPS_CLIENT_SAMPLE_INFO;

DPS_CLIENT_SAMPLE_INFO dps_info;
DPS_SECURE_DEVICE_HANDLE dps_sec;

DEFINE_ENUM_STRINGS(DRS_ERROR, DRS_ERROR_VALUES);
DEFINE_ENUM_STRINGS(DRS_REGISTRATION_STATUS, DRS_REGISTRATION_STATUS_VALUES);

void ResetDrs(unsigned int slot)
{
	TRACE(__FUNCTION__);

	//   limpet <slot> -DUR
	TRACE("call limpet <slot> -EHK");
	EvictHmacKey(slot);
	//   limpet <slot> -DUR
	TRACE("call limpet <slot> -DUR");
	DestroyServiceUrl(slot);
}

void ResetDps0()
{
	TRACE(__FUNCTION__);

	DestroyServiceUrl(0);
}

static void on_drs_error_callback(DRS_ERROR error_type, void* user_context)
{
	TRACE(__FUNCTION__);

	if (user_context == NULL)
	{
		TRACE("user_context is NULL");
	}
	else
	{
		DPS_CLIENT_SAMPLE_INFO* err_drs_info = (DPS_CLIENT_SAMPLE_INFO*)user_context;
		TRACEP("Failure encountered in DRS info: ", ENUM_TO_STRING(DRS_ERROR, error_type));
		err_drs_info->registration_complete = DPS_FAILURE;
	}

}

static void drs_registation_status(DRS_REGISTRATION_STATUS reg_status, void* user_context)
{
    if (user_context == NULL)
    {
        TRACE("user_context is NULL");
    }
    else
    {
        DPS_CLIENT_SAMPLE_INFO* local_drs_info = (DPS_CLIENT_SAMPLE_INFO*)user_context;

        TRACEP("DPS Status: ", ENUM_TO_STRING(DRS_REGISTRATION_STATUS, reg_status));
        if (reg_status == DRS_REGISTRATION_STATUS_CONNECTED)
        {
            // Slow down the query of the device until after public preview
            local_drs_info->sleep_time = 600;
        }
        else if (reg_status == DRS_REGISTRATION_STATUS_REGISTERING)
        {
            // Slow down the query of the device  until after public preview
            local_drs_info->sleep_time = 900;
        }
        else if (reg_status == DRS_REGISTRATION_STATUS_ASSIGNING)
        {
            // Slow down the query of the device  until after public preview
            local_drs_info->sleep_time = 1200;
        }
    }
}

static void iothub_drs_register_device(IOTHUB_DRS_RESULT register_result, const char* iothub_uri, const char* device_id, void* user_context)
{
	TRACE(__FUNCTION__);

	DPS_CLIENT_SAMPLE_INFO* reg_drs_info = (DPS_CLIENT_SAMPLE_INFO*)user_context;
	if (reg_drs_info != NULL && register_result == IOTHUB_DRS_OK)
	{
		std::string fullUri = iothub_uri;
		fullUri += "/";
		fullUri += device_id;

		// Store connection string in TPM:
		//   limpet <slot> -SUR <uri>/<deviceId>
		TRACEP("call limpet <slot> -SUR: ", fullUri.c_str());
		StoreServiceUrl(reg_drs_info->slot, fullUri.c_str());
		reg_drs_info->registration_complete = DPS_SUCCESS;
	}
}

void DoDpsWork()
{
    TRACE(__FUNCTION__);
 
	xlogging_set_log_function(&LoggingForDpsSdk);

	// Query the TPM for connection string (assume slot is 0)
	std::string emptyToken = "<SASToken/>";
	std::string serviceUrl = "";
	
	try {
		serviceUrl = GetServiceUrl(0);
	}
	catch (DMException dme)
	{
		TRACEP("Failed to get ServiceUrl: ", dme.what());
		serviceUrl = emptyToken;
		TRACEP("Setting ServiceUrl: ", serviceUrl.c_str());
		TRACEP("        emptyToken: ", emptyToken.c_str());
	}

    auto it = std::search(
        serviceUrl.begin(), serviceUrl.end(),
        emptyToken.begin(), emptyToken.end());
    if (it != serviceUrl.end())
    {
		// If connection string is not present, query 
		// azure device registration service for it
		TRACE("No SAS token found in TPM, query DRS");

		// Wait for an internet connection to be
		// established
		DWORD result;
		while (!InternetGetConnectedState(&result, 0))
		{
			TRACE("No internet connection found ... wait 5 seconds and check again.");
			ThreadAPI_Sleep(5000);
		}

		memset(&dps_info, 0, sizeof(DPS_CLIENT_SAMPLE_INFO));
		dps_info.registration_complete = DPS_RUNNING;
        dps_info.sleep_time = 10;

		std::wstring wdrs_slot;
		if (ERROR_SUCCESS != Utils::TryReadRegistryValue(L"System\\CurrentControlSet\\Control\\Wininit", L"drs_slot", wdrs_slot))
		{
			TRACE("tpm slot not found in registry.");
			wdrs_slot = L"0";
		}
		TRACEP(L"tpm slot: ", wdrs_slot.c_str());
		dps_info.slot = _wtoi(wdrs_slot.c_str());

		std::wstring wdrs_uri =
			Utils::ReadRegistryValue(L"System\\CurrentControlSet\\Control\\Wininit", L"drs_uri");
		TRACEP(L"uri from registry: ", wdrs_uri.c_str());

        std::string drs_uri = Utils::WideToMultibyte(wdrs_uri.c_str());
		TRACEP("uri to char: ", drs_uri.data());

        std::wstring wdrs_scope_id =
            Utils::ReadRegistryValue(L"System\\CurrentControlSet\\Control\\Wininit", L"drs_scope");
        TRACEP(L"scope from registry: ", wdrs_scope_id.c_str());

        std::string drs_scope_id = Utils::WideToMultibyte(wdrs_scope_id.c_str());
        TRACEP("scope to char: ", drs_scope_id.data());

        if (platform_init() != 0)
		{
			TRACE("Failed calling platform_init");
		}

		ResetDrs(dps_info.slot);

		do
		{
			TRACE("Start registration process");
			dps_info.registration_complete = DPS_RUNNING;

			IOTHUB_DRS_LL_HANDLE handle;
            if ((handle = IoTHub_DRS_LL_Create(drs_uri.data(), drs_scope_id.data(), DPS_PROTOCOL_TYPE_HTTP, on_drs_error_callback, &dps_info)) == NULL)
			{
				TRACE("failed calling IoTHub_DRS_LL_Create");
				return;
			}
            if (IoTHub_DRS_LL_Register_Device(handle, iothub_drs_register_device, &dps_info, drs_registation_status, &dps_info) != IOTHUB_DRS_OK)
			{
				TRACE("failed calling IoTHub_DRS_LL_Register_Device");
				return;
			}

			do
			{
				IoTHub_DRS_LL_DoWork(handle);
                ThreadAPI_Sleep(dps_info.sleep_time);
			} while (DPS_RUNNING == dps_info.registration_complete);

			IoTHub_DRS_LL_Destroy(handle);

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

