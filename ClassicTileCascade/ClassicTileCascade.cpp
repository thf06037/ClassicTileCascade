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
#include "WinUtils.h"
#include "ClassicTileWnd.h"

const static std::wstring MUTEX_GUID = L"{436805EB-7307-4A82-A1AB-C87DC5EE85B6";

bool CheckAlreadyRunning(UINT uRetries);

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

    ClassicTileWnd classicTileWnd;
    bool bSuccess = false;

    // Check whether the /REGISTER,/UNREGISTER, /REGISTERUSER
    // or /UNREGISTERUSER command line parameters have been passed
    // These are passed by the Windows installer to orchestrate the 
    // creation or deletion of the registry entries for the application
    if (classicTileWnd.RegUnReg(bSuccess)) {
        return bSuccess ? 0 : 1;
    }
    
    // No command line parameters passed - create notification icon and
    // wire the message handlers for the notification icon
    if (!classicTileWnd.InitInstance(hInstance)){
        return 1;
    }

    // if we made it to here, our notification icon is created and 
    // message handlers are set up. Run a normal windows msg loop
    MSG msg;

    while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
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