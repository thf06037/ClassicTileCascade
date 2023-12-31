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

void CTWinUtils::ShellHelper(LPSHELLFUNC lpShellFunc)
{
    _COM_SMARTPTR_TYPEDEF(IShellDispatch, IID_IShellDispatch);
    try {
        if (lpShellFunc) {
            CCoInitialize coInit;
            IShellDispatchPtr shell;
            eval_error_hr(shell.CreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER));
            eval_error_hr((shell.GetInterfacePtr()->*lpShellFunc)());
        }
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}

bool CTWinUtils::CheckMenuItem(HMENU hMenu, UINT uID, bool bChecked)
{
    MENUITEMINFOW mii = { 0 };
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE;
    mii.fState = bChecked ? MFS_CHECKED : MFS_UNCHECKED;
    return (::SetMenuItemInfoW(hMenu, uID, FALSE, &mii) == TRUE);
}

bool CTWinUtils::ShellExecInExplorerProcess(std::wstring_view szFile, std::wstring_view szArgs, LPDWORD pDWProcID)
{
    const static DWORD DW_TOKEN_RIGHTS = TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY | TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID;
    bool fRetVal = false;

    STARTUPINFOW si = { 0 };
    si.cb = sizeof(STARTUPINFOW);
    PROCESS_INFORMATION pi = { 0 };

    try{
        if (pDWProcID) {
            *pDWProcID = 0;
        }

        std::wstring szCommandLine { szFile };
        CTWinUtils::PathQuoteSpacesW(szCommandLine);
        if (!szArgs.empty()) {
            szCommandLine.append(L" ");
            szCommandLine.append(szArgs);
        }

        if (IsUserAnAdmin()) {
            SPHANDLE_EX hProcessToken;
            eval_error_nz(::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, std::out_ptr(hProcessToken)));
        
            TOKEN_PRIVILEGES tkp = {0};
            tkp.PrivilegeCount = 1;
            eval_error_nz(::LookupPrivilegeValueW(nullptr, SE_INCREASE_QUOTA_NAME, &tkp.Privileges[0].Luid));
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        
            eval_error_nz(::AdjustTokenPrivileges(hProcessToken.get(), FALSE, &tkp, 0, nullptr, nullptr));
            if (::GetLastError()  != ERROR_SUCCESS){
                generate_error("AdjustTokenPrivileges did not get required privileges");
            }

            HWND hWnd = eval_error_nz(::GetShellWindow());
        
            DWORD dwPID;
            eval_error_nz(GetWindowThreadProcessId(hWnd, &dwPID));

            SPHANDLE_EX hShellProcess(eval_error_nz(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID)) );

            SPHANDLE_EX hShellProcessToken;
            eval_error_nz(::OpenProcessToken(hShellProcess.get(), TOKEN_DUPLICATE, std::out_ptr(hShellProcessToken)));

            SPHANDLE_EX hPrimaryToken;
            eval_error_nz(::DuplicateTokenEx(hShellProcessToken.get(), DW_TOKEN_RIGHTS, nullptr, SecurityImpersonation, TokenPrimary, std::out_ptr(hPrimaryToken)));

            eval_error_nz(::CreateProcessWithTokenW(hPrimaryToken.get(), LOGON_WITH_PROFILE, nullptr, szCommandLine.data(), 0, nullptr, nullptr, &si, &pi));

            fRetVal = true;
        }else {
            eval_error_nz(::CreateProcessW(NULL, szCommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi));
            fRetVal = true;
        }
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }

    if (fRetVal) {
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);
        if (pDWProcID) {
            *pDWProcID = pi.dwProcessId;
        }
    }

    return fRetVal;
}


DWORD CTWinUtils::GetCurrModuleFileName(std::wstring& szCurrModFileName)
{
    szCurrModFileName.clear();
    sz_wbuf szCurrModFileNameBuf(szCurrModFileName, MAX_PATH);
    return ::GetModuleFileNameW(nullptr, szCurrModFileNameBuf, static_cast<DWORD>(szCurrModFileNameBuf.size()));
}

void CTWinUtils::Wstring2string(std::string& szString, const std::wstring& szWString)
{
    szString.clear();
    sz_buf szStringBuf(szString, szWString.size() + 1);
    ::wcstombs_s(nullptr, szStringBuf, szStringBuf.size(), szWString.c_str(), szStringBuf.size() - 1);
}

void CTWinUtils::String2wstring(std::wstring& szWString, const std::string& szString)
{
    szWString.clear();
    sz_wbuf szWStringBuf(szWString, szString.size() + 1);
    ::mbstowcs_s(nullptr, szWStringBuf, szWStringBuf.size(), szString.c_str(), static_cast<size_t>(szWStringBuf.size() - 1));
}

bool CTWinUtils::GetMenuStringSubMenu(HMENU hMenu, UINT uItem, bool bByPosition, std::wstring& szMenu, HMENU* phSubMenu)
{
    bool bRetVal = false;
    szMenu.clear();
    if (phSubMenu) {
        *phSubMenu = nullptr;
    }

    MENUITEMINFOW mii = { 0 };
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_FTYPE | MIIM_STRING;

    if (::GetMenuItemInfoW(hMenu, uItem, bByPosition ? TRUE : FALSE, &mii)
        && (mii.fType == MFT_STRING)
        && (mii.cch > 0))
    {
        sz_wbuf szMenuBuf(szMenu, mii.cch + 1);
        mii.dwTypeData = szMenuBuf;
        mii.cch = static_cast<UINT>(szMenuBuf.size());
        mii.fMask = MIIM_STRING;
        if (phSubMenu) {
            mii.fMask |= MIIM_SUBMENU;
        }

        if (::GetMenuItemInfoW(hMenu, uItem, bByPosition ? TRUE : FALSE, &mii)) {
            if (phSubMenu) {
                *phSubMenu = mii.hSubMenu;
            }
            bRetVal = true;
        }
    }
    return bRetVal;
}

