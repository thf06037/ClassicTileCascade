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
#include "WinUtils.h"
#include "ClassicTileRegUtil.h"

//helper functions
LONG OpenOrCreateRegKey(std::wstring_view szPath, bool bCreate, SPHKEY& hKey);
LONG GetDWORDRegValue(std::wstring_view szPath, std::wstring_view szValueName, DWORD& dwValue);
LONG SetDWORDRegValue(std::wstring_view szPath, std::wstring_view szValueName, DWORD dwValue, bool bCreate);
LONG GetBoolRegValue(std::wstring_view szPath, std::wstring_view szValueName, bool& bValue);
LONG SetBoolRegValue(std::wstring_view szPath, std::wstring_view szValueName, bool bValue, bool bCreate);

//Registry paths and value names
constexpr static std::wstring_view REG_KEY_PATH = L"Software\\thf\\ClassicTileCascade";
constexpr static std::wstring_view REG_KEY_PARENT_PATH = L"Software\\thf";
constexpr static std::wstring_view REG_LEFT_CLICK_VAL = L"LeftClickAction";
constexpr static std::wstring_view REG_RUN_PATH = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr static std::wstring_view REG_RUN_NAME = L"ClassicTileCascade";
constexpr static std::wstring_view REG_LOGGING_VAL = L"Logging";
constexpr static std::wstring_view REG_DEFWNDTILE_VAL = L"DefWndTile";
constexpr static std::wstring_view REG_STATUSBAR_VAL = L"StatusBar";


//LONG OpenOrCreateRegKey(const std::wstring& szPath, bool bCreate, SPHKEY& hKey)
LONG OpenOrCreateRegKey(std::wstring_view szPath, bool bCreate, SPHKEY& hKey)
{
    return  bCreate
            ?
            ::RegCreateKeyExW(HKEY_CURRENT_USER, szPath.data(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, std::out_ptr(hKey), nullptr)
            :
            ::RegOpenKeyExW(HKEY_CURRENT_USER, szPath.data(), 0, KEY_ALL_ACCESS, std::out_ptr(hKey));
}

LONG GetDWORDRegValue(std::wstring_view szPath, std::wstring_view szValueName, DWORD& dwValue)
{
    DWORD dwSize = sizeof(dwValue);
    return ::RegGetValueW(HKEY_CURRENT_USER, szPath.data(), szValueName.data(), RRF_RT_DWORD, nullptr, &dwValue, &dwSize);
}

LONG SetDWORDRegValue(std::wstring_view szPath, std::wstring_view szValueName, DWORD dwValue, bool bCreate)
{
    SPHKEY hKey;
    LONG lReturnValue = OpenOrCreateRegKey(szPath, bCreate, hKey);

    if (lReturnValue == ERROR_SUCCESS) {
        lReturnValue = ::RegSetValueExW(hKey.get(), szValueName.data(), 0, REG_DWORD, reinterpret_cast<LPBYTE>(&dwValue), sizeof(dwValue));
    }
    return lReturnValue;
}

LONG GetBoolRegValue(std::wstring_view szPath, std::wstring_view szValueName, bool& bValue)
{
    bValue = false;
    DWORD dwValue = 0;
    LONG lReturnValue = GetDWORDRegValue(szPath, szValueName, dwValue);

    if (lReturnValue == ERROR_SUCCESS) {
        bValue = (dwValue != 0);
    }
    return lReturnValue;
}

LONG SetBoolRegValue(std::wstring_view szPath, std::wstring_view szValueName, bool bValue, bool bCreate)
{
    return SetDWORDRegValue(szPath, szValueName, bValue ? 1 : 0, bCreate);
}


LONG ClassicTileRegUtil::CheckRegAppPath()
{
    SPHKEY hKey;

    return OpenOrCreateRegKey(REG_KEY_PATH.data(), false, hKey);
}

// Delete the main registry app path. The parent path ("Software\\thf") is the direct decendent of HKCU\Software.
// If the parent path has no child keys or values, delete it too.
LONG ClassicTileRegUtil::DeleteRegAppPath()
{
    LONG lResult = ::RegDeleteKeyW(HKEY_CURRENT_USER, REG_KEY_PATH.data());
    if (lResult == ERROR_SUCCESS) {
        SPHKEY hKey;
        lResult = ::RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY_PARENT_PATH.data(), 0, KEY_QUERY_VALUE, std::out_ptr(hKey));
        if (lResult == ERROR_SUCCESS) {
            DWORD cSubKeys = 0;
            DWORD cValues = 0;

            lResult = ::RegQueryInfoKeyW(hKey.get(), nullptr, nullptr, nullptr, &cSubKeys, nullptr, nullptr, &cValues, nullptr, nullptr, nullptr, nullptr);

            if ((cSubKeys == 0) && (cValues == 0)) {
                lResult = ::RegDeleteKeyW(HKEY_CURRENT_USER, REG_KEY_PARENT_PATH.data());
            }
        }
    }
    return lResult;
}

