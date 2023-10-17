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
#include "CTGlobals.h"

const  std::wstring CTGlobals::LOG_PATH = []() {
    constexpr static std::wstring_view LOG_NAME = L"ClassicTileCascade.log";

    std::wstring szLogPath;

    LPWSTR lpwstrPath = nullptr;
    HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &lpwstrPath);
    if (SUCCEEDED(hr)) {
        std::wstring szPath = lpwstrPath;
        ::CoTaskMemFree(lpwstrPath);

        CTWinUtils::PathCombineEx(szLogPath, static_cast<std::wstring_view>(szPath), LOG_NAME);
    }

    return szLogPath;
}();

const std::wstring CTGlobals::CURR_MODULE_PATH = []() {
    std::wstring szCurrModulePath;
    CTWinUtils::GetCurrModuleFileName(szCurrModulePath);
    return szCurrModulePath;
}();