LPSTR MyPathCombine(LPSTR pszDest, LPCSTR pszDir, LPCSTR pszFile)
{
	return ::PathCombineA(pszDest, pszDir, pszFile);
}

LPWSTR MyPathCombine(LPWSTR pszDest, LPCWSTR pszDir, LPCWSTR pszFile)
{
	return ::PathCombineW(pszDest, pszDir, pszFile);
}

template<class T, class TV>
bool CTWinUtils::PathCombineEx(T& szDest, TV szDir, TV szFile)
{
    szDest.clear();
    return (MyPathCombine(basic_sz_buf<T>(szDest, MAX_PATH), szDir.data(), szFile.data()) != nullptr);
}

template
bool CTWinUtils::PathCombineEx(std::string& szDest, std::string_view szDir, std::string_view szFile);

template
bool CTWinUtils::PathCombineEx(std::wstring& szDest, std::wstring_view szDir, std::wstring_view szFile);

bool CTWinUtils::CreateProcessHelper(std::wstring_view szCommand, std::wstring_view szArguments, LPDWORD lpdwProcId, const std::optional<int>& nShowWindow)
{
    bool bRetVal = false;
    if (lpdwProcId) {
        *lpdwProcId = 0;
    }

    STARTUPINFO si = { 0 };
    si.cb = sizeof(si);
    if (nShowWindow) {
        si.dwFlags |= STARTF_USESHOWWINDOW;
        si.wShowWindow = static_cast<WORD>(*nShowWindow);
    }

    PROCESS_INFORMATION pi = { 0 };
    
    std::wstring szCommandLine{szCommand};
    CTWinUtils::PathQuoteSpacesW(szCommandLine);
    if (!szArguments.empty()) {
        szCommandLine += L" ";
        szCommandLine += szArguments;
    }

    bRetVal = (::CreateProcessW(nullptr, szCommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) == TRUE);
    if (bRetVal) {
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);

        if (lpdwProcId) {
            *lpdwProcId = pi.dwProcessId;
        }
    }
    return bRetVal;
}

bool CTWinUtils::PathQuoteSpacesW(std::wstring& szPath)
{
    return (::PathQuoteSpacesW(sz_wbuf(szPath, MAX_PATH)) == TRUE);
}

UINT CTWinUtils::SetSubMenuDataFromItemData(HMENU hMenu)
{
    UINT uRetVal = 0;

    int nCount = ::GetMenuItemCount(hMenu);

    for (UINT i = 0; i < static_cast<UINT>(nCount); i++) {
        MENUITEMINFOW mii = { 0 };
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_ID;

        if (    
                ::GetMenuItemInfoW(hMenu, i, TRUE, &mii)
                &&
                mii.hSubMenu
        ) {
            if (mii.wID > 0) {
                MENUINFO mi = { 0 };
                mi.cbSize = sizeof(mi);
                mi.fMask = MIM_MENUDATA;
                mi.dwMenuData = mii.wID;
                if (::SetMenuInfo(mii.hSubMenu, &mi)) {
                    uRetVal++;
                }
            }
            uRetVal += SetSubMenuDataFromItemData(mii.hSubMenu);
        }
    }

    return uRetVal;
}

bool CTWinUtils::FileExists(std::wstring_view szPath)
{
    DWORD dwAttrib = ::GetFileAttributesW(szPath.data());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES) && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

void CTWinUtils::OpenTextFile(HWND hwnd, std::wstring_view szPath, std::wstring_view szAppName)
{
    constexpr static wchar_t FMT_FILE_NOT_FOUND[] = L"Log file <{0}> does not exist yet.";

    try {
        const static std::wstring NOTEPAD_PATH = [] {
            eval_error_nz(::SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE));
            std::wstring szNotepadPath;
            sz_wbuf szNotepadPathBuf(szNotepadPath, MAX_PATH);
            eval_error_nz(::SearchPathW(nullptr, L"notepad.exe", nullptr, static_cast<DWORD>(szNotepadPathBuf.size()), szNotepadPathBuf, nullptr));
            return szNotepadPath;
        }();

        if (!FileExists(szPath)) {
            ::MessageBoxW(hwnd, std::format(FMT_FILE_NOT_FOUND, szPath).c_str(), szAppName.data(), MB_OK | MB_ICONINFORMATION);
        } else {
            std::wstring szResult;
            LONG_PTR nRetVal = reinterpret_cast<LONG_PTR>(::FindExecutableW(szPath.data(), NULL, sz_wbuf(szResult, MAX_PATH)));
            if (nRetVal <= 32) {
                szResult = NOTEPAD_PATH;
            }

            std::wstring szPathQuote { szPath };
            PathQuoteSpacesW(szPathQuote);
            eval_error_nz(CreateProcessHelper(szResult, szPathQuote, nullptr, SW_SHOWMAXIMIZED));
        }
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}
