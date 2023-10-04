/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `CLogViewer.cpp` for details.
 */
#pragma once

#include "MemMgmt.h"
#include "win_log.h"
#include "BaseWnd.h"

class CLogViewer : public BaseWnd<CLogViewer>
{
public:
	CLogViewer(bool bMainWnd = false);

	bool InitInstance(HINSTANCE hInstance, const std::wstring& szFilePath);
	bool ProcessDlgMsg(LPMSG lpMsg) override;
	bool SetFile(const std::wstring& szFilePath);
	
	CLogViewer(const CLogViewer&) = delete;
	CLogViewer(CLogViewer&&) noexcept = delete;
	CLogViewer& operator=(const CLogViewer&) = delete;
	CLogViewer& operator=(CLogViewer&&) noexcept = delete;

protected:
	using BaseWnd::InitInstance;
	////////////////////
	//type definitions
	////////////////////
	_COM_SMARTPTR_TYPEDEF(IRichEditOle, IID_IRichEditOle);
	_COM_SMARTPTR_TYPEDEF(ITextDocument, __uuidof(ITextDocument));
	_COM_SMARTPTR_TYPEDEF(ITextSelection, __uuidof(ITextSelection));
	_COM_SMARTPTR_TYPEDEF(ITextRange, __uuidof(ITextRange));

	////////////////////
	//BaseWnd overrides
	////////////////////
	bool BeforeWndCreate(bool bRanPrior) override;
	bool AfterWndCreate(bool bRanPrior) override;
	LRESULT ClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	////////////////////
	//Callbacks
	////////////////////
	static INT_PTR CALLBACK s_DlgFunc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK s_RESubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	////////////////////
	//Helper functions
	////////////////////
	bool OpenFile();
	void FindString(LPFINDREPLACEW lpfr);
	void GetDocRange(ITextRangePtr& spRange);
	INT_PTR DoModal(HWND hwnd, UINT uResource);
	LRESULT LVDefDlgProcEx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void CreateDestroyStatusBar(HWND hwnd);
	double GetZoom(long* pnNumerator = nullptr, long* pnDenominator = nullptr);
	void SetLineNumbers(HWND hwnd);

	////////////////////////
	//Top-level msg handlers
	////////////////////////
	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
	void OnSize(HWND hwnd, UINT state, int cx, int cy);
	void OnCommand(HWND hwnd, int id, HWND, UINT);
	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);
	void OnFindMsg(HWND hwnd, LPFINDREPLACEW lpfr);
	void OnSetFocus(HWND hwnd, HWND hwndOldFocus);
	LRESULT OnNotify(HWND hwnd, int uControl, NMHDR* lpNMHDR);
	void OnDestroy(HWND hwnd) override;

	////////////////////////
	//WM_COMMAND handlers
	////////////////////////
	void OnCopy(HWND hwnd);
	void OnSetSel(HWND hwnd);
	void OnFind(HWND hwnd);
	void OnFindAgain(HWND hwnd, int id);
	void OnZoom(HWND hwnd, int id);
	void OnGoto(HWND hwnd);
	void OnLineNumbers(HWND hwnd);
	void OnStatusBar(HWND hwnd);

	////////////////////////
	//WM_NOTIFY handlers
	////////////////////////
	void OnSelChange(HWND hwnd);

	////////////////////////////
	//WM_INITMENUPOPUP handlers
	////////////////////////////
	void OnInitEditMenu(HMENU hMenu);
	void OnInitZoomMenu(HMENU hMenu);
	void OnInitViewMenu(HMENU hMenu);

	//////////////////////////////
	//"Go to Line" dialog function
	//////////////////////////////
	LRESULT GotoDlgFunc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	/////////////////////////////////////////////
	//"Go to Line" dialog Top-level msg handlers
	/////////////////////////////////////////////
	BOOL GotoOnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
	void GotoOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	void GotoOnSetFocus(HWND hwnd, HWND hwndOldFocus);

	/////////////////////////////////////////////
	//"Go to Line" dialog WM_COMMAND handlers
	/////////////////////////////////////////////
	void GotoOnButtonGo(HWND hwnd);

	//////////////////
	//static members
	//////////////////
	constexpr static UINT MIN_NUMERATOR = 10;
	constexpr static UINT MAX_NUMERATOR = 500;
	constexpr static UINT STATUS_PARTS = 4;

	//////////////////
	//instance members
	//////////////////
	std::wstring m_szFilePath;

	HWND m_hEdit = nullptr;
	HWND m_hStatus = nullptr;

	ITextDocumentPtr m_spTextDoc;

	LPFINDREPLACEW m_pFR = nullptr;

	HWND m_hDlgFind = nullptr;

	long m_nGotoLine = 0;
	BOOL m_fRecursing = FALSE;

	bool m_bLineNumbers = false;
	bool m_bStatusBar = false;

	HACCEL m_hAccel = nullptr;

	UINT m_uCurrDlg = 0;
};