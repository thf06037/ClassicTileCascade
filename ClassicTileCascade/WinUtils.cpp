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
#include "win_log.h"
#include "MemMgmt.h"
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

UINT CTWinUtils::GetSubMenuPosByString(HMENU hMenu, const std::wstring& szString)
{
    int nSubMenuPos = -1;
    if (hMenu) {
        int nMenuCount = ::GetMenuItemCount(hMenu);
        for (int i = 0; i < nMenuCount; i++) {
            std::wstring szMenuString;
            HMENU hSubMenu = nullptr;
            if (GetMenuStringSubMenu(hMenu, i, true, szMenuString, &hSubMenu)
                && hSubMenu
                && (szMenuString == szString))
            {
                nSubMenuPos = i;
                break;
            }
        }
    }

    if (nSubMenuPos < 0) {
        std::string szStringNarrow;
        Wstring2string(szStringNarrow, szString);
        std::string err = "No submenu with string " + szStringNarrow;
        generate_error(err.c_str());
    }
    return nSubMenuPos;
}

bool CTWinUtils::CheckMenuItem(HMENU hMenu, UINT uID, bool bChecked)
{
    MENUITEMINFOW mii = { 0 };
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE;
    mii.fState = bChecked ? MFS_CHECKED : MFS_UNCHECKED;
    return (::SetMenuItemInfoW(hMenu, static_cast<UINT>(uID), FALSE, &mii) == TRUE);
}

bool CTWinUtils::ShellExecInExplorerProcess(const std::wstring& szFile, const std::wstring& szArgs, LPDWORD pDWProcID)
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

        std::wstring szCommandLine = L"\"" + szFile + L"\"" + ((szArgs.size() > 0) ? (L" " + szArgs) : L"");

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

            SPHANDLE_EX hShellProcess;
            hShellProcess.reset(eval_error_nz(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID)));

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
    DWORD dwRetVal = 0;
    szCurrModFileName.clear();
    szCurrModFileName.resize(MAX_PATH);
    dwRetVal = ::GetModuleFileNameW(nullptr, szCurrModFileName.data(), static_cast<DWORD>(szCurrModFileName.size()));
    RemoveFromNull(szCurrModFileName);
    return dwRetVal;
}

void CTWinUtils::Wstring2string(std::string& szString, const std::wstring& szWString)
{
    szString.clear();
    szString.resize(szWString.size() + 1);
    ::wcstombs_s(nullptr, szString.data(), szString.size(), szWString.c_str(), szString.size() - 1);
    RemoveFromNull(szString);
}

void CTWinUtils::String2wstring(std::wstring& szWString, const std::string& szString)
{
    szWString.clear();
    szWString.resize(szString.size() + 1);
    ::mbstowcs_s(nullptr, szWString.data(), szWString.size(), szString.c_str(), static_cast<size_t>(szWString.size() - 1));
    RemoveFromNull(szWString);
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
        szMenu.resize(mii.cch + 1);
        mii.dwTypeData = szMenu.data();
        mii.cch = static_cast<UINT>(szMenu.size());
        mii.fMask = MIIM_STRING;
        if (phSubMenu) {
            mii.fMask |= MIIM_SUBMENU;
        }

        if (::GetMenuItemInfoW(hMenu, uItem, bByPosition ? TRUE : FALSE, &mii)) {
            CTWinUtils::RemoveFromNull(szMenu);
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

template<class T>
bool CTWinUtils::PathCombineEx(T& szDest, const T& szDir, const T& szFile)
{
    bool bRetVal = false;
    szDest.clear();
    szDest.resize(MAX_PATH);
    bRetVal = (MyPathCombine(szDest.data(), szDir.c_str(), szFile.c_str()) != nullptr);

    if (bRetVal) {
        RemoveFromNull(szDest);
    }
    return bRetVal;
}

template
bool CTWinUtils::PathCombineEx(std::string& szDest, const std::string& szDir, const std::string& szFile);

template
bool CTWinUtils::PathCombineEx(std::wstring& szDest, const std::wstring& szDir, const std::wstring& szFile);

bool CTWinUtils::GetFinalPathNameByFILE(FILE* pFile, std::wstring& szPath)
{
    bool bRetVal = false;

    szPath.clear();

    int nFN = _fileno(pFile);
    if (nFN >= 0) {
        HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(nFN));
        if (hFile != INVALID_HANDLE_VALUE) {
            szPath.resize(MAX_PATH);
            if (::GetFinalPathNameByHandleW(hFile, szPath.data(), szPath.size(), 0)) {
                RemoveFromNull(szPath);
                if (SUCCEEDED(::PathCchStripPrefix(szPath.data(), szPath.size()))) {
                    RemoveFromNull(szPath);
                    bRetVal = true;
                }
            }
        }
    }
    return bRetVal;
}