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
#include "Logger.h"
#include "IotException.h"

namespace Utils
{
    // Threading helpers
    class JoiningThread
    {
    public:
        std::thread& operator=(std::thread&& t)
        {
            _thread = std::move(t);
            return _thread;
        }

        void Join()
        {
            if (_thread.joinable())
            {
                _thread.join();
            }
        }

        ~JoiningThread()
        {
            Join();
        }
    private:
        std::thread _thread;
    };

    template<class T>
    T TrimString(const T& s, const T& chars)
    {
        T trimmedString;

        // trim leading characters
        size_t startpos = s.find_first_not_of(chars);
        if (T::npos != startpos)
        {
            trimmedString = s.substr(startpos);
        }

        // trim trailing characters
        size_t endpos = trimmedString.find_last_not_of(chars);
        if (T::npos != endpos)
        {
            trimmedString = trimmedString.substr(0, endpos + 1);
        }
        return trimmedString;
    }

    class AutoCloseHandle
    {
    public:
        AutoCloseHandle() :
            _handle(NULL)
        {}

        AutoCloseHandle(HANDLE&& handle) :
            _handle(handle)
        {
            handle = NULL;
        }

        HANDLE operator=(HANDLE&& handle)
        {
            _handle = handle;
            handle = NULL;
            return _handle;
        }

        HANDLE Get() { return _handle; }
        uint64_t Get64() { return reinterpret_cast<uint64_t>(_handle); }
        HANDLE* GetAddress() { return &_handle; }

        BOOL Close()
        {
            BOOL result = TRUE;
            if (_handle != NULL)
            {
                result = CloseHandle(_handle);
                _handle = NULL;
            }
            return result;
        }

        ~AutoCloseHandle()
        {
            Close();
        }

    private:
        AutoCloseHandle(const AutoCloseHandle &);            // prevent copy
        AutoCloseHandle& operator=(const AutoCloseHandle&);  // prevent assignment

        HANDLE _handle;
    };

    void LaunchProcess(const std::wstring& commandString, unsigned long& returnCode, std::string& output);
    std::wstring MultibyteToWide(const char* s);
    std::wstring ReadRegistryValue(const std::wstring& subKey, const std::wstring& propName);
    LSTATUS TryReadRegistryValue(const std::wstring& subKey, const std::wstring& propName, std::wstring& propValue);
    std::string WideToMultibyte(const wchar_t* s);
}
