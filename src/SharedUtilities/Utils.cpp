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
#pragma once

#include <stdafx.h>
#include "Utils.h"

#define ERROR_PIPE_HAS_BEEN_ENDED 109

using namespace std;

namespace Utils
{
    void LaunchProcess(const wstring& commandString, unsigned long& returnCode, string& output)
    {
        TRACEP(L"Launching: ", commandString.c_str());

        SECURITY_ATTRIBUTES securityAttributes;
        securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        securityAttributes.bInheritHandle = TRUE;
        securityAttributes.lpSecurityDescriptor = NULL;

        AutoCloseHandle stdOutReadHandle;
        AutoCloseHandle stdOutWriteHandle;
        DWORD pipeBufferSize = 4096;

        if (!CreatePipe(stdOutReadHandle.GetAddress(), stdOutWriteHandle.GetAddress(), &securityAttributes, pipeBufferSize))
        {
            throw IotExceptionWithErrorCode(GetLastError());
        }

        if (!SetHandleInformation(stdOutReadHandle.Get(), HANDLE_FLAG_INHERIT, 0 /*flags*/))
        {
            throw IotExceptionWithErrorCode(GetLastError());
        }

        PROCESS_INFORMATION piProcInfo;
        ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

        STARTUPINFO siStartInfo;
        ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdError = stdOutWriteHandle.Get();
        siStartInfo.hStdOutput = stdOutWriteHandle.Get();
        siStartInfo.hStdInput = NULL;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        if (!CreateProcess(NULL,
            const_cast<wchar_t*>(commandString.c_str()), // command line 
            NULL,         // process security attributes 
            NULL,         // primary thread security attributes 
            TRUE,         // handles are inherited 
            0,            // creation flags 
            NULL,         // use parent's environment 
            NULL,         // use parent's current directory 
            &siStartInfo, // STARTUPINFO pointer 
            &piProcInfo)) // receives PROCESS_INFORMATION
        {
            throw IotExceptionWithErrorCode(GetLastError());
        }
        TRACE("Child process has been launched.");

        bool doneWriting = false;
        while (!doneWriting)
        {
            // Let the child process run for 1 second, and then check if there is anything to read...
            DWORD waitStatus = WaitForSingleObject(piProcInfo.hProcess, 1000);
            if (waitStatus == WAIT_OBJECT_0)
            {
                TRACE("Child process has exited.");
                if (!GetExitCodeProcess(piProcInfo.hProcess, &returnCode))
                {
                    TRACEP("Warning: Failed to get process exist code. GetLastError() = ", GetLastError());
                    // ToDo: do we ignore?
                }
                CloseHandle(piProcInfo.hProcess);
                CloseHandle(piProcInfo.hThread);

                // Child process has exited, no more writing will take place.
                // Without closing the write channel, the ReadFile will keep waiting.
                doneWriting = true;
                stdOutWriteHandle.Close();
            }
            else
            {
                TRACE("Child process is still running...");
            }

            DWORD bytesAvailable = 0;
            if (PeekNamedPipe(stdOutReadHandle.Get(), NULL, 0, NULL, &bytesAvailable, NULL))
            {
                if (bytesAvailable > 0)
                {
                    DWORD readByteCount = 0;
                    vector<char> readBuffer(bytesAvailable + 1);
                    if (ReadFile(stdOutReadHandle.Get(), readBuffer.data(), static_cast<DWORD>(readBuffer.size() - 1), &readByteCount, NULL) || readByteCount == 0)
                    {
                        readBuffer[readByteCount] = '\0';
                        output += readBuffer.data();
                    }
                }
            }
            else
            {
                DWORD retCode = GetLastError();
                if (ERROR_PIPE_HAS_BEEN_ENDED != retCode)
                {
                    printf("error code = %d\n", retCode);
                }
                break;
            }
        }

        TRACEP("Command return Code: ", returnCode);
        TRACEP("Command output : ", output.c_str());
    }

    wstring MultibyteToWide(const char* s)
    {
        size_t length = s ? strlen(s) : 0;
        size_t requiredCharCount = MultiByteToWideChar(CP_UTF8, 0, s, static_cast<int>(length), nullptr, 0);

        // add room for \0
        ++requiredCharCount;

        vector<wchar_t> wideString(requiredCharCount);
        MultiByteToWideChar(CP_UTF8, 0, s, static_cast<int>(length), wideString.data(), static_cast<int>(wideString.size()));

        return wstring(wideString.data());
    }

    wstring ReadRegistryValue(const wstring& subKey, const wstring& propName)
    {
        wstring propValue;
        LSTATUS status = TryReadRegistryValue(subKey, propName, propValue);
        if (status != ERROR_SUCCESS)
        {
            TRACEP(L"Error: Could not read registry value: ", (subKey + L"\\" + propName).c_str());
            throw IotExceptionWithErrorCode(status);
        }
        return propValue;
    }

    LSTATUS TryReadRegistryValue(const wstring& subKey, const wstring& propName, wstring& propValue)
    {
        DWORD dataSize = 0;
        LSTATUS status;
        status = RegGetValue(HKEY_LOCAL_MACHINE, subKey.c_str(), propName.c_str(), RRF_RT_REG_SZ, NULL, NULL, &dataSize);
        if (status != ERROR_SUCCESS)
        {
            return status;
        }

        vector<char> data(dataSize);
        status = RegGetValue(HKEY_LOCAL_MACHINE, subKey.c_str(), propName.c_str(), RRF_RT_REG_SZ, NULL, data.data(), &dataSize);
        if (status != ERROR_SUCCESS)
        {
            return status;
        }

        propValue = reinterpret_cast<const wchar_t*>(data.data());

        return ERROR_SUCCESS;
    }

    string WideToMultibyte(const wchar_t* s)
    {
        size_t length = s ? wcslen(s) : 0;
        size_t requiredCharCount = WideCharToMultiByte(CP_UTF8, 0, s, static_cast<int>(length), nullptr, 0, nullptr, nullptr);

        // add room for \0
        ++requiredCharCount;

        vector<char> multibyteString(requiredCharCount);
        WideCharToMultiByte(CP_UTF8, 0, s, static_cast<int>(length), multibyteString.data(), static_cast<int>(multibyteString.size()), nullptr, nullptr);

        return string(multibyteString.data());
    }
}