#include "pch.h"
#include "MemMgmt.h"
#include "win_log.h"
#include "WinUtils.h"
#include "CTGlobals.h"

const  std::wstring CTGlobals::LOG_PATH = []() {
    const static std::wstring LOG_NAME = L"ClassicTileCascade.log";

    std::wstring szLogPath;

    LPWSTR lpwstrPath = nullptr;
    HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &lpwstrPath);
    if (SUCCEEDED(hr)) {
        std::wstring szPath = lpwstrPath;
        ::CoTaskMemFree(lpwstrPath);

        CTWinUtils::PathCombineEx(szLogPath, szPath, LOG_NAME);
    }

    return szLogPath;
}();

const std::wstring CTGlobals::CURR_MODULE_PATH = []() {
    std::wstring szCurrModulePath;
    CTWinUtils::GetCurrModuleFileName(szCurrModulePath);
    return szCurrModulePath;
}();
