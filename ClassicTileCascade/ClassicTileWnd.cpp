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
#include "resource.h"
#include "ClassicTileRegUtil.h"
#include "WinUtils.h"
#include "CTGlobals.h"
#include "ClassicTileWnd.h"

#define SWM_TRAYMSG	WM_APP //the message ID sent to our window

//Message Handlers
/* BOOL Cls_OnSWMTrayMsg(HWND hwnd, WORD wNotifEvent, WORD wIconId, int x, int y) */
#define HANDLE_SWM_TRAYMSG(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), LOWORD(lParam), HIWORD(lParam), GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)), 0L)
#define FORWARD_SWM_TRAYMSG(hwnd, wNotifEvent, wIconId, x, y, fn) \
	(void)(fn)((hwnd), SWM_TRAYMSG, MAKEWPARAM((x), (y)), MAKELPARAM((wNotifEvent), (wIconId)))

ClassicTileWnd ClassicTileWnd::s_classicTileWnd;

ClassicTileWnd::ClassicTileWnd()
    : BaseWnd(true){}


bool ClassicTileWnd::BeforeWndCreate(bool bRanPrior) 
{
    constexpr static std::wstring_view CLASS_NAME = L"ClassicTileWndClass";

    ClassicTileRegUtil::GetRegLogging(m_bLogging);
    if (m_bLogging) {
        EnableLogging();
    }

    if (bRanPrior) {
        CloseTaskDlg();
        if (m_hWnd) {
            m_bQuit = false;
            try {
                OnClose(m_hWnd);
            } catch (...) {
                m_bQuit = true;
                throw;
            }
            m_bQuit = true;

            m_hMenu.reset();
            m_hPopupMenu = nullptr;

            ::ZeroMemory(&m_niData, sizeof(m_niData));
        }
    }else {
        try {
            eval_warn_nz(::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
        } catch (const LoggingException& le) {
            le.Log();
        } catch (...) {
            log_error("Unhandled exception");
        }

        ::InitCommonControls();
    }

    LONG lReturnValue = ClassicTileRegUtil::GetRegLeftClickAction(m_nLeftClick);
    if (lReturnValue != ERROR_SUCCESS) {
        m_nLeftClick = ID_FILE_CASCADEWINDOWS;
    }

    m_bAutoStart = (ClassicTileRegUtil::CheckRegRun() == ERROR_SUCCESS);

    ClassicTileRegUtil::GetRegDefWndTile(m_bDefWndTile);

    m_wcex.lpszClassName = CLASS_NAME.data();

    return true;
}

bool ClassicTileWnd::AfterWndCreate(bool bRanPrior)
{
    eval_fatal_nz(AddTrayIcon(m_hWnd));

    m_hMenu.reset(eval_error_nz(::LoadMenuW(m_hInst, MAKEINTRESOURCEW(IDR_MENUPOPUP))));
    m_hPopupMenu = eval_error_nz(::GetSubMenu(m_hMenu.get(), 0));
    eval_error_nz(CTWinUtils::SetSubMenuDataFromItemData(m_hPopupMenu));

    log_info("ClassicTileCascade starting.");
    
    return true;
}

LRESULT ClassicTileWnd::ClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    const static UINT WM_TASKBARCREATED =  ::RegisterWindowMessageW(L"TaskbarCreated");

    switch (uMsg)
    {
        HANDLE_MSG(hwnd, SWM_TRAYMSG, OnSWMTrayMsg);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_CLOSE, OnClose);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
    default:
        if (uMsg == WM_TASKBARCREATED) {
            try {
                eval_fatal_nz(AddTrayIcon(hwnd));
            } catch (const LoggingException& le) {
                le.Log();
                return -1;
            } catch (...) {
                log_error("Unhandled exception in CTWWndProc");
                return -1;
            }
            return 0;
        }
        break;
    }

    return __super::ClassWndProc(hwnd, uMsg, wParam, lParam);
}

