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
#include "resource.h"
#include "MemMgmt.h"
#include "win_log.h"
#include "WinUtils.h"
#include "ClassicTileWnd.h"
#include "ClassicTileRegUtil.h"
#include "CTGlobals.h"

const static std::wstring MUTEX_GUID = L"{436805EB-7307-4A82-A1AB-C87DC5EE85B6";

bool CheckAlreadyRunning(UINT uRetries);
bool RegUnReg(bool& fSuccess);
bool RegUnRegAsUser(const std::wstring& szFirstArg);
bool Unregister();
bool Register();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE ,
                     _In_ LPWSTR,
                     _In_ int )
{
    // This function call ensures that we don't log anything to stderr. 
    // In InitInstance, the ClassicTileWnd class will determine whether file 
    // logging is on or off based on thesetting in "Settings | Logging" menu item
    log_set_quiet(true);

    // Letting the application run more than once would create multiple 
    // notification icons. This function uses a mutex to determine whether
    // app is already running. ABEND if another instance is running.
    if (CheckAlreadyRunning(5)) {
        return 1;
    }

    // Check whether the /REGISTER,/UNREGISTER, /REGISTERUSER
    // or /UNREGISTERUSER command line parameters have been passed
    // These are passed by the Windows installer to orchestrate the 
    // creation or deletion of the registry entries for the application
    bool bSuccess = false;
    if (RegUnReg(bSuccess)) {
        return bSuccess ? 0 : 1;
    }
    
    // No command line parameters passed - create notification icon and
    // wire the message handlers for the notification icon
    if(!ClassicTileWnd::Run(hInstance)){
        return 1;
    }

    // if we made it to here, our notification icon is created and 
    // message handlers are set up. Run a normal windows msg loop
    MSG msg = { 0 };

    while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        if (!ClassicTileWnd::CTWProcessDlgMsg(&msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

bool CheckAlreadyRunning(UINT uRetries)
{
    bool bRetVal = true;
    for (UINT i = 0; bRetVal && (i < uRetries); i++) {
        HANDLE hFirst = ::CreateMutexW(nullptr, FALSE, MUTEX_GUID.c_str());
        bRetVal = (hFirst && (::GetLastError() == ERROR_ALREADY_EXISTS));
        if (bRetVal ) {
            ::CloseHandle(hFirst);
            if (i != (uRetries - 1)) {
                ::Sleep(500);
            }
        }
    }
    return bRetVal;
}



bool RegUnReg(bool& fSuccess)
{
    static const std::wstring REG = L"/REGISTER";
    static const std::wstring UNREG = L"/UNREGISTER";
    static const std::wstring REGUSER = L"/REGISTERUSER";
    static const std::wstring UNREGUSER = L"/UNREGISTERUSER";

    bool fRetVal = false;
    fSuccess = false;
    int nArgs = 0;
    LPWSTR* lpszArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (lpszArglist) {
        std::wstring szFirstArg;
        if (nArgs > 1) {
            szFirstArg = lpszArglist[1];
        }
        LocalFree(lpszArglist);

        if (nArgs > 1) {
            SPFILE pFile;
            if (!enable_logging(CTGlobals::LOG_PATH, pFile)) {
                return fRetVal;
            }

            fRetVal = true;

            log_info_procid("<%S> command line argument passed.", szFirstArg.c_str());

            std::transform(szFirstArg.begin(), szFirstArg.end(), szFirstArg.begin(), [](auto c) { return std::toupper(c); });

            if ((szFirstArg == REG) || (szFirstArg == UNREG)) {
                //our process may be running as an elevated user. we need the registry entries to be added in
                //a process that is running as the logged in user
                fSuccess = RegUnRegAsUser(szFirstArg);
            } else if ((szFirstArg == REGUSER) || (szFirstArg == UNREGUSER)) {
                log_info_procid("<%S> parameter passed - starting register/unregister process", szFirstArg.c_str());

                fSuccess = Unregister();

                if (fSuccess && (szFirstArg == REGUSER)) {
                    fSuccess = Register();
                }
            } else {
                log_fatal_procid("<%S> unrecognized command line argument.", szFirstArg.c_str());
                fSuccess = false;
            }
        }
    }

    return fRetVal;

}

bool RegUnRegAsUser(const std::wstring& szFirstArg)
{
    bool fSuccess = false;
    try {
        std::wstring szRegUser = szFirstArg + L"USER";
        log_info_procid("Processs running at %s level.",  IsUserAnAdmin() ? "elevated" : "regular");
        log_info_procid("<%S> parameter passed - attempting to launch app as logged in user with command line argument <%S>",
            szFirstArg.c_str(), szRegUser.c_str());

        DWORD dwNewProcID = 0;
        eval_fatal_nz(CTWinUtils::ShellExecInExplorerProcess(CTGlobals::CURR_MODULE_PATH, szRegUser, &dwNewProcID));

        log_info_procid("Launched app as standard user with ProcID <%u>",  dwNewProcID);

        fSuccess = true;
    } catch (const LoggingException& le) {
        le.Log();
        fSuccess = false;
    } catch (...) {
        log_error("Unhandled exception");
        fSuccess = false;
    }
    return fSuccess;
}

bool Unregister()
{
    log_info_procid("Starting unregister function");

    bool fSuccess = false;

    try {
        LONG lReturnValue = ClassicTileRegUtil::CheckRegAppPath();
        if (lReturnValue == ERROR_SUCCESS) {
            log_info_procid("Application registry key exists, attempting delete");
            eval_error_es(ClassicTileRegUtil::DeleteRegAppPath());
            log_info_procid("Application registry key delete successful");
        } else {
            log_info_procid("Application registry key does not exist.");
        }

        lReturnValue = ClassicTileRegUtil::CheckRegRun();
        if (lReturnValue == ERROR_SUCCESS) {
            log_info_procid("Auto run registry value exists, attempting delete");
            eval_error_es(ClassicTileRegUtil::DeleteRegRun());
            log_info_procid("Auto run registry value key delete successful");
        } else {
            log_info_procid("Auto run registry value does not exist.");
        }
        fSuccess = true;
    } catch (const LoggingException& le) {
        le.Log();
        fSuccess = false;
    } catch (...) {
        log_error("Unhandled exception");
        fSuccess = false;
    }

    log_info_procid("ClassicTileCascade %s", fSuccess ? "unregistered" : "unregister failed");
    return fSuccess;
}

bool Register()
{
    log_info_procid("Starting register process.");

    bool fSuccess = false;

    try {
        eval_error_es(ClassicTileRegUtil::SetRegLeftClickAction(ID_FILE_CASCADEWINDOWS));
        log_info_procid("Added Left Click Action registry value.");

        eval_error_es(ClassicTileRegUtil::SetRegLogging(false));
        log_info_procid("Added Logging registry value.");

        eval_error_es(ClassicTileRegUtil::SetRegDefWndTile(false));
        log_info_procid("Added Default/Custom Window Tile/Cascade registry value.");

        eval_error_es(ClassicTileRegUtil::SetRegStatusBar(true));
        log_info_procid("Added Status Bar registry value.");

        eval_error_es(ClassicTileRegUtil::SetRegRun());
        log_info_procid("Added Auto Run registry value.");

        log_info_procid("Attempting to start interactive application.");

        DWORD dwNewProcId = 0;
        eval_error_nz(CTWinUtils::CreateProcessHelper(CTGlobals::CURR_MODULE_PATH, L"", &dwNewProcId));

        log_info_procid("Interactive application started successfully with ProcID <%u>.",  dwNewProcId);

        fSuccess = true;
    } catch (const LoggingException& le) {
        le.Log();
        fSuccess = false;
    } catch (...) {
        log_error("Unhandled exception");
        fSuccess = false;
    }
    log_info_procid("ClassicTileCascade %s",  fSuccess ? "registered" : "register failed");
    return fSuccess;
}
