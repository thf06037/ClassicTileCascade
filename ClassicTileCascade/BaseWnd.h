/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ClassicTileCascade.cpp` for details.
 */
#pragma once
//Base class for classes representing windows
//Provides methods for common window initialization and destruction tasks
template<class derived>
class BaseWnd
{
public:
	BaseWnd(bool bQuitOnDestroy = false)
		: m_bQuitOnDestory(bQuitOnDestroy) {};

	//Method to call before starting windows msg pump.
	//This method will create windows class and window 
	//with minimal functionality (basically functionality for 
	//an invisible window. Subclasses should override BeforeWndCreate
	//to set windows class features (by assigning values to the members
	//of m_wcex) and/or to set windows features (by assigning values to
	//the various other member variables as described below. Subclasses
	//that need to perform custom initiation actions after windows creation
	//should override AfterWndCreate
	virtual bool InitInstance(HINSTANCE hInstance)
	{
		bool bRetVal = false;

		try {
			bool bRanPrior = m_hInst;

			m_hInst = hInstance;

			//Set up miminal windows class values. 
			m_wcex.cbSize = sizeof(WNDCLASSEX);
			m_wcex.lpfnWndProc = s_WndProc;
			m_wcex.cbWndExtra = sizeof(this);
			m_wcex.hInstance = m_hInst;

			if (BeforeWndCreate(bRanPrior)) {
				//At a minimum, BeforeWndCreate must set the windows class name
				eval_fatal_nz(m_wcex.lpszClassName);

				//Check if we've registered windows class previously and only
				//register if we haven't
				WNDCLASSEXW wcex = { 0 };
				wcex.cbSize = sizeof(WNDCLASSEX);
				if (!::GetClassInfoExW(m_hInst, m_wcex.lpszClassName, &wcex)) {
					eval_fatal_nz(::RegisterClassExW(&m_wcex));
				}

				//A CBT windows hook is used to ensure that we can set GWLP_USERDATA (windows class user data) with this
				//pointer before the first time our windows proc is called. The hook is removed immediately after the the
				//window is created
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

	//Method that should be called from within windows msg pump to 
	//Subclasses should override if they need to call any dialog or 
	//accelerator functions. 
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
	//Miminal windows proc that just calls the DefWindowProc. Almost all subclasses 
	//will override this method and just call BaseWnd's version of the method if 
	//their ClassWndProc doesn't process a certain msg
	virtual LRESULT ClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}

	//Pure virtual function that is called from InitInstance before registering 
	//Windows class and creating the window. At a minimum, m_wcex.lpszClassName
	//must be set in this method, but most subclasses will also set other Windows
	//class members (i.e. members of m_wcex) and other member variables that
	//are passed to CreateWindow. If bRanPrior is true, subclasses can use this to
	//do any cleanup necessary. Returning false from this method will cause
	//InitInstance return false and to not register the windows calss or create the window.
	virtual bool BeforeWndCreate(bool bRanPrior) = 0;

	//This method can be  overriden if subclass needs to do any custom initiation after window creation is complete
	//Returning false from this method will cause InitInstance to return false
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

		if (m_bQuitOnDestory) {
			::PostQuitMessage(0);
		}
	}

	//Actual WndProc for all derived classes. This method simply retrieves the this pointer
	//from GWLP_USERDATA (which was set in the CBT windows hook process before the first message
	//is dispatched to this proc) and calls ClassWndProc of the subclass
	static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT nRetVal = 0;
		BaseWnd* pThis = reinterpret_cast<derived*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		if (pThis) {
			nRetVal = pThis->ClassWndProc(hwnd, uMsg, wParam, lParam);
		} else {
			//Something is wrong if we are receiving WM_NCCREATE or WM_CREATE and the
			//this pointer isn't in GWLP_USERDATA yet. Return the failure values for each
			//of these message types 
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
	//HINSTANCE passed by client into the call to InitInstance
	HINSTANCE m_hInst = nullptr;

	//HWND for our class
	HWND m_hWnd = nullptr;

	//Windows class info. Subclasses should update values
	//as necessary in BeforeWndCreate
	WNDCLASSEXW m_wcex = { 0 };

	//Parameters to CreateWindow. Subclasses should update values
	//as necessary in BeforeWndCreate
	std::wstring m_szWinTitle;
	DWORD     m_dwExStyle = 0;
	DWORD     m_dwStyle = 0;
	int		  m_x = 0;
	int		  m_y = 0;
	int		  m_nWidth = 0;
	int		  m_nHeight = 0;
	HWND      m_hwndParent = nullptr;
	HMENU	  m_hMenu = nullptr;

	//This controls whether or not OnDestroy will 
	//call PostQuitMessage to end the message pump
	bool	m_bQuitOnDestory = false;
};