LONG ClassicTileRegUtil::GetRegLeftClickAction(DWORD& dwLeftClickAction)
{
    return GetDWORDRegValue(REG_KEY_PATH, REG_LEFT_CLICK_VAL, dwLeftClickAction);
}

LONG ClassicTileRegUtil::SetRegLeftClickAction(DWORD dwLeftClickAction)
{
    return SetDWORDRegValue(REG_KEY_PATH, REG_LEFT_CLICK_VAL, dwLeftClickAction, true);
}

LONG ClassicTileRegUtil::GetRegLogging(bool& bLogging)
{
    return GetBoolRegValue(REG_KEY_PATH, REG_LOGGING_VAL, bLogging);
}

LONG ClassicTileRegUtil::SetRegLogging(bool bLogging)
{
    return SetBoolRegValue(REG_KEY_PATH, REG_LOGGING_VAL, bLogging, true);
}

LONG ClassicTileRegUtil::CheckRegRun()
{
    return ::RegGetValueW(HKEY_CURRENT_USER, REG_RUN_PATH.data(), REG_RUN_NAME.data(), RRF_RT_REG_SZ, nullptr, nullptr, nullptr);
}

LONG ClassicTileRegUtil::SetRegRun()
{
    SPHKEY hKey;
    LONG lReturnValue = OpenOrCreateRegKey(REG_RUN_PATH, false, hKey);
    if (lReturnValue == ERROR_SUCCESS) {
        std::wstring szExePath;
        DWORD dwExePathLen = CTWinUtils::GetCurrModuleFileName(szExePath);
        lReturnValue = ::RegSetValueExW(hKey.get(), REG_RUN_NAME.data(), 0, REG_SZ, reinterpret_cast<LPBYTE>(szExePath.data()), dwExePathLen * sizeof(wchar_t));
    }
    return lReturnValue;
}

LONG ClassicTileRegUtil::DeleteRegRun()
{
    SPHKEY hKey;
    LONG lReturnValue = OpenOrCreateRegKey(REG_RUN_PATH, false, hKey);
    if (lReturnValue == ERROR_SUCCESS) {
        lReturnValue = ::RegDeleteValueW(hKey.get(), REG_RUN_NAME.data());
    }
    return lReturnValue;
}

LONG ClassicTileRegUtil::GetRegDefWndTile(bool& bDefWndTile)
{
    return GetBoolRegValue(REG_KEY_PATH, REG_DEFWNDTILE_VAL, bDefWndTile);
}

LONG ClassicTileRegUtil::SetRegDefWndTile(bool bDefWndTile)
{
    return SetBoolRegValue(REG_KEY_PATH, REG_DEFWNDTILE_VAL, bDefWndTile, true);
}

LONG ClassicTileRegUtil::GetRegStatusBar(bool& bStatusBar)
{
    return GetBoolRegValue(REG_KEY_PATH, REG_STATUSBAR_VAL, bStatusBar);
}

LONG ClassicTileRegUtil::SetRegStatusBar(bool bStatusBar)
{
    return SetBoolRegValue(REG_KEY_PATH, REG_STATUSBAR_VAL, bStatusBar, true);
}
