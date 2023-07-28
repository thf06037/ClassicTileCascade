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


 //ShellExecInExplorerProcess is adapted from WindowsSDK7-Samples/winui/shell/appplatform/ExecInExplorer/ExecInExplorer.cpp
 //https://github.com/pauldotknopf/WindowsSDK7-Samples/tree/master/winui/shell/appplatform/ExecInExplorer

#include "pch.h"
#include "WinUtils.h"
#include "win_log.h"
#include "resource.h"
#include "MemMgmt.h"

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

HRESULT CTWinUtils::ShellExecInExplorerProcess(const std::wstring& szFile, const std::wstring& szArgs)
{
    _COM_SMARTPTR_TYPEDEF(IShellWindows, IID_IShellWindows);
    _COM_SMARTPTR_TYPEDEF(IShellBrowser, IID_IShellBrowser);
    _COM_SMARTPTR_TYPEDEF(IShellFolderViewDual, IID_IShellFolderViewDual);
    _COM_SMARTPTR_TYPEDEF(IShellView, IID_IShellView);
    _COM_SMARTPTR_TYPEDEF(IShellDispatch2, IID_IShellDispatch2);


    HRESULT hr = E_UNEXPECTED;
    try {
        IShellWindowsPtr spShellWindows;
        hr = eval_error_hr(spShellWindows.CreateInstance(CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER));
        
        HWND hwnd = nullptr;
        IDispatchPtr spDisp;
        _variant_t vtEmpty;
        hr = eval_error_hr(spShellWindows->FindWindowSW(&vtEmpty, &vtEmpty, SWC_DESKTOP, reinterpret_cast<long*>(&hwnd), SWFO_NEEDDISPATCH, &spDisp));
        if (hr != S_OK) {
            generate_error("IShellWindows::FindWindowSW did not return S_OK");
        }

        IShellBrowserPtr spSB;
        hr = eval_error_hr(::IUnknown_QueryService(spDisp, SID_STopLevelBrowser, spSB.GetIID(), reinterpret_cast<void**>(&spSB)));

        IShellViewPtr spSV;
        hr = eval_error_hr(spSB->QueryActiveShellView(&spSV));

        hr = eval_error_hr(spSV->GetItemObject(SVGIO_BACKGROUND, spDisp.GetIID(), reinterpret_cast<void**>(&spDisp)));

        IShellFolderViewDualPtr spSFVD = spDisp;
        hr = eval_error_hr(spSFVD ? S_OK : E_NOINTERFACE);

        hr = eval_error_hr(spSFVD->get_Application(&spDisp));

        IShellDispatch2Ptr spSD2 = spDisp;
        hr = eval_error_hr(spSD2 ? S_OK : E_NOINTERFACE);

        _bstr_t bstrFile = szFile.c_str();
        _variant_t vtArgs;
        if (szArgs.size() > 0) {
            vtArgs = szArgs.c_str();
        }
        hr = spSD2->ShellExecuteW(bstrFile, vtArgs, vtEmpty, vtEmpty, vtEmpty);
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
        hr = E_UNEXPECTED;
    }

    return hr;
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