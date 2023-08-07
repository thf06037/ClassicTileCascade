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
#include "resource.h"
#include "ClassicTileRegUtil.h"
#include "MemMgmt.h"
#include "WinUtils.h"
#include "ClassicTileWnd.h"

#define SWM_TRAYMSG	WM_APP //the message ID sent to our window

//Message Handlers
/* BOOL Cls_OnSWMTrayMsg(HWND hwnd, WORD wNotifEvent, WORD wIconId, int x, int y) */
#define HANDLE_SWM_TRAYMSG(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), LOWORD(lParam), HIWORD(lParam), GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)), 0L)
#define FORWARD_SWM_TRAYMSG(hwnd, wNotifEvent, wIconId, x, y, fn) \
	(void)(fn)((hwnd), SWM_TRAYMSG, MAKEWPARAM((x), (y)), MAKELPARAM((wNotifEvent), (wIconId)))

const UINT ClassicTileWnd::WM_TASKBARCREATED = ::RegisterWindowMessageW(L"TaskbarCreated");
const std::wstring ClassicTileWnd::CLASS_NAME = L"ClassicTileWndClass";

const ClassicTileWnd::MenuId2MenuItem ClassicTileWnd::Default2FileMap = {
    {ID_DEFAULT_CASCADEWINDOWS, File2DefaultStruct(ID_DEFAULT_CASCADEWINDOWS,ID_FILE_CASCADEWINDOWS)},
    {ID_DEFAULT_SHOWTHEDESKTOP, File2DefaultStruct(ID_DEFAULT_SHOWTHEDESKTOP,ID_FILE_SHOWTHEDESKTOP)},
    {ID_DEFAULT_SHOWWINDOWSSIDEBYSIDE, File2DefaultStruct(ID_DEFAULT_SHOWWINDOWSSIDEBYSIDE, ID_FILE_SHOWWINDOWSSIDEBYSIDE)},
    {ID_DEFAULT_SHOWWINDOWSSTACKED,File2DefaultStruct(ID_DEFAULT_SHOWWINDOWSSTACKED,ID_FILE_SHOWWINDOWSSTACKED)},
    {ID_DEFAULT_UNDOMINIMIZE, File2DefaultStruct(ID_DEFAULT_UNDOMINIMIZE,ID_FILE_UNDOMINIMIZE)}
};


const ClassicTileWnd::MenuId2MenuItem ClassicTileWnd::File2DefaultMap = 
        ClassicTileWnd::Default2FileMap | 
                std::views::transform([](const MenuId2MenuItemPair& f2ds) {return std::make_pair(f2ds.second.uFile, f2ds.second); }) | 
                std::ranges::to<std::map>();

const std::wstring ClassicTileWnd::CURR_MODULE_PATH = []() {
    std::wstring szCurrModulePath;
    CTWinUtils::GetCurrModuleFileName(szCurrModulePath);
    return szCurrModulePath;
}();

const std::wstring ClassicTileWnd::LOG_PATH = []() {
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

const std::string ClassicTileWnd::LOG_PATH_NARROW = []() {
    std::string szLogPathNarrow;
    CTWinUtils::Wstring2string(szLogPathNarrow, LOG_PATH);
    return szLogPathNarrow;
}();

ClassicTileWnd::ClassicTileWnd()
{
    m_pLogFP.reset(_fsopen(LOG_PATH_NARROW.c_str(), "a+", _SH_DENYWR));
}

bool ClassicTileWnd::InitInstance(HINSTANCE hInstance)
{
    try {
        ClassicTileRegUtil::GetRegLogging(m_bLogging);
        if (m_bLogging) {
            if (m_pLogFP) {
                log_add_fp(m_pLogFP.get(), LOG_TRACE);
            }else{
                generate_fatal("Invalid log file stream.");
            }
        }

        try {
            eval_warn_nz(::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
        }catch (const LoggingException& le) {
            le.Log();
        }catch (...) {
            log_error("Unhandled exception");
        }

        ::InitCommonControls();

        m_hInst = hInstance; 

        LONG lReturnValue = ClassicTileRegUtil::GetRegLeftClickAction(m_nLeftClick);
        if (lReturnValue != ERROR_SUCCESS) {
            m_nLeftClick = ID_FILE_CASCADEWINDOWS;
        }

        m_bAutoStart = (ClassicTileRegUtil::CheckRegRun() == ERROR_SUCCESS);

        ClassicTileRegUtil::GetRegDefWndTile(m_bDefWndTile);

        WNDCLASSEXW wcex = { 0 };
        wcex.cbSize = sizeof(wcex);
        wcex.lpfnWndProc = s_WndProc;
        wcex.hInstance = m_hInst;
        wcex.lpszClassName = CLASS_NAME.c_str();

        eval_fatal_nz(::RegisterClassExW(&wcex));

        HWND hWnd = eval_fatal_nz(::CreateWindowW(CLASS_NAME.c_str(), nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, m_hInst, this));

        eval_fatal_nz(AddTrayIcon(hWnd));

        log_info("ClassicTileCascade starting.");
    }catch (const LoggingException& le) {
        le.Log();
        return false;
    }catch (...) {
        log_error("Unhandled exception");
        return false;
    }

    return true;
}

LRESULT ClassicTileWnd::CTWWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_TASKBARCREATED) {
        if (!CheckTrayIcon()) {
            try {
                eval_fatal_nz(AddTrayIcon(hwnd));
            }catch (const LoggingException& le) {
                le.Log();
                return -1;
            }catch (...) {
                log_error("Unhandled exception in CTWWndProc");
                return -1;
            }
        }
        return 0;
    }

    switch (uMsg)
    {
        HANDLE_MSG(hwnd, SWM_TRAYMSG, OnSWMTrayMsg);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_CLOSE, OnClose);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
    }

    return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
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

bool ClassicTileWnd::CheckTrayIcon() const
{
    NOTIFYICONIDENTIFIER NID = { 0 };
    NID.cbSize = sizeof(NOTIFYICONIDENTIFIER);
    NID.hWnd = m_niData.hWnd;
    NID.uID = m_niData.uID;
    RECT r = { 0 };
    HRESULT hr = ::Shell_NotifyIconGetRect(&NID, &r);
    return SUCCEEDED(hr);
}

LRESULT CALLBACK ClassicTileWnd::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static bool s_bWMCreateCalled = false;
    if (uMsg == WM_CREATE) {
        s_bWMCreateCalled = true;
        LPCREATESTRUCTW pCW = reinterpret_cast<LPCREATESTRUCTW>(lParam);

        SetLastError(0);
        if (! ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCW->lpCreateParams))) {
            DWORD dwErr = ::GetLastError();
            if (dwErr != 0) {
                return -1;
            }
        }
    }

    ClassicTileWnd* self = reinterpret_cast<ClassicTileWnd*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!self) {
        if (s_bWMCreateCalled) {
            return -1;
        }else {
            return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
    }

    return self->CTWWndProc(hwnd, uMsg, wParam, lParam);
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
        FORWARD_SWM_TRAYMSG(hwnd, wNotifEvent, wIconId, x, y, DefWindowProcW);
    }
}

void ClassicTileWnd::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
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
        OpenTextFile(hwnd, LOG_PATH);
        break;

    default:
        FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, DefWindowProcW);
        break;
    }


}

void ClassicTileWnd::OnClose(HWND hwnd)
{
    ::HtmlHelpW(nullptr, nullptr, HH_CLOSE_ALL, 0);
    ::Sleep(100);

    ::DestroyWindow(hwnd);
}

void ClassicTileWnd::OnDestroy(HWND)
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

    log_info("ClassicTileCascade ending.");

    ::PostQuitMessage(0);
}

void ClassicTileWnd::OnContextMenu(HWND hwnd, HWND, UINT xPos, UINT yPos) 
{
    const static std::wstring SETTINGS_STRING = L"&Settings";
    const static std::wstring DEFAULTS_STRING = L"&Left click does";

    const static auto MIN_MAX_DEFAULT = std::ranges::minmax_element(Default2FileMap | std::views::keys);

    SPHMENU hMenu;
    try {
        CloseTaskDlg();
        hMenu.reset(eval_error_nz(::LoadMenuW(m_hInst, MAKEINTRESOURCEW(IDR_MENUPOPUP))));

        HMENU hPopupMenu = eval_error_nz(::GetSubMenu(hMenu.get(), 0));
        
        const static UINT SETTING_POS = CTWinUtils::GetSubMenuPosByString(hPopupMenu, SETTINGS_STRING);

        HMENU hSettingsMenu = eval_error_nz(::GetSubMenu(hPopupMenu, SETTING_POS ));
        eval_error_nz(CTWinUtils::CheckMenuItem(hSettingsMenu, ID_SETTINGS_AUTOSTART, m_bAutoStart));
        eval_error_nz(CTWinUtils::CheckMenuItem(hSettingsMenu, ID_SETTINGS_DEFWNDTILE, m_bDefWndTile));
        eval_error_nz(CTWinUtils::CheckMenuItem(hSettingsMenu, ID_SETTINGS_LOGGING, m_bLogging));

        const static UINT DEFAULTS_POS = CTWinUtils::GetSubMenuPosByString(hSettingsMenu, DEFAULTS_STRING);

        HMENU hDefaultsMenu = eval_error_nz(::GetSubMenu(hSettingsMenu, DEFAULTS_POS));
        eval_error_nz(::CheckMenuRadioItem(hDefaultsMenu, *MIN_MAX_DEFAULT.min, *MIN_MAX_DEFAULT.max, FindMenuId2MenuItem(File2DefaultMap, m_nLeftClick).uDefault, MF_BYCOMMAND));

        eval_error_nz(::SetForegroundWindow(hwnd));
        eval_error_nz(::TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN, xPos, yPos, 0, hwnd, nullptr));
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}

void ClassicTileWnd::TileCascadeHelper(TILE_CASCADE_FUNC pTileCascadeFunc, UINT uHow) const
{
    HwndVector hwndVector;
    if (!m_bDefWndTile && EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&hwndVector)) && (hwndVector.size() > 0)) {
        (*pTileCascadeFunc)(nullptr, uHow, nullptr, static_cast<UINT>(hwndVector.size()), hwndVector.data());
    }
    else {
        (*pTileCascadeFunc)(nullptr, uHow, nullptr, 0, nullptr);
    }
}

void ClassicTileWnd::OnHelp(HWND hwnd) const
{
    try {
        const static std::wstring CHM_PATH = []() {
            const static std::wstring CHM_FILE_NAME = L"ClassicTileCascadeHelp.chm";
            std::wstring szExePath = CURR_MODULE_PATH;

            eval_error_es(::PathCchRemoveFileSpec(sz_wbuf(szExePath), szExePath.size()));

            std::wstring szHelpPath;
            CTWinUtils::PathCombineEx(szHelpPath, szExePath, CHM_FILE_NAME);
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
        m_nLeftClick = FindMenuId2MenuItem(Default2FileMap, id).uFile;
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
    static const std::wstring MSG_BOX_MAIN = L"Logging enabled.";
    
    static constexpr wchar_t MSG_BOX_FMT_LOGGING[] = L"Log files are stored at: <A HREF=\"{0}\">{0}</A>.";
    static const std::wstring MSG_BOX_CONTENT = std::format(MSG_BOX_FMT_LOGGING, LOG_PATH);

    try {
        m_bLogging = !m_bLogging;

        eval_error_es(ClassicTileRegUtil::SetRegLogging(m_bLogging));

        if (m_bLogging) {
            if (log_find_fp(m_pLogFP.get(), LOG_TRACE) != 0) {
                eval_error_es(log_add_fp(m_pLogFP.get(), LOG_TRACE));
                log_info("Logging enabled.");
            }

            TASKDIALOGCONFIG tdc = { 0 };
            tdc.cbSize = sizeof(tdc);
            tdc.hwndParent = hwnd;
            tdc.hInstance = m_hInst;
            tdc.dwCommonButtons = TDCBF_OK_BUTTON;
            tdc.pszWindowTitle = L"Classic Tile Cascade";
            tdc.pszMainIcon = TD_INFORMATION_ICON;
            tdc.pszMainInstruction = MSG_BOX_MAIN.c_str();
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
    const static std::wstring TIP_FMT = L"Classic Tile & Cascade\r\nLeft-click: %s";

    try {
        std::wstring szLeftClick = FindMenuId2MenuItem(File2DefaultMap, m_nLeftClick).szFileString;

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




BOOL CALLBACK ClassicTileWnd::EnumProc(HWND hwnd, LPARAM lParam)
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

const ClassicTileWnd::File2DefaultStruct& ClassicTileWnd::FindMenuId2MenuItem(const MenuId2MenuItem& menuID2MenuItem, UINT uSought)
{
    MenuId2MenuItem::const_iterator it = menuID2MenuItem.find(uSought);
    if (it == menuID2MenuItem.end()) {
        generate_error("No MenuId2MenuItem entry: " + std::to_string(uSought));
    }
    return it->second;
}


ClassicTileWnd::File2DefaultStruct::File2DefaultStruct(UINT uDefault_, UINT uFile_)
    :uDefault(uDefault_),
     uFile(uFile_)
{
    try {
        SPHMENU hMenu;
        hMenu.reset(eval_error_nz(::LoadMenuW(::GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDR_MENUPOPUP))));

        HMENU hPopupMenu = eval_error_nz(::GetSubMenu(hMenu.get(), 0));

        eval_error_nz(CTWinUtils::GetMenuStringSubMenu(hPopupMenu, uFile, false, szFileString));

        std::erase(szFileString, L'&');
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }

}


bool ClassicTileWnd::RegUnReg(bool& fSuccess)
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
            log_add_fp(m_pLogFP.get(), LOG_TRACE);

            fRetVal = true;

            DWORD dwProcId = ::GetCurrentProcessId();

            log_info("ProcID <%u>: <%S> command line argument passed.", dwProcId, szFirstArg.c_str());

            std::transform(szFirstArg.begin(), szFirstArg.end(), szFirstArg.begin(), [](auto c) { return std::toupper(c); });
            
            if ( (szFirstArg == REG) || (szFirstArg == UNREG) ) {
                //our process may be running as an elevated user. we need the registry entries to be added in
                //a process that is running as the logged in user

                fSuccess = RegUnRegAsUser(szFirstArg);
            } else if ( (szFirstArg == REGUSER) || (szFirstArg == UNREGUSER) ) {
                log_info("ProcID <%u>: <%S> parameter passed - starting register/unregister process", dwProcId, szFirstArg.c_str());

                fSuccess = Unregister();

                if (fSuccess && (szFirstArg == REGUSER) ) {
                    fSuccess = Register();
                }
            }else{
                log_fatal("ProcID <%u>: <%S> unrecognized command line argument.", dwProcId, szFirstArg.c_str());
                fSuccess = false;
            }
        }
    }

    return fRetVal;

}

bool ClassicTileWnd::RegUnRegAsUser(const std::wstring& szFirstArg)
{
    DWORD dwProcId = ::GetCurrentProcessId();
    bool fSuccess = false;
    try {
        std::wstring szRegUser = szFirstArg + L"USER";
        log_info("ProcID <%u>: Processs running at %s level.", dwProcId, IsUserAnAdmin() ? "elevated" : "regular");
        log_info("ProcID <%u>: <%S> parameter passed - attempting to launch app as logged in user with command line argument <%S>",
            dwProcId, szFirstArg.c_str(), szRegUser.c_str());

        DWORD dwNewProcID = 0;
        eval_fatal_nz(CTWinUtils::ShellExecInExplorerProcess(CURR_MODULE_PATH, szRegUser, &dwNewProcID));

        log_info("ProcID <%u>: Launched app as standard user with ProcID <%u>", dwProcId, dwNewProcID);

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

bool ClassicTileWnd::Unregister()
{
    DWORD dwProcId = ::GetCurrentProcessId();

    log_info("ProcID <%u>: Starting unregister function", dwProcId);

    bool fSuccess = false;

    try {
        LONG lReturnValue = ClassicTileRegUtil::CheckRegAppPath();
        if (lReturnValue == ERROR_SUCCESS) {
            log_info("ProcID <%u>: Application registry key exists, attempting delete", dwProcId);
            eval_error_es(ClassicTileRegUtil::DeleteRegAppPath());
            log_info("ProcID <%u>: Application registry key delete successful", dwProcId);
        }else {
            log_info("ProcID <%u>: Application registry key does not exist.", dwProcId);
        }

        lReturnValue = ClassicTileRegUtil::CheckRegRun();
        if (lReturnValue == ERROR_SUCCESS) {
            log_info("ProcID <%u>: Auto run registry value exists, attempting delete", dwProcId);
            eval_error_es(ClassicTileRegUtil::DeleteRegRun());
            log_info("ProcID <%u>: Auto run registry value key delete successful", dwProcId);
        }else {
            log_info("ProcID <%u>: Auto run registry value does not exist.", dwProcId);
        }
        fSuccess = true;
    }
    catch (const LoggingException& le) {
        le.Log();
        fSuccess = false;
    }
    catch (...) {
        log_error("Unhandled exception");
        fSuccess = false;
    }

    log_info("ProcID <%u>: ClassicTileCascade %s", dwProcId, fSuccess ? "unregistered" : "unregister failed");
    return fSuccess;
}

bool ClassicTileWnd::Register()
{
    DWORD dwProcId = ::GetCurrentProcessId();
    log_info("ProcID <%u>: Starting register process.", dwProcId);
    
    bool fSuccess = false;
    
    try {
        eval_error_es(ClassicTileRegUtil::SetRegLeftClickAction(ID_FILE_CASCADEWINDOWS));
        log_info("ProcID <%u>: Added Left Click Action registry value.", dwProcId);

        eval_error_es(ClassicTileRegUtil::SetRegLogging(0));
        log_info("ProcID <%u>: Added Logging registry value.", dwProcId);

        eval_error_es(ClassicTileRegUtil::SetRegDefWndTile(0));
        log_info("ProcID <%u>: Added Default/Custom Window Tile/Cascade registry value.", dwProcId);

        eval_error_es(ClassicTileRegUtil::SetRegRun());
        log_info("ProcID <%u>: Added Auto Run registry value.", dwProcId);

        log_info("ProcID <%u>: Attempting to start interactive application.", dwProcId);

        DWORD dwNewProcId = 0;
        eval_error_nz(CTWinUtils::CreateProcessHelper(CURR_MODULE_PATH, L"", &dwNewProcId));

        log_info("ProcID <%u>: Interactive application started successfully with ProcID <%u>.", dwProcId, dwNewProcId);

        fSuccess = true;
    } catch (const LoggingException& le) {
        le.Log();
        fSuccess = false;
    } catch (...) {
        log_error("Unhandled exception");
        fSuccess = false;
    }
    log_info("ProcID <%u>: ClassicTileCascade %s", dwProcId, fSuccess ? "registered" : "register failed");
    return fSuccess;
}

void ClassicTileWnd::OnHyperlinkClick(LPARAM lpszURL) const
{
    try {
        const std::wstring szURL = eval_error_nz(reinterpret_cast<LPCWSTR>(lpszURL));
        OpenTextFile(m_niData.hWnd, szURL);
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }

}

void ClassicTileWnd::OpenTextFile(HWND hWnd, const std::wstring& szPath)
{
    constexpr static wchar_t FMT_FILE_NOT_FOUND[] = L"Log file <{0}> does not exist yet.";

    try {
        const static std::wstring NOTEPAD_PATH = [] {
            eval_error_nz(::SetSearchPathMode(BASE_SEARCH_PATH_ENABLE_SAFE_SEARCHMODE));
            std::wstring szNotepadPath;
            sz_wbuf szNotepadPathBuf(szNotepadPath, MAX_PATH);
            eval_error_nz(::SearchPathW(nullptr, L"notepad.exe", nullptr, szNotepadPathBuf.size(), szNotepadPathBuf, nullptr));
            return szNotepadPath;
        }();

        DWORD dwAttrib = ::GetFileAttributes(szPath.c_str());
        if ((dwAttrib == INVALID_FILE_ATTRIBUTES) || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
            ::MessageBoxW(hWnd, std::format(FMT_FILE_NOT_FOUND, szPath).c_str(), L"Classic Tile Cascade", MB_OK | MB_ICONINFORMATION);
        }else {
            std::wstring szResult;
            int nRetVal = reinterpret_cast<int>(::FindExecutableW(szPath.c_str(), NULL, sz_wbuf(szResult, MAX_PATH)));
            if (nRetVal <= 32) {
                szResult = NOTEPAD_PATH;
            }

            std::wstring szPathQuote = szPath;
            CTWinUtils::PathQuoteSpacesW(szPathQuote);
            eval_error_nz(CTWinUtils::CreateProcessHelper(szResult, szPathQuote, nullptr, SW_SHOWMAXIMIZED));
        }
    }catch (const LoggingException& le) {
        le.Log();
    }catch (...) {
        log_error("Unhandled exception");
    }
}