bool ClassicTileWnd::AddTrayIcon(HWND hWnd)
{
    try {
        m_niData.cbSize = sizeof(NOTIFYICONDATA);
        m_niData.uVersion = NOTIFYICON_VERSION_4;
        m_niData.uID = TRAYICONID;
        m_niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
        eval_fatal_es(::LoadIconMetric(m_hInst, MAKEINTRESOURCE(IDI_CLASSICTILECASCADE), LIM_SMALL, &m_niData.hIcon));
        m_niData.hWnd = hWnd;
        m_niData.uCallbackMessage = SWM_TRAYMSG;

        GetToolTip();

        eval_fatal_nz(::Shell_NotifyIconW(NIM_ADD, &m_niData));

        eval_fatal_nz(::Shell_NotifyIconW(NIM_SETVERSION, &m_niData));
    }catch (const LoggingException& le) {
        le.Log();
        return false;
    }catch (...) {
        log_error("Unhandled exception");
        return false;
    }
    return true;
}


void ClassicTileWnd::CloseTaskDlg()
{
    if (m_hwndTaskDlg) {
        ::SendMessageW(m_hwndTaskDlg, TDM_CLICK_BUTTON, IDOK, 0);
    }
}

void ClassicTileWnd::OnSWMTrayMsg(HWND hwnd, WORD wNotifEvent, WORD wIconId, int x, int y)
{
    bool bHandled = false;
    if (wIconId == TRAYICONID) {
        switch (wNotifEvent)
        {
        case NIN_SELECT:
        case NIN_KEYSELECT:
            CloseTaskDlg();
            OnCommand(hwnd, m_nLeftClick, 0, 0);
            bHandled = true;
            break;

        case WM_CONTEXTMENU:
            OnContextMenu(hwnd, nullptr, x, y);
            bHandled = true;
            break;

        }
    }

    if (!bHandled) {
        FORWARD_SWM_TRAYMSG(hwnd, wNotifEvent, wIconId, x, y, __super::ClassWndProc);
    }
}

void ClassicTileWnd::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    using TILE_CASCADE_FUNC = WORD(WINAPI*)(HWND, UINT, const RECT*, UINT, const HWND*);
    
    auto TileCascadeHelper = [this](TILE_CASCADE_FUNC pTileCascadeFunc, UINT uHow){
        HwndVector hwndVector;
        if (!m_bDefWndTile && EnumWindows(s_EnumProc, reinterpret_cast<LPARAM>(&hwndVector)) && (hwndVector.size() > 0)) {
            (*pTileCascadeFunc)(nullptr, uHow, nullptr, static_cast<UINT>(hwndVector.size()), hwndVector.data());
        } else {
            (*pTileCascadeFunc)(nullptr, uHow, nullptr, 0, nullptr);
        }
    };

    switch (id)
    {
    case ID_FILE_CASCADEWINDOWS:
        TileCascadeHelper(&CascadeWindows, MDITILE_ZORDER);
        break;

    case ID_FILE_SHOWWINDOWSSTACKED:
        TileCascadeHelper(&TileWindows, MDITILE_HORIZONTAL);
        break;

    case ID_FILE_SHOWWINDOWSSIDEBYSIDE:
        TileCascadeHelper(&TileWindows, MDITILE_VERTICAL);
        break;

    case ID_FILE_SHOWTHEDESKTOP:
        CTWinUtils::ShellHelper(&IShellDispatch::MinimizeAll);
        break;

    case ID_FILE_UNDOMINIMIZE:
        CTWinUtils::ShellHelper(&IShellDispatch::UndoMinimizeALL);
        break;

    case IDM_EXIT:
        ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
        break;

    case ID_FILE_HELP:
        OnHelp(hwnd);
        break;

    case ID_DEFAULT_CASCADEWINDOWS:
    case ID_DEFAULT_SHOWTHEDESKTOP:
    case ID_DEFAULT_UNDOMINIMIZE:
    case ID_DEFAULT_SHOWWINDOWSSTACKED:
    case ID_DEFAULT_SHOWWINDOWSSIDEBYSIDE:
        OnChangeDefault(id);
        break;

    case ID_SETTINGS_AUTOSTART:
        OnSettingsAutoStart();
        break;

    case ID_SETTINGS_LOGGING:
        OnSettingsLogging(hwnd);
        break;

    case ID_SETTINGS_DEFWNDTILE:
        OnSettingsDefWndTile();
        break;

    case ID_SETTINGS_OPENLOGFILE:
        OnSettingsOpenLogFile(hwnd, CTGlobals::LOG_PATH);
        break;

    default:
        FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, __super::ClassWndProc);
        break;
    }


}

void ClassicTileWnd::OnClose(HWND hwnd)
{

    if (m_logViewer.GetHWND()) {
        ::SendMessageW(m_logViewer.GetHWND(), WM_CLOSE, 0, 0);
    }

    ::HtmlHelpW(nullptr, nullptr, HH_CLOSE_ALL, 0);
    ::Sleep(100);

    __super::OnClose(hwnd);
}

void ClassicTileWnd::OnDestroy(HWND hwnd)
{
    try {
        if (m_niData.hIcon && eval_error_nz(::DestroyIcon(m_niData.hIcon))) {
            m_niData.hIcon = NULL;
        }
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }

    try {
        m_niData.uFlags = 0;
        eval_error_nz(::Shell_NotifyIconW(NIM_DELETE, &m_niData));
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }

    if (m_bQuit) {
        log_info("ClassicTileCascade ending.");
    }

    __super::OnDestroy(hwnd);
}

void ClassicTileWnd::OnContextMenu(HWND hwnd, HWND, UINT xPos, UINT yPos) 
{
    try {
        CloseTaskDlg();
        
        eval_error_nz(::SetForegroundWindow(hwnd));
        eval_error_nz(::TrackPopupMenu(m_hPopupMenu, TPM_BOTTOMALIGN, xPos, yPos, 0, hwnd, nullptr));
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}

void ClassicTileWnd::OnHelp(HWND hwnd) const
{
    try {
        const static std::wstring CHM_PATH = []() {
            constexpr static std::wstring_view CHM_FILE_NAME = L"ClassicTileCascadeHelp.chm";
            std::wstring szExePath = CTGlobals::CURR_MODULE_PATH;

            eval_error_es(::PathCchRemoveFileSpec(sz_wbuf(szExePath), szExePath.size()));

            std::wstring szHelpPath;
            CTWinUtils::PathCombineEx(szHelpPath, static_cast<std::wstring_view>(szExePath), CHM_FILE_NAME);
            return szHelpPath;
        }();

        eval_error_nz(::HtmlHelpW(GetDesktopWindow(), CHM_PATH.c_str(), HH_DISPLAY_TOPIC, 0));
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}

void ClassicTileWnd::OnChangeDefault(int id)
{
    try{
        m_nLeftClick = FindMenuId2MenuItem(MenuId2MenuItemDir::Default2FileMap, id).uFile;
        eval_error_es(ClassicTileRegUtil::SetRegLeftClickAction(m_nLeftClick));
        GetToolTip();
        eval_error_nz(::Shell_NotifyIconW(NIM_MODIFY, &m_niData));
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }

}


void ClassicTileWnd::OnSettingsAutoStart()
{
    try{
        m_bAutoStart = !m_bAutoStart;
        if (m_bAutoStart) {
            eval_error_es(ClassicTileRegUtil::SetRegRun());
        }else {
            eval_error_es(ClassicTileRegUtil::DeleteRegRun());
        }
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}

void ClassicTileWnd::OnSettingsLogging(HWND hwnd)
{
    static constexpr std::wstring_view MSG_BOX_MAIN = L"Logging enabled.";
    
    static constexpr wchar_t MSG_BOX_FMT_LOGGING[] = L"Log files are stored at: <A HREF=\"{0}\">{0}</A>.";
    static const std::wstring MSG_BOX_CONTENT = std::format(MSG_BOX_FMT_LOGGING, CTGlobals::LOG_PATH);

    try {
        m_bLogging = !m_bLogging;

        eval_error_es(ClassicTileRegUtil::SetRegLogging(m_bLogging));

        if (m_bLogging) {
            if (log_find_fp(m_pLogFP.get(), LOG_TRACE) != 0) {
                EnableLogging();
                log_info("Logging enabled.");
            }

            TASKDIALOGCONFIG tdc = { 0 };
            tdc.cbSize = sizeof(tdc);
            tdc.hwndParent = hwnd;
            tdc.hInstance = m_hInst;
            tdc.dwCommonButtons = TDCBF_OK_BUTTON;
            tdc.pszWindowTitle = APP_NAME.data();
            tdc.pszMainIcon = TD_INFORMATION_ICON;
            tdc.pszMainInstruction = MSG_BOX_MAIN.data();
            tdc.pfCallback = s_TaskDlgProc;
            tdc.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
            tdc.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ENABLE_HYPERLINKS;
            tdc.pszContent = MSG_BOX_CONTENT.c_str();

            eval_error_es(::TaskDialogIndirect(&tdc, nullptr, nullptr, nullptr));
        }else{
            if (log_find_fp(m_pLogFP.get(), LOG_TRACE) == 0) {
                log_info("Disabling logging");
                eval_error_es(log_remove_fp(m_pLogFP.get(), LOG_TRACE));
            }

        }
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}

void ClassicTileWnd::OnSettingsDefWndTile()
{
    try{
        m_bDefWndTile = !m_bDefWndTile;

        eval_error_es(ClassicTileRegUtil::SetRegDefWndTile(m_bDefWndTile));
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}


void ClassicTileWnd::GetToolTip()
{
    const static std::wstring TIP_FMT = std::wstring(APP_NAME) + L"\r\nLeft-click: %s";

    try {
        const std::wstring& szLeftClick = FindMenuId2MenuItem(MenuId2MenuItemDir::File2DefaultMap, m_nLeftClick).szFileString;

        ::swprintf_s(m_niData.szTip, _countof(m_niData.szTip), TIP_FMT.c_str(), szLeftClick.c_str());
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}

HRESULT CALLBACK ClassicTileWnd::s_TaskDlgProc(HWND hwnd, UINT msg, WPARAM, LPARAM lParam, LONG_PTR lpRefData)
{
    if (lpRefData) {
        ClassicTileWnd* pThis = reinterpret_cast<ClassicTileWnd*>(lpRefData);
        switch (msg) 
        {
        case TDN_CREATED:
            pThis->m_hwndTaskDlg = hwnd;
            break;

        case TDN_DESTROYED:
            pThis->m_hwndTaskDlg = nullptr;
            break;

        case TDN_HYPERLINK_CLICKED:
            pThis->OnHyperlinkClick(lParam);
            break;
        }
    }
    return S_OK;
}




BOOL CALLBACK ClassicTileWnd::s_EnumProc(HWND hwnd, LPARAM lParam)
{
    if (!hwnd) {
        return TRUE;
    }

    DWORD cloaked = 0;
    HRESULT hrTemp = ::DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));

    //See: https://devblogs.microsoft.com/oldnewthing/20071008-00/?p=24863
    //Which windows appear in the Alt+Tab list?
    //Old New Thing blog from October 8th, 2007
    HWND  hwndWalk = nullptr;
    HWND hwndTry = ::GetAncestor(hwnd, GA_ROOTOWNER);
    while (hwndTry != hwndWalk)
    {
        hwndWalk = hwndTry;
        hwndTry = ::GetLastActivePopup(hwndWalk);
        if (::IsWindowVisible(hwndTry))
            break;
    }

    TITLEBARINFO ti = { 0 };
    ti.cbSize = sizeof(ti);
    ::GetTitleBarInfo(hwnd, &ti);

    //Get maximized windows that would appear in ALT-TAB window and Task Manager "Apps"
    if (    ::IsWindowVisible(hwnd) 
            && (hwndWalk == hwnd)
            && (::GetShellWindow() != hwnd) //exclude desktop
            && ((::GetWindowLong(hwnd, GWL_STYLE) & WS_DISABLED) != WS_DISABLED) 
            && ((::GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) != WS_EX_TOOLWINDOW) //exclude tool windows
            && (cloaked != DWM_CLOAKED_SHELL)
            && (::GetWindowTextLengthW(hwnd) > 0)
            && ((ti.rgstate[0] & STATE_SYSTEM_INVISIBLE) != STATE_SYSTEM_INVISIBLE)
            && ! ::IsIconic(hwnd)
        )
    {
        HwndVector* pHwndVector = reinterpret_cast<HwndVector*>(lParam);
        pHwndVector->push_back(hwnd);
    }
    return TRUE;
}


const ClassicTileWnd::File2DefaultStruct& ClassicTileWnd::FindMenuId2MenuItem(MenuId2MenuItemDir menuID2MenuItemDir, UINT uSought)
{
    using MenuId2MenuItem = std::map<UINT, File2DefaultStruct>;
    using MenuId2MenuItemPair = std::pair<UINT, File2DefaultStruct>;

    const static MenuId2MenuItem Default2FileMap_ = {
        {ID_DEFAULT_CASCADEWINDOWS, File2DefaultStruct(ID_DEFAULT_CASCADEWINDOWS,ID_FILE_CASCADEWINDOWS)},
        {ID_DEFAULT_SHOWTHEDESKTOP, File2DefaultStruct(ID_DEFAULT_SHOWTHEDESKTOP,ID_FILE_SHOWTHEDESKTOP)},
        {ID_DEFAULT_SHOWWINDOWSSIDEBYSIDE, File2DefaultStruct(ID_DEFAULT_SHOWWINDOWSSIDEBYSIDE, ID_FILE_SHOWWINDOWSSIDEBYSIDE)},
        {ID_DEFAULT_SHOWWINDOWSSTACKED,File2DefaultStruct(ID_DEFAULT_SHOWWINDOWSSTACKED,ID_FILE_SHOWWINDOWSSTACKED)},
        {ID_DEFAULT_UNDOMINIMIZE, File2DefaultStruct(ID_DEFAULT_UNDOMINIMIZE,ID_FILE_UNDOMINIMIZE)}
    };


    const static MenuId2MenuItem File2DefaultMap_ = 
        Default2FileMap_ |
        std::views::transform([](const MenuId2MenuItemPair& f2ds) {return std::make_pair(f2ds.second.uFile, f2ds.second); }) |
        std::ranges::to<std::map>();

    const MenuId2MenuItem* pMenuID2MenuItem = nullptr;

    switch (menuID2MenuItemDir) {
    case MenuId2MenuItemDir::Default2FileMap:
        pMenuID2MenuItem = &Default2FileMap_;
        break;

    case MenuId2MenuItemDir::File2DefaultMap:
        pMenuID2MenuItem = &File2DefaultMap_;
        break;

    default:
        generate_error("Unknown MenuId2MenuItemDir: " + std::to_string(static_cast<UINT>(menuID2MenuItemDir)));
        break;
    }

    MenuId2MenuItem::const_iterator it = pMenuID2MenuItem->find(uSought);

    if (it == pMenuID2MenuItem->end()) {
        generate_error("No MenuId2MenuItem entry: " + std::to_string(uSought));
    }
    return it->second;
}

ClassicTileWnd::File2DefaultStruct::File2DefaultStruct(UINT uDefault_, UINT uFile_)
    :uDefault(uDefault_),
     uFile(uFile_)
{
    try {
        SPHMENU hMenu(eval_error_nz(::LoadMenuW(::GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDR_MENUPOPUP))));

        HMENU hPopupMenu = eval_error_nz(::GetSubMenu(hMenu.get(), 0));

        eval_error_nz(CTWinUtils::GetMenuStringSubMenu(hPopupMenu, uFile, false, szFileString));

        std::erase(szFileString, L'&');
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }

}

void ClassicTileWnd::OnHyperlinkClick(LPARAM lpszURL) 
{
    try {
        OnSettingsOpenLogFile(m_niData.hWnd, eval_error_nz(reinterpret_cast<LPCWSTR>(lpszURL)));
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }

}

void ClassicTileWnd::OnInitMenuPopup(HWND hWnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    bool bHandled = false;

    MENUINFO mi = { 0 };
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_MENUDATA;

    if (
            !fSystemMenu && 
            ::GetMenuInfo(hMenu, &mi) && 
            mi.dwMenuData
        )
    {
        switch (mi.dwMenuData) {
        case ID_SETTINGS:
            OnSettingsPopup(hMenu);
            bHandled = true;
            break;

        case ID_DEFAULT:
            OnLeftClickDoesPopup(hMenu);
            bHandled = true;
            break;
        }
    }

    if (!bHandled) {
        FORWARD_WM_INITMENUPOPUP(hWnd, hMenu, item, fSystemMenu, __super::ClassWndProc);
    }
}

void ClassicTileWnd::OnSettingsPopup(HMENU hMenu)
{
    try {
        eval_error_nz(CTWinUtils::CheckMenuItem(hMenu, ID_SETTINGS_AUTOSTART, m_bAutoStart));
        eval_error_nz(CTWinUtils::CheckMenuItem(hMenu, ID_SETTINGS_DEFWNDTILE, m_bDefWndTile));
        eval_error_nz(CTWinUtils::CheckMenuItem(hMenu, ID_SETTINGS_LOGGING, m_bLogging));

        eval_error_nz(::EnableMenuItem(hMenu, ID_SETTINGS_OPENLOGFILE, MF_BYCOMMAND | (CTWinUtils::FileExists(CTGlobals::LOG_PATH) ? MF_ENABLED : MF_GRAYED)) >= 0);
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}

void ClassicTileWnd::OnLeftClickDoesPopup(HMENU hMenu)
{
    const static auto MENU_MIN_MAX = [hMenu]() {
        std::vector<UINT> menuVect(::GetMenuItemCount(hMenu));
        std::ranges::generate(menuVect, [hMenu, i=0u]() mutable {return ::GetMenuItemID(hMenu, i++);});
        return std::ranges::minmax(menuVect);
    }();


    try{
        eval_error_nz(::CheckMenuRadioItem(hMenu, MENU_MIN_MAX.min, MENU_MIN_MAX.max, FindMenuId2MenuItem(MenuId2MenuItemDir::File2DefaultMap, m_nLeftClick).uDefault, MF_BYCOMMAND));
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}

void ClassicTileWnd::EnableLogging()
{
    if (!enable_logging(CTGlobals::LOG_PATH, m_pLogFP)) {
        generate_fatal("Invalid log file stream.");
    }
}


bool ClassicTileWnd::Run(HINSTANCE hInst)
{
    return s_classicTileWnd.InitInstance(hInst);
}

void ClassicTileWnd::OnSettingsOpenLogFile(HWND hwnd, std::wstring_view szPath)
{
    try {
        constexpr static wchar_t FMT_FILE_NOT_FOUND[] = L"Log file <{0}> does not exist yet.";

        if (!CTWinUtils::FileExists(szPath)) {
            ::MessageBoxW(hwnd, std::format(FMT_FILE_NOT_FOUND, szPath).c_str(), APP_NAME.data(), MB_OK | MB_ICONINFORMATION);
        } else if (m_logViewer.GetHWND() ){
            if (::IsIconic(m_logViewer.GetHWND())) {
                eval_error_nz(::ShowWindow(m_logViewer.GetHWND(), SW_RESTORE));
            }
            eval_error_nz(m_logViewer.SetFile(szPath));
            eval_error_nz(::SetForegroundWindow(m_logViewer.GetHWND()));
        }else{
            eval_error_nz(m_logViewer.InitInstance(m_hInst, szPath));
        }
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}

bool ClassicTileWnd::ProcessDlgMsg(LPMSG lpMsg)
{
    bool bRetVal = false;
    if (m_logViewer.GetHWND()) {
        bRetVal = m_logViewer.ProcessDlgMsg(lpMsg);
    }

    return bRetVal;
}

bool ClassicTileWnd::CTWProcessDlgMsg(LPMSG lpMsg)
{
    return s_classicTileWnd.ProcessDlgMsg(lpMsg);
}