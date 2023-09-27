/*
 * Copyright (c) 2023 thf
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "pch.h"
#include "MemMgmt.h"
#include "win_log.h"
#include "WinUtils.h"

const DWORD PROC_ID = ::GetCurrentProcessId();

DWORD eval_log_errorsuccess_getlasterror(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, DWORD dwError)
{
    if (dwError != ERROR_SUCCESS) {
        throw DWLoggingException(level, file, line, function, functionCalled, dwError);
    }
    return dwError;
}

HRESULT eval_log_getcomerror(int level, const char* file, int line, const std::string& function, const std::string& functionCalled, HRESULT hr)
{
    if (FAILED(hr)) {
        throw HRLoggingException(level, file, line, function, functionCalled, hr);
    }
    return hr;
}

void generate_exception(int level, const char* file, int line, const std::string& function, const std::string& msg)
{
    throw MSGLoggingException(level, file, line, function, msg);
}

LoggingException::LoggingException(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled)//, const T& errVal)
    : m_level(level),
    m_file(file),
    m_line(line),
    m_function(function),
    m_functionCalled(functionCalled){}

bool LoggingException::FormatMsg(DWORD dwErrVal, std::string& szErrMsg)
{
    LPSTR lpszErrMsg = NULL;

    DWORD dwFmtMsg = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwErrVal,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&lpszErrMsg),
        0,
        NULL);

    if (dwFmtMsg) {
        szErrMsg = lpszErrMsg;
        ::LocalFree(lpszErrMsg);
    }

    return dwFmtMsg > 0;
}

HRLoggingException::HRLoggingException(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, HRESULT errVal)
    :   LoggingException(level, file, line, function, functionCalled),
        m_errVal(errVal){}


void HRLoggingException::Log() const
{
    static const std::string FMT_FUNCTION = "%s: Calling function <%s>: Received error : <0X%08X> %s";

    IErrorInfoPtr spErrInfo;

    std::string szErrMsg;
    HRESULT hrGEI = ::GetErrorInfo(NULL, &spErrInfo);
    //if (SUCCEEDED(hrGEI)) {
    if (hrGEI == S_OK) {
        _bstr_t szBDesc;
        hrGEI = spErrInfo->GetDescription(szBDesc.GetAddress());
        szErrMsg = static_cast<LPSTR>(szBDesc);
    }

//    if (FAILED(hrGEI)) {
    if (hrGEI != S_OK) {
        if (!FormatMsg(m_errVal, szErrMsg)) {
            szErrMsg = "Unknown COM error";
        }
    }

    log_log(m_level, m_file.c_str(), m_line, FMT_FUNCTION.c_str(), m_function.c_str(), m_functionCalled.c_str(), m_errVal, szErrMsg.c_str());
}

DWLoggingException::DWLoggingException(int level, const std::string& file, int line, const std::string& function, const std::string& functionCalled, DWORD errVal)
    :   LoggingException(level, file, line, function, functionCalled),
        m_errVal(errVal){}

void DWLoggingException::Log() const
{
    static const std::string FMT_FUNCTION = "%s: Calling function <%s>: Received error : <0X%08X> %s";

    std::string szErrVal;
    if (!FormatMsg(m_errVal, szErrVal)) {
        szErrVal = "Unknown Windows error";
    }

    log_log(m_level, m_file.c_str(), m_line, FMT_FUNCTION.c_str(), m_function.c_str(), m_functionCalled.c_str(), m_errVal, szErrVal.c_str());
}

MSGLoggingException::MSGLoggingException(int level, const std::string& file, int line, const std::string& function, const std::string& errMsg)
    :   LoggingException(level, file, line, function, ""),
        m_errMsg(errMsg){}

void MSGLoggingException::Log() const
{
    static const std::string FMT_FUNCTION = "%s: %s";
    log_log(m_level, m_file.c_str(), m_line, FMT_FUNCTION.c_str(), m_function.c_str(), m_errMsg.c_str());
}


bool enable_logging(const std::string& szLogPath, SPFILE& spFile )
{
    bool bRetVal = false;


    if (!spFile) {
        spFile.reset( _fsopen(szLogPath.c_str(), "a+", _SH_DENYWR) );
    }

    if (spFile) {
        if (log_find_fp(spFile.get(), LOG_TRACE) == 0) {
            bRetVal = true;
        } else {
            bRetVal = (log_add_fp(spFile.get(), LOG_TRACE) == 0);
        }
    } 

    return bRetVal;
}

bool enable_logging(const std::wstring& szLogPath, SPFILE& spFile)
{
    std::string szLogPathNarrow;
    CTWinUtils::Wstring2string(szLogPathNarrow, szLogPath);
    return enable_logging(szLogPathNarrow, spFile);
}