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
	CLogViewer(bool bQuitOnDestroy = false);

	//Overload of this function to include the file path of the file
	//being viewed in the viewer
	bool InitInstance(HINSTANCE hInstance, std::wstring_view szFilePath);
	
	bool ProcessDlgMsg(LPMSG lpMsg) override;
	
	//Set the viewed file name and (re)open the file as read-only
	//in the rich edit control
	bool SetFile(std::wstring_view szFilePath);
	
	virtual ~CLogViewer() = default;
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
	//Actual dialog callback. Sets DWLP_USER to the this pointer in WM_INITDIALOG.
	//Delegates to GotoDlgFunc 
	static INT_PTR CALLBACK s_DlgFunc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//Callback for subclassing the rich edit control. Necessary to capture mouse wheel events and update the status 
	//of the zoom in the status bar
	static LRESULT CALLBACK s_RESubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

	////////////////////
	//Helper functions
	////////////////////
	//(Re)open the file indicated in m_szFilePath 
	//and move the rich edit box cursor to the end of the 
	//box
	bool OpenFile();


	void FindString(LPFINDREPLACEW lpfr);

	//Get the entire range of the doc
	void GetDocRange(ITextRangePtr& spRange);

	//Launch a modal dialog with uResource resource ID.
	//Uses s_DlgFunc as the callback
	INT_PTR DoModal(HWND hwnd, UINT uResource);

	LRESULT LVDefDlgProcEx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//Create (if hwnd is nullptr) or destroy (if hwnd != nullptr) the status bar
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

	//Describes the 4 sections of the status bar
	enum class StatSection : UINT
	{
		LINE,
		LINE_CHARACTER,
		CHARACTER,
		ZOOM
	};
	constexpr static UINT STATUS_PARTS = static_cast<UINT>(StatSection::ZOOM) + 1;

	//MIN and MAX numerators for rich edit zoom 
	constexpr static UINT MIN_NUMERATOR = 10;
	constexpr static UINT MAX_NUMERATOR = 500;


	//////////////////
	//instance members
	//////////////////
	
	//File to view
	std::wstring m_szFilePath;

	HWND m_hEdit = nullptr;
	HWND m_hStatus = nullptr;

	//ITextDocument is the COM interface that allows manipulation 
	//of the contents of the Rich Edit box. Prefer this to using
	//SendMessage(m_hEdit, ...) for manipulating contents
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