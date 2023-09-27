/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ClassicTileCascade.cpp` for details.
 */
#pragma once
template<class derived>
class BaseWnd
{
public:
	BaseWnd(bool bMainWnd = false) 
		: m_bMainWnd(bMainWnd){};

	virtual bool InitInstance(HINSTANCE hInstance)
	{
		bool bRetVal = false;

		try {
			bool bRanPrior = m_hInst;

			m_hInst = hInstance;

			m_wcex.cbSize = sizeof(WNDCLASSEX);
			m_wcex.lpfnWndProc = s_WndProc;
			m_wcex.cbWndExtra = sizeof(this);
			m_wcex.hInstance = m_hInst;

			if (BeforeWndCreate(bRanPrior)) {

				eval_fatal_nz(m_wcex.lpszClassName);

				WNDCLASSEXW wcex = { 0 };
				wcex.cbSize = sizeof(WNDCLASSEX);
				if (!::GetClassInfoExW(m_hInst, m_wcex.lpszClassName, &wcex)) {
					eval_fatal_nz(::RegisterClassExW(&m_wcex));
				}

				HookStruct hookStruct = { this, eval_fatal_nz(::SetWindowsHookExW(WH_CBT, s_CBTProc, 0, GetCurrentThreadId())) };

				try {
					m_hWnd = eval_fatal_nz(::CreateWindowExW(m_dwExStyle, m_wcex.lpszClassName, m_szWinTitle.c_str(), m_dwStyle, m_x, m_y, m_nWidth, m_nHeight,
						m_hwndParent, m_hMenu, m_hInst, &hookStruct));
				} catch (...) {
					::UnhookWindowsHookEx(hookStruct.hHook);
					throw;
				}

				eval_fatal_nz(::UnhookWindowsHookEx(hookStruct.hHook));

				bRetVal = AfterWndCreate(bRanPrior);
			}
		} catch (const LoggingException& le) {
			le.Log();
			bRetVal = false;
		} catch (...) {
			log_error("Unhandled exception");
			bRetVal = false;
		}

		return bRetVal;
	}

	virtual bool ProcessDlgMsg(LPMSG lpMsg)
	{
		return false;
	}

	virtual HWND GetHWND()
	{
		return m_hWnd;
	}

	BaseWnd(const BaseWnd&) = delete;
	BaseWnd(BaseWnd&&) noexcept = delete;
	BaseWnd& operator=(const BaseWnd&) = delete;
	BaseWnd& operator=(BaseWnd&&) noexcept = delete;

protected:
	struct HookStruct
	{
		BaseWnd * pThis;
		HHOOK hHook;
	};

protected:
	virtual LRESULT ClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}

	virtual bool BeforeWndCreate(bool bRanPrior)
	{
		return true;
	}

	virtual bool AfterWndCreate(bool bRanPrior)
	{
		return true;
	}

	virtual void OnClose(HWND hwnd)
	{
		::DestroyWindow(hwnd);
	}

	virtual void OnDestroy(HWND)
	{
		m_hWnd = nullptr;
		m_szWinTitle.clear();
		m_dwExStyle = 0;
		m_dwStyle = 0;
		m_x = 0;
		m_y = 0;
		m_nWidth = 0;
		m_nHeight = 0;
		m_hwndParent = nullptr;
		m_hMenu = nullptr;

		if (m_bMainWnd && m_bQuit) {
			::PostQuitMessage(0);
		}
	}


	static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT nRetVal = 0;
		BaseWnd* pThis = reinterpret_cast<derived*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		if (pThis) {
			nRetVal = pThis->ClassWndProc(hwnd, uMsg, wParam, lParam);
		} else {
			switch (uMsg) {
			case WM_NCCREATE:
				nRetVal = FALSE;
				break;

			case WM_CREATE:
				nRetVal = -1;
				break;
			}
		}

		return  nRetVal;
	}

	static LRESULT CALLBACK s_CBTProc(int code, WPARAM wp, LPARAM lp)
	{
		LRESULT nRetVal = 0;
		CBT_CREATEWNDW* lpCreateWndw = reinterpret_cast<CBT_CREATEWNDW*>(lp);

		if (
			(code == HCBT_CREATEWND)
			&&
			lpCreateWndw
			&&
			lpCreateWndw->lpcs
			&&
			lpCreateWndw->lpcs->lpCreateParams
			)
		{
			HookStruct* pHookStruct = reinterpret_cast<HookStruct*>(lpCreateWndw->lpcs->lpCreateParams);
			if (pHookStruct) {
				if (pHookStruct->pThis) {
					::SetLastError(0);
					if (!SetWindowLongPtrW(reinterpret_cast<HWND>(wp), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pHookStruct->pThis))
						&&
						(::GetLastError() != 0)
						)
					{
						nRetVal = 1;
					}
					pHookStruct->pThis = nullptr;
				}
				nRetVal = (nRetVal == 0) ? CallNextHookEx(pHookStruct->hHook, code, wp, lp) : nRetVal;
			}
		}

		return nRetVal;

	}

protected:
	HINSTANCE m_hInst = nullptr;
	HWND m_hWnd = nullptr;

	WNDCLASSEXW m_wcex = { 0 };

	std::wstring m_szWinTitle;
	DWORD     m_dwExStyle = 0;
	DWORD     m_dwStyle = 0;
	int		  m_x = 0;
	int		  m_y = 0;
	int		  m_nWidth = 0;
	int		  m_nHeight = 0;
	HWND      m_hwndParent = nullptr;
	HMENU	  m_hMenu = nullptr;
	bool	  m_bQuit = true;
	bool	  m_bMainWnd = false;

};

