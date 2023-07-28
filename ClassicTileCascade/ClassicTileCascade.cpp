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

// ClassicTileCascade.cpp : Defines the entry point for the application.
//


#include "pch.h"
#include "framework.h"
#include "resource.h"
#include "ClassicTileWnd.h"
#include "ClassicTileRegUtil.h"
#include "win_log.h"
#include "MemMgmt.h"
#include "WinUtils.h"

const static std::wstring MUTEX_GUID = L"{436805EB-7307-4A82-A1AB-C87DC5EE85B6";

bool RegDeReg(SPFILE& pLogFP);
void Unregister();
bool GetLogPath(std::string& szLogPath);
FILE* InitLogging(const std::string& szLogPath);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE ,
                     _In_ LPWSTR,
                     _In_ int )
{
    DWORD dwLogging = 0;
    bool bLogging = false;
    LONG lReturnValue = ClassicTileRegUtil::GetRegLogging(dwLogging);
    if (lReturnValue == ERROR_SUCCESS) {
        bLogging = (dwLogging != 0);
    }

    log_set_quiet(true);

    SPFILE pLogFP;
    std::string szLogPath;
    if (GetLogPath(szLogPath)) {
        pLogFP.reset(InitLogging(szLogPath));
    }


    if (RegDeReg(pLogFP)) {
        return 0;
    }

    if (bLogging) {
        if (pLogFP) {
            log_add_fp(pLogFP.get(), LOG_TRACE);
        }else {
            return 1;
        }
    }


    HANDLE hFirst = ::CreateMutexW(nullptr, FALSE, MUTEX_GUID.c_str());
    if (hFirst && (::GetLastError() == ERROR_ALREADY_EXISTS))
    {
        log_info("ClassicTileCascade already running, exiting.");
        return 1;
    }

    ClassicTileWnd classiTileWnd(bLogging, szLogPath);
    if (!classiTileWnd.InitInstance(hInstance))
    {
        return 1;
    }


    MSG msg;

    while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}


bool RegDeReg(SPFILE& pLogFP)
{
    static const std::wregex REG(L"^/REGISTER$", std::regex_constants::icase);
    static const std::wregex UNREG(L"^/UNREGISTER$", std::regex_constants::icase);
    static const std::wregex REGUSER(L"^/REGISTERUSER$", std::regex_constants::icase);
    static const std::wregex UNREGUSER(L"^/UNREGISTERUSER$", std::regex_constants::icase);

    log_add_fp(pLogFP.get(), LOG_TRACE);

    bool fRetVal = false;
    int nArgs = 0;
    LPWSTR* lpszArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (lpszArglist) {
        std::wstring szFirstArg;
        if (nArgs > 1) {
            szFirstArg = lpszArglist[1];
        }
        LocalFree(lpszArglist);
        if (nArgs > 1) {
            std::wstring szExePath;
            CTWinUtils::GetCurrModuleFileName(szExePath);

            if (std::regex_match(szFirstArg, REG) || std::regex_match(szFirstArg, UNREG)) {
                //our process may be running as an elevated user. we need the registry entries to be added in
                //a process that is running as the logged in user
                try {
                    std::wstring szRegUser = szFirstArg + L"USER";

                    CCoInitialize coInit;
                    eval_fatal_hr(CTWinUtils::ShellExecInExplorerProcess(szExePath, szRegUser));
                    log_info("Launched app as standard user");
                    fRetVal = true;
                }catch (const LoggingException& le) {
                    le.Log();
                }catch (...) {
                    log_error("Unhandled exception");
                }
            }else if (std::regex_match(szFirstArg, REGUSER) || std::regex_match(szFirstArg, UNREGUSER)) {
                Unregister();
                if (std::regex_match(szFirstArg, REGUSER)) {
                    bool fSuccess = true;
                    try {
                        eval_error_es(ClassicTileRegUtil::SetRegLeftClickAction(ID_FILE_CASCADEWINDOWS));

                        eval_error_es(ClassicTileRegUtil::SetRegLogging(0));

                        eval_error_es(ClassicTileRegUtil::SetRegDefWndTile(0));

                        eval_error_es(ClassicTileRegUtil::SetRegRun());

                        STARTUPINFO si = { 0 };
                        si.cb = sizeof(si);

                        PROCESS_INFORMATION pi = { 0 };
                        eval_error_nz(::CreateProcessW(nullptr, szExePath.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi));
                        ::CloseHandle(pi.hProcess);
                        ::CloseHandle(pi.hThread);

                        fSuccess = true;
                    }catch(const LoggingException& le){
                        le.Log();
                        fSuccess = false;
                    }catch (...) {
                        log_error("Unhandled exception");
                        fSuccess = false;
                    }
                    
                    log_info("ClassicTileCascade %s", fSuccess ? "registered" : "register failed");
                }
                fRetVal = true;
            }
        }
    }

    return fRetVal;
}

void Unregister()
{
    bool fSuccess = true;
    LONG lReturnValue = ClassicTileRegUtil::CheckRegAppPath();
    if (lReturnValue == ERROR_SUCCESS) {
        try {
            eval_error_es(ClassicTileRegUtil::DeleteRegAppPath());
        }catch(const LoggingException& le) {
            le.Log();
            fSuccess = false;
        }catch (...) {
            log_error("Unhandled exception");
            fSuccess = false;
        }
    }

    lReturnValue = ClassicTileRegUtil::CheckRegRun();
    if (lReturnValue == ERROR_SUCCESS) {
        try {
            eval_error_es(ClassicTileRegUtil::DeleteRegRun());
        }catch (const LoggingException& le) {
            le.Log();
            fSuccess = false;
        }catch (...) {
            log_error("Unhandled exception");
            fSuccess = false;
        }
    }

    log_info("ClassicTileCascade %s", fSuccess ? "unregistered" : "unregister failed");
}

FILE* InitLogging(const std::string& szLogPath)
{
    FILE* pLogFP = nullptr;
    errno_t openErr = ::fopen_s(&pLogFP, szLogPath.c_str(), "a+");
    return pLogFP;
}

bool GetLogPath(std::string& szLogPath)
{
    const static std::string LOG_NAME = "ClassicTileCascade.log";
    bool fRetVal = false;
    LPWSTR lpwstrPath = nullptr;

    HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &lpwstrPath);
    if (SUCCEEDED(hr)) {
        std::wstring strPath = lpwstrPath;
        ::CoTaskMemFree(lpwstrPath);

        std::string szPath;
        CTWinUtils::Wstring2string(szPath, strPath);

        CTWinUtils::PathCombineEx(szLogPath, szPath, LOG_NAME);
        fRetVal = true;
    }
    return fRetVal;
}

