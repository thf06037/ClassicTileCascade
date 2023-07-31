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

bool GetLogPath(std::string& szLogPath);
FILE* InitLogging();
bool CheckAlreadyRunning(UINT uRetries);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE ,
                     _In_ LPWSTR,
                     _In_ int )
{
    log_set_quiet(true);

    if (CheckAlreadyRunning(5)) {
        return 1;
    }

    FILE* pLogFP = InitLogging();
    ClassicTileWnd classicTileWnd(pLogFP);
    bool bSuccess = false;
    if (classicTileWnd.RegUnReg(bSuccess)) {
        return bSuccess ? 0 : 1;
    }
    
    if (!classicTileWnd.InitInstance(hInstance)){
        return 1;
    }

    MSG msg;

    while (::GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }

    if (pLogFP) {
        fclose(pLogFP);
    }

    return static_cast<int>(msg.wParam);
}


FILE* InitLogging()
{
    FILE* pLogFP = nullptr;
    std::string szLogPath;
    if (GetLogPath(szLogPath)) {
        pLogFP = _fsopen(szLogPath.c_str(), "a+", _SH_DENYWR);
    }

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

bool CheckAlreadyRunning(UINT uRetries)
{
    bool bRetVal = true;
    for (UINT i = 0; bRetVal && (i < uRetries); i++) {
        HANDLE hFirst = ::CreateMutexW(nullptr, FALSE, MUTEX_GUID.c_str());
        bRetVal = (hFirst && (::GetLastError() == ERROR_ALREADY_EXISTS));
        if (bRetVal && (i != (uRetries - 1))) {
            ::Sleep(500);
        }
    }
    return bRetVal;
}