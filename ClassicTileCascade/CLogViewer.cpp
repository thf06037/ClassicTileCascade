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
#include "resource.h"
#include "MemMgmt.h"
#include "WinUtils.h"
#include "win_log.h"
#include "BaseWnd.h"
#include "ClassicTileRegUtil.h"
#include "CLogViewer.h"



CLogViewer::CLogViewer(bool bMainWnd)
	:   BaseWnd(bMainWnd){}

bool CLogViewer::InitInstance(HINSTANCE hInstance, const std::wstring& szFilePath)
{
    m_szFilePath = szFilePath;
    return __super::InitInstance(hInstance);
}

bool CLogViewer::SetFile(const std::wstring& szFilePath)
{
    m_szFilePath = szFilePath;
    return OpenFile();
}

bool CLogViewer::BeforeWndCreate(bool bRanPrior)
{
    const static std::wstring CLASS_NAME = L"ClassicTileWndLogViewer";

    if (!bRanPrior) {
        const static SPHMODULE hRtfLib ( eval_fatal_nz(::LoadLibraryW(L"Msftedit.dll")) );

        m_wcex.style = CS_HREDRAW | CS_VREDRAW;
        m_wcex.hIcon = eval_fatal_nz(::LoadIcon(m_hInst, reinterpret_cast<LPCWSTR>(IDI_CLASSICTILECASCADE)));
        m_wcex.hCursor = eval_fatal_nz(::LoadCursor(NULL, IDC_ARROW));
        m_wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        m_wcex.lpszMenuName = reinterpret_cast<LPCWSTR>(IDC_LOGVIEWER);
        m_wcex.lpszClassName = CLASS_NAME.c_str();
        m_wcex.hIconSm = eval_fatal_nz(::LoadIcon(m_hInst, reinterpret_cast<LPCWSTR>(IDI_CLASSICTILECASCADE)));
    }

    m_szWinTitle = L"Classic Tile Log Viewer";
    m_dwStyle = WS_OVERLAPPEDWINDOW;
    m_x = CW_USEDEFAULT;
    m_nWidth = CW_USEDEFAULT;

    return true;
}

bool CLogViewer::AfterWndCreate(bool bRanPrior)
{
    if (!bRanPrior) {
        m_hAccel = eval_fatal_nz(::LoadAcceleratorsW(m_hInst, MAKEINTRESOURCE(IDC_LOGVIEWER)));
    }

    eval_fatal_nz(CTWinUtils::SetSubMenuDataFromItemData(::GetMenu(m_hWnd)));

    ::ShowWindow(m_hWnd, SW_SHOWNORMAL);

    IRichEditOlePtr spRichEditOle;
    eval_fatal_nz(::SendMessageW(m_hEdit, EM_GETOLEINTERFACE, 0, reinterpret_cast<LPARAM>(&spRichEditOle)));
    eval_fatal_hr( (m_spTextDoc = spRichEditOle) ? S_OK : E_NOINTERFACE);

    OpenFile();

    if (ClassicTileRegUtil::GetRegStatusBar(m_bStatusBar) != ERROR_SUCCESS) {
        m_bStatusBar = false;
    }

    CreateDestroyStatusBar(m_hWnd);

    return true;
}

bool CLogViewer::OpenFile()
{
    bool bRetVal = false;
    
    try {
        _variant_t varFile(m_szFilePath.c_str());

        eval_fatal_hr(m_spTextDoc->Open(&varFile, tomReadOnly | tomOpenExisting | tomText, 0));

        SetLineNumbers(m_hWnd);

        ITextSelectionPtr spTextSelection;
        eval_fatal_hr(m_spTextDoc->GetSelection(&spTextSelection));
        eval_fatal_hr(spTextSelection->EndKey(tomStory, tomMove, nullptr));

        bRetVal = true;
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
    
    return bRetVal;
}

LRESULT CLogViewer::ClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    const static UINT ID_FINDMSGSTRING = RegisterWindowMessage(FINDMSGSTRING);

    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_CLOSE, OnClose);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
        HANDLE_MSG(hwnd, WM_SETFOCUS, OnSetFocus);
        HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);

    default:
        if (uMsg == ID_FINDMSGSTRING) {
            OnFindMsg(hwnd, reinterpret_cast<LPFINDREPLACE>(lParam));
            return 0;
        } else {
            return __super::ClassWndProc(hwnd, uMsg, wParam, lParam);
        }
        break;
    }

    return __super::ClassWndProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CLogViewer::OnNotify(HWND hwnd, int uControl, NMHDR* lpNMHDR)
{
    try{
        if ((lpNMHDR->idFrom == IDC_LOGEDIT) && (lpNMHDR->code == EN_SELCHANGE)) {
            OnSelChange(hwnd);
        } else {
            FORWARD_WM_NOTIFY(hwnd, uControl, lpNMHDR, DefWindowProcW);
        }
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
    return 0;
}

void CLogViewer::OnSelChange(HWND hwnd)
{
    try{
        if (m_hStatus && ::IsWindow(m_hStatus)) {
            ITextSelectionPtr spTextSelection;
            eval_error_hr(m_spTextDoc->GetSelection(&spTextSelection));

            long nEnd = 0;
            eval_error_hr(spTextSelection->GetEnd(&nEnd));

            long    nChar = 0,
                    nCharEnd = 0,
                    nLine = 0,
                    nLineEnd = 0,
                    nLineStartChar = 0,
                    nLineEndChar = 0,
                    nCharOnLine = 0,
                    nCharOnLineEnd = 0;
            
            eval_error_hr(spTextSelection->GetIndex(tomCharacter, &nChar));
            eval_error_hr(spTextSelection->GetIndex(tomLine, &nLine));

            ITextRangePtr spRange;
            eval_error_hr(spTextSelection->GetDuplicate(&spRange));
            eval_error_hr(spRange->SetIndex(tomLine, nLine, 0));
            eval_error_hr(spRange->GetIndex(tomCharacter, &nLineStartChar));

            double fZoom = GetZoom();

            if (nEnd != 0) {
                ITextRangePtr spRangeEnd;
                eval_error_hr(spTextSelection->GetDuplicate(&spRangeEnd));
                eval_error_hr(spRangeEnd->SetIndex(tomCharacter, nEnd, 0));

                eval_error_hr(spRangeEnd->GetIndex(tomCharacter, &nCharEnd));
                eval_error_hr(spRangeEnd->GetIndex(tomLine, &nLineEnd));

                eval_error_hr(spRange->SetIndex(tomLine, nLineEnd, 0));
                eval_error_hr(spRange->GetIndex(tomCharacter, &nLineEndChar));

                nCharOnLine = nChar - nLineStartChar + 1;
                nCharOnLineEnd = nCharEnd - nLineEndChar + 1;
            } else {
                nCharOnLine = 1;
            }

            for (UINT i = 0; i < STATUS_PARTS; i++) {
                std::wstring szPartName;
                std::wstring szCell;
                switch (i) {
                case 0:
                    szPartName = L"Line";

                    if (nLine == nLineEnd) {
                        szCell = std::to_wstring(nLine);
                    } else if ( nLineEnd < nLine) {
                        szCell = std::format(L"{0},{1}", nLineEnd, nLine);
                    } else {
                        szCell = std::format(L"{0}-{1}", nLine, nLineEnd);
                    }

                    break;

                case 1:
                    szPartName = L"Character";

                    if (nChar == nCharEnd) {
                        szCell = std::to_wstring(nChar);
                    } else if (nCharEnd < nChar) {
                        szCell = std::format(L"{0},{1}", nCharEnd, nChar);
                    } else {
                        szCell = std::format(L"{0}-{1}", nChar, nCharEnd);
                    }

                    break;

                case 2:
                    szPartName = L"Line:Character";
                    if (nChar == nCharEnd) {
                        szCell = std::format(L"{0}:{1}", nLine, nCharOnLine);
                    } else if (nCharEnd < nChar) {
                        szCell = std::format(L"{0}:{1},{2}:{3}", nLineEnd, nCharOnLineEnd, nLine, nCharOnLine);
                    } else {
                        szCell = std::format(L"{0}:{1}-{2}:{3}", nLine, nCharOnLine, nLineEnd, nCharOnLineEnd);
                    }

                    break;

                case 3:
                    szPartName = L"Zoom";
                    szCell = std::format( L"{0}%", std::lround(fZoom));
                    break;

                }
                
                std::wstring szCellValue = std::format(L"{0} = {1}", szPartName, szCell);
                eval_error_nz(::SendMessageW(m_hStatus, SB_SETTEXTW, i, reinterpret_cast<LPARAM>(szCellValue.data())));
            }
        }
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}

BOOL CLogViewer::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    BOOL fRetVal = FALSE;
    try{
        
        RECT r = { 0 };
        eval_error_nz( ::GetClientRect(hwnd, &r) );

        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                        WS_VSCROLL | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_READONLY | ES_AUTOHSCROLL | WS_HSCROLL;
    
        m_hEdit = eval_error_nz( ::CreateWindowExW(WS_EX_CLIENTEDGE | ES_EX_ZOOMABLE,
                                    MSFTEDIT_CLASS,
                                    L"",
                                    dwStyle, 
                                    0,
                                    0, 
                                    r.right,
                                    r.bottom, 
                                    hwnd,
                                    reinterpret_cast<HMENU>(IDC_LOGEDIT),
                                    m_hInst,
                                    nullptr));
        ::SendMessageW(m_hEdit, EM_SETEVENTMASK, 0, ENM_SELCHANGE);
        eval_error_nz(::SetWindowSubclass(m_hEdit, s_RESubClass, 0, reinterpret_cast<DWORD_PTR>(this)));
        eval_error_nz(::SetFocus(m_hEdit));
        fRetVal = TRUE;
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

    return fRetVal;
}

void CLogViewer::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    try {
        int iStatusHeight = 0;
        if (m_hStatus && ::IsWindow(m_hStatus)) {
            ::SendMessageW(m_hStatus, WM_SIZE, 0, 0);
            RECT rStatus = { 0 };
            eval_error_nz(::GetWindowRect(m_hStatus, &rStatus));
            iStatusHeight = rStatus.bottom - rStatus.top;
        }

        eval_error_nz( ::SetWindowPos(m_hEdit, hwnd, 0, 0, cx, cy - iStatusHeight, SWP_NOMOVE | SWP_NOZORDER) );
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}


void CLogViewer::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDM_EXIT:
        OnClose(hwnd);
        break;

    case ID_FILE_RELOAD:
        OpenFile();
        break;

    case ID_EDIT_COPY:
        OnCopy(hwnd);
        break;

    case ID_EDIT_SELECTALL:
        OnSetSel(hwnd);
        break;

    case ID_EDIT_FIND:
        OnFind(hwnd);
        break;

    case ID_EDIT_FINDNEXT:
    case ID_EDIT_FINDPREVIOUS:
        OnFindAgain(hwnd, id);
        break;
        
    case ID_ZOOM_ZOOMIN:
    case ID_ZOOM_ZOOMOUT:
        OnZoom(hwnd, id);
        break;

    case ID_EDIT_GOTO:
        OnGoto(hwnd);
        break;

    case ID_VIEW_LINENUMBERS:
        OnLineNumbers(hwnd);
        break;

    case ID_VIEW_STATUSBAR:
        OnStatusBar(hwnd);
        break;

    default:
        FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, DefWindowProcW);
        break;
    }

}

INT_PTR CLogViewer::DoModal(HWND hwnd, UINT uResource)
{
    m_fRecursing = FALSE;
    m_uCurrDlg = uResource;
    
    INT_PTR nRetVal = ::DialogBoxParamW(m_hInst, MAKEINTRESOURCEW(m_uCurrDlg), hwnd, s_DlgFunc, reinterpret_cast<LPARAM>(this));
    m_uCurrDlg = 0;

    return nRetVal;
}

void CLogViewer::OnCopy(HWND hwnd)
{
    try{
        ITextSelectionPtr spTextSelection;
        eval_error_hr(m_spTextDoc->GetSelection(&spTextSelection));
        eval_error_hr(spTextSelection->Copy(nullptr));
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}

void CLogViewer::GetDocRange(ITextRangePtr& spRange)
{
    eval_error_hr(m_spTextDoc->Range(0, 0, &spRange));
    eval_error_hr(spRange->MoveEnd(tomStory, 1, nullptr));
}

void CLogViewer::OnSetSel(HWND hwnd)
{
    try {
        ITextRangePtr spRange;
        GetDocRange(spRange);

        eval_error_hr(spRange->Select());

    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

}
void CLogViewer::OnInitMenuPopup(HWND hWnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
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
        case ID_VIEW:
            OnInitViewMenu(hMenu);
            bHandled = true;
            break;

        case ID_EDIT:
            OnInitEditMenu(hMenu);
            bHandled = true;
            break;

        case ID_ZOOM:
            OnInitZoomMenu(hMenu);
            bHandled = true;
            break;
        }
    }

    if (!bHandled) {
        FORWARD_WM_INITMENUPOPUP(hWnd, hMenu, item, fSystemMenu, DefWindowProcW);
    }
}

void CLogViewer::FindString(LPFINDREPLACEW lpfr)
{
    try{
        ITextSelectionPtr spSelection;
        eval_error_hr(m_spTextDoc->GetSelection(&spSelection));

        long nFlags = 0;
        if (lpfr->Flags & FR_MATCHCASE) {
            nFlags |= tomMatchCase;
        }

        if (lpfr->Flags & FR_WHOLEWORD) {
            nFlags |= tomMatchWord;
        }

        _bstr_t szFind = lpfr->lpstrFindWhat;


        long nLength = 0;
        HRESULT hr = E_UNEXPECTED;
        if (lpfr->Flags & FR_DOWN) {
            long nCheckStart = 0, nCheckEnd = 0;
            eval_error_hr(spSelection->GetStart(&nCheckStart));
            eval_error_hr(spSelection->GetEnd(&nCheckEnd));
            eval_error_hr(spSelection->Collapse(tomEnd));

#pragma push_macro("FindText") 
#undef FindText
            hr = eval_error_hr(spSelection->FindText(szFind, tomForward, nFlags, &nLength));
#pragma pop_macro("FindText") 
            
            if (hr != S_OK) {
                eval_error_hr(spSelection->SetStart(nCheckStart));
                eval_error_hr(spSelection->SetEnd(nCheckEnd));
            }
            
        } else {
            long nStart = 0;
            eval_error_hr(spSelection->GetStart(&nStart));

            hr = eval_error_hr(spSelection->FindTextStart(szFind, -nStart, nFlags, &nLength));
        }

        if (hr == S_OK) {
            long nSelStart = 0;
            eval_error_hr(spSelection->GetStart(&nSelStart));
            eval_error_hr(spSelection->SetEnd(nSelStart + nLength));
        }
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}

void CLogViewer::OnFind(HWND hwnd)
{
    static FINDREPLACEW fr = { 0 };
    static wchar_t lpszFindBuf[160] = { 0 };

    try{
        ::ZeroMemory(&fr, sizeof(FINDREPLACEW));
        ::ZeroMemory(lpszFindBuf, sizeof(lpszFindBuf));

        fr.lStructSize = sizeof(FINDREPLACEW);
        fr.hwndOwner = hwnd;
        fr.lpstrFindWhat = lpszFindBuf;
        fr.wFindWhatLen = sizeof(lpszFindBuf);

        m_hDlgFind = eval_error_nz (::FindTextW(&fr));
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

}

void CLogViewer::OnFindMsg(HWND hwnd, LPFINDREPLACEW lpfr)
{
    if (lpfr->Flags & FR_DIALOGTERM) {
        m_hDlgFind = nullptr;
    }else if (lpfr->Flags & FR_FINDNEXT) {
        m_pFR = lpfr;
        FindString(lpfr);
    }

}

void CLogViewer::OnFindAgain(HWND hwnd, int id)
{
    if (id == ID_EDIT_FINDNEXT) {
        m_pFR->Flags |= FR_DOWN;
    } else {
        m_pFR->Flags &= ~FR_DOWN;
    }
    FindString(m_pFR);
}

bool CLogViewer::ProcessDlgMsg(LPMSG lpMsg)
{
    bool bRetVal = m_hDlgFind && ::IsDialogMessageW(m_hDlgFind, lpMsg);
    if (!bRetVal) {
        bRetVal = ::TranslateAcceleratorW(m_hWnd, m_hAccel, lpMsg);
    }
    return bRetVal;
}


void CLogViewer::OnZoom(HWND hwnd, int id)
{
    constexpr static UINT NUMERATOR_CHANGE = 10;

    try{
        long nNumerator = 0;
        long nDenominator = 0;
        GetZoom(&nNumerator, &nDenominator);

        nNumerator -= (nNumerator % NUMERATOR_CHANGE);

        if (id == ID_ZOOM_ZOOMIN ) {
            if (nNumerator < MAX_NUMERATOR) {
                nNumerator += NUMERATOR_CHANGE;
            }
        } else if (id == ID_ZOOM_ZOOMOUT) {
            if (nNumerator > MIN_NUMERATOR) {
                nNumerator -= NUMERATOR_CHANGE;
            }
        }
        eval_error_nz(::SendMessageW(m_hEdit, EM_SETZOOM, static_cast<WPARAM>(nNumerator), static_cast<LPARAM>(nDenominator)));
        OnSelChange(hwnd);
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

}

double CLogViewer::GetZoom(long* pnNumerator, long* pnDenominator)
{
    long    nNumerator = 0,
            nDenominator = 0;

    long* pnNumerator_ = pnNumerator ? pnNumerator : &nNumerator;
    long* pnDenominator_ = pnDenominator ? pnDenominator : &nDenominator;

    *pnNumerator_ = 0;
    *pnDenominator_ = 0;

    eval_error_nz(::SendMessageW(m_hEdit, EM_GETZOOM, reinterpret_cast<WPARAM>(pnNumerator_), reinterpret_cast<LPARAM>(pnDenominator_)));

    if (*pnDenominator_ == 0 && *pnNumerator_ == 0) {
        *pnNumerator_ = 100;
        *pnDenominator_ = 100;
    }

    return (static_cast<double>(*pnNumerator_) / static_cast<double>(*pnDenominator_)) * 100.0;
}


void CLogViewer::OnSetFocus(HWND, HWND)
{
    ::SetFocus(m_hEdit);
}

void CLogViewer::OnInitEditMenu(HMENU hMenu)
{

    UINT uFlag = MF_BYCOMMAND | (m_pFR ? MF_ENABLED : MF_DISABLED);
    ::EnableMenuItem(hMenu, ID_EDIT_FINDNEXT, uFlag);
    ::EnableMenuItem(hMenu, ID_EDIT_FINDPREVIOUS, uFlag);

}

void CLogViewer::OnInitZoomMenu(HMENU hMenu)
{
    try{
        long nNumerator = 0;
        long nDenominator = 0;
        GetZoom(&nNumerator, &nDenominator);

        ::EnableMenuItem(hMenu, ID_ZOOM_ZOOMIN, MF_BYCOMMAND | ((nNumerator < MAX_NUMERATOR) ? MF_ENABLED : MF_DISABLED));
        ::EnableMenuItem(hMenu, ID_ZOOM_ZOOMOUT, MF_BYCOMMAND | ((nNumerator > MIN_NUMERATOR) ? MF_ENABLED : MF_DISABLED));
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

}

void CLogViewer::OnInitViewMenu(HMENU hMenu)
{
    try{
        eval_error_nz( CTWinUtils::CheckMenuItem(hMenu, ID_VIEW_LINENUMBERS, m_bLineNumbers));
        eval_error_nz(CTWinUtils::CheckMenuItem(hMenu, ID_VIEW_STATUSBAR, m_bStatusBar));
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

}

void CLogViewer::OnGoto(HWND hwnd)
{
    try {
        ITextSelectionPtr spTextSelection;
        eval_error_hr(m_spTextDoc->GetSelection(&spTextSelection));

        long nCurrLine = 0;
        eval_error_hr(spTextSelection->GetIndex(tomLine, &nCurrLine));

        m_nGotoLine = nCurrLine;

        if (DoModal(hwnd, IDD_GOTO) ) {
            ITextRangePtr spRange;
            eval_error_hr(m_spTextDoc->Range(0, 0, &spRange));
            eval_error_hr(spRange->MoveStart(tomStory, 1, nullptr));

            long nCount = 0;
            eval_error_hr(spRange->GetIndex(tomLine, &nCount));
            if (m_nGotoLine > 0 && m_nGotoLine <= nCount) {
                eval_error_hr(spTextSelection->GetDuplicate(&spRange));
                eval_error_hr(spRange->Move(tomCharacter, -1, nullptr));
                long nNewLine = 0;
                eval_error_hr(spRange->GetIndex(tomLine, &nNewLine));
                if (nNewLine == nCurrLine) {
                    eval_error_hr(spTextSelection->HomeKey(tomLine, tomMove, nullptr));
                }

                eval_error_hr(spTextSelection->SetIndex(tomLine, m_nGotoLine, 0));
            } else {
                eval_error_nz(::MessageBoxW(hwnd, L"The line number is beyond the total number of lines", m_szWinTitle.c_str(), MB_OK | MB_ICONWARNING | MB_APPLMODAL));
            }
        }

    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

    m_nGotoLine = 0;
}

INT_PTR CALLBACK CLogViewer::s_DlgFunc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    if (uMsg == WM_INITDIALOG) {
        ::SetLastError(0);
        if (!::SetWindowLongPtrW(hwnd, DWLP_USER, lParam) && ::GetLastError() != 0) {
            ::EndDialog(hwnd, FALSE);
            return TRUE;
        }
    }
    
    INT_PTR nRetVal = FALSE;
    CLogViewer* pThis = reinterpret_cast<CLogViewer*>(::GetWindowLongPtrW(hwnd, DWLP_USER));
    if (pThis) {
        CheckDefDlgRecursion(&pThis->m_fRecursing);
        bool bFuncFound = false;
        LRESULT lr = 0;
        switch (pThis->m_uCurrDlg) {
        case IDD_GOTO:
            lr = pThis->GotoDlgFunc(hwnd, uMsg, wParam, lParam);
            bFuncFound = true;
            break;

        }

        if (bFuncFound) {
            nRetVal = SetDlgMsgResult(hwnd, uMsg, lr);
        }
    } 
    return nRetVal;
}

LRESULT CLogViewer::GotoDlgFunc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        HANDLE_MSG(hwnd, WM_INITDIALOG, GotoOnInitDialog);  
        HANDLE_MSG(hwnd, WM_COMMAND, GotoOnCommand);
        HANDLE_MSG(hwnd, WM_SETFOCUS, GotoOnSetFocus);
    }

    return LVDefDlgProcEx(hwnd, uMsg, wParam, lParam);
}


LRESULT CLogViewer::LVDefDlgProcEx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefDlgProcEx(hwnd, uMsg, wParam, lParam, &m_fRecursing);
}

BOOL CLogViewer::GotoOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    try{
        HWND hWndEdit = eval_error_nz(::GetDlgItem(hwnd, IDC_EDITLINE));
        eval_error_nz(Edit_SetText(hWndEdit, std::to_wstring(m_nGotoLine).c_str()));
        ::SetFocus(hWndEdit);
        Edit_SetSel(hWndEdit, 0, -1);
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }


    return FALSE;
}

void CLogViewer::GotoOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {
    case IDC_BUTTONGOTO:
        GotoOnButtonGo(hwnd);
        break;

    case IDCANCEL:
        ::EndDialog(hwnd, FALSE);
        break;

    default:
        FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, LVDefDlgProcEx);
    }
}

void CLogViewer::GotoOnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    try{
        HWND hWndEdit = eval_error_nz(::GetDlgItem(hwnd, IDC_EDITLINE));
        ::SetFocus(hWndEdit);
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

}

void CLogViewer::GotoOnButtonGo(HWND hwnd)
{
    try {
        HWND hWndEdit = eval_error_nz(::GetDlgItem(hwnd, IDC_EDITLINE));
        int ccbEdit = Edit_GetTextLength(hWndEdit);
        if (ccbEdit) {
            std::wstring szEdit;
            eval_error_nz(Edit_GetText(hWndEdit, sz_wbuf(szEdit, ccbEdit + 1), ccbEdit + 1));
            m_nGotoLine = std::stoul(szEdit);
        }
        ::EndDialog(hwnd, TRUE);
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

}

void CLogViewer::OnLineNumbers(HWND hwnd)
{
    try{
        m_bLineNumbers = !m_bLineNumbers;

        SetLineNumbers(hwnd);
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }

}

void CLogViewer::SetLineNumbers(HWND hwnd)
{
    ITextSelectionPtr spTextSelection;
    eval_error_hr(m_spTextDoc->GetSelection(&spTextSelection));

    long nStart = 0, nEnd = 0;
    eval_error_hr(spTextSelection->GetStart(&nStart));
    eval_error_hr(spTextSelection->GetEnd(&nEnd));

    OnSetSel(hwnd);

    PARAFORMAT2 pf = { 0 };
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_NUMBERING | PFM_NUMBERINGSTYLE | PFM_NUMBERINGSTART;
    if (m_bLineNumbers) {
        pf.wNumbering = PFN_ARABIC;
        pf.wNumberingStyle = PFNS_PERIOD;
        pf.wNumberingStart = 1;
    }

    eval_error_nz(::SendMessageW(m_hEdit, EM_SETPARAFORMAT, 0, reinterpret_cast<LPARAM>(&pf)));

    eval_error_hr(spTextSelection->SetStart(nStart));
    eval_error_hr(spTextSelection->SetEnd(nEnd));
}

void CLogViewer::OnStatusBar(HWND hwnd)
{
    try{
        m_bStatusBar = !m_bStatusBar;
        
        eval_error_es(ClassicTileRegUtil::SetRegStatusBar(m_bStatusBar));
        
        CreateDestroyStatusBar(hwnd);
    } catch (const LoggingException& le) {
        le.Log();
    } catch (...) {
        log_error("Unhandled exception");
    }
}

void CLogViewer::CreateDestroyStatusBar(HWND hwnd)
{
    RECT r = { 0 };
    eval_error_nz(::GetClientRect(hwnd, &r));

    if (m_bStatusBar) {
        m_hStatus = eval_error_nz(::CreateWindowExW(0,
            STATUSCLASSNAME,
            nullptr,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0,
            0,
            0,
            0,
            hwnd,
            reinterpret_cast<HMENU>(IDC_LOGSTATUS),
            m_hInst,
            nullptr));

        int nWidth = r.right / STATUS_PARTS;
        int rightEdge = nWidth;

        std::vector<int> vParts(STATUS_PARTS);
        for (int i = 0; i < STATUS_PARTS; i++) {
            vParts[i] = rightEdge;
            rightEdge += nWidth;
        }

        WPARAM wStatusParts = STATUS_PARTS;
        eval_error_nz(::SendMessageW(m_hStatus, SB_SETPARTS, wStatusParts, reinterpret_cast<LPARAM>(vParts.data())));

        ::SendMessageW(m_hStatus, WM_SIZE, 0, 0);


        OnSelChange(hwnd);
    } else if (m_hStatus) {
        eval_error_nz(::DestroyWindow(m_hStatus));
        m_hStatus = nullptr;
    }
    OnSize(hwnd, SIZE_RESTORED, r.right, r.bottom);
}


void CLogViewer::OnDestroy(HWND hwnd)
{
    m_hEdit = nullptr;
    m_hStatus = nullptr;
    m_spTextDoc.Release();

    m_pFR = nullptr;

    m_hDlgFind = nullptr;

    m_nGotoLine = 0;
    m_fRecursing = FALSE;

    m_bLineNumbers = false;
    m_bStatusBar = false;
    m_uCurrDlg = 0;

    __super::OnDestroy(hwnd);
}

LRESULT CALLBACK CLogViewer::s_RESubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    LRESULT nRetVal = ::DefSubclassProc(hWnd, uMsg, wParam, lParam);

    switch (uMsg) {
    case WM_MOUSEWHEEL:
        if ((GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) == MK_CONTROL) {
            CLogViewer* pThis = reinterpret_cast<CLogViewer*>(dwRefData);
            pThis->OnSelChange(pThis->m_hWnd);
        }
        break;

    case WM_NCDESTROY:
        ::RemoveWindowSubclass(hWnd, s_RESubClass, 0);
        break;
    }

    return nRetVal;
}