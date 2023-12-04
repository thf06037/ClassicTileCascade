/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ClassicTileWnd.cpp` for details.
 */
#pragma once

// Creates a Shell Notification Icon and encapsulates an invisible window that processes messages coming from 
// that Shell Notification icon. Caller calls ClassicTileWnd::Run which 
// instantiates a singleton ClassicTileWnd object and calls InitInstance, which will deserialize
// state from the registry, set up logging, and create the Shell Notification Icon. Once InitInstance returns 
// successfully, caller should start a Windows messaging loop.
#include "CLogViewer.h"

class ClassicTileWnd : public BaseWnd< ClassicTileWnd>
{
public:
	static bool Run(HINSTANCE hInst);
	static bool CTWProcessDlgMsg(LPMSG lpMsg);

protected:
	////////////////////////////
	//helper classes & typedefs
	////////////////////////////
	// Class to map the top level tile/cascade actions to the equivalent choices
	// on the "Settings | Left click does" submenu. Also maps both to the text of the
	// menu entries for those menus.
	struct File2DefaultStruct
	{
		UINT uDefault{};
		UINT uFile{};
		std::wstring szFileString;

		File2DefaultStruct(UINT uDefault_, UINT uFile_);
		File2DefaultStruct() = default;
		File2DefaultStruct(const File2DefaultStruct&) = default;
		File2DefaultStruct(File2DefaultStruct&&) noexcept = default;
		File2DefaultStruct& operator=(const File2DefaultStruct&) = default;
		File2DefaultStruct& operator=(File2DefaultStruct&&) noexcept = default;
	};
	
	using HwndVector = std::vector<HWND>;
	
	enum class MenuId2MenuItemDir
	{
		Default2FileMap,
		File2DefaultMap
	};

	
	//////////////////
	//Main functions
	//////////////////
	ClassicTileWnd();
	bool BeforeWndCreate(bool bRanPrior) override;
	bool AfterWndCreate(bool bRanPrior) override;
	bool ProcessDlgMsg(LPMSG lpMsg) override;
	LRESULT ClassWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	virtual ~ClassicTileWnd() = default;
	ClassicTileWnd(const ClassicTileWnd&) = delete;
	ClassicTileWnd(ClassicTileWnd&&) = delete;
	ClassicTileWnd& operator=(const ClassicTileWnd&) noexcept = delete;
	ClassicTileWnd& operator=(ClassicTileWnd&&) noexcept = delete;

	//////////////////
	//Helper functions
	//////////////////
	bool AddTrayIcon(HWND hWnd);
	// Set the hover-over tool tip for the notification icon using the selected value of the 
	// "Settings | Left click does" radio button submenu
	void GetToolTip();
	void CloseTaskDlg();
	void EnableLogging();

	/////////////////////////
	//Static helper functions
	/////////////////////////
	static  const File2DefaultStruct& FindMenuId2MenuItem(MenuId2MenuItemDir menuID2MenuItemDir, UINT uSought);

	////////////////////
	//callback functions
	////////////////////
	static HRESULT CALLBACK s_TaskDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData);
	static BOOL CALLBACK s_EnumProc(HWND hwnd, LPARAM lParam);

	////////////////////////
	//Top-level msg handlers
	////////////////////////
	void OnSWMTrayMsg(HWND hwnd, WORD wNotifEvent, WORD wIconId, int x, int y);
	void OnCommand(HWND hwnd, int id, HWND, UINT);
	void OnClose(HWND hwnd) override;
	void OnDestroy(HWND) override;
	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);

	//////////////////////////
	//SWM_TRAYMSG msg handlers
	//////////////////////////
	void OnContextMenu(HWND hwnd, HWND, UINT xPos, UINT yPos);

	//////////////////////////////
	//WM_COMMAND menu msg handlers
	//////////////////////////////
	void OnHelp(HWND hwnd) const;
	void OnChangeDefault(int id);
	void OnSettingsAutoStart();
	void OnSettingsLogging(HWND hwnd);
	void OnSettingsDefWndTile();
	void OnSettingsOpenLogFile(HWND hwnd, std::wstring_view szPath);

	//////////////////////////
	//Task Dialog msg handlers
	//////////////////////////
	void OnHyperlinkClick(LPARAM lpszURL);

	///////////////////////
	//InitMenuPopupHandlers
	///////////////////////
	void OnSettingsPopup(HMENU hMenu);
	void OnLeftClickDoesPopup(HMENU hMenu);

protected:
	/////////////////
	//class constants
	/////////////////
	// For use with NOTIFYICONDATA::uID
	constexpr static UINT TRAYICONID = 1;
	constexpr static std::wstring_view APP_NAME = L"Classic Tile Cascade";


protected:
	//////////////////
	//instance members
	//////////////////

	// Structure used to create and modify the Shell Notification
	NOTIFYICONDATA	m_niData = { 0 };

	// Default action chosen for left click under "Settings | Left click does" 
	DWORD m_nLeftClick = ID_FILE_CASCADEWINDOWS;

	// Whether logging is enabled. Represents check state of menu item 
	// "Settings | Logging"
	bool m_bLogging = false;

	// Whether or not to start automatically when user logs in. Represents the 
	// check state of menu item under "Settings | Start Automatically"
	bool m_bAutoStart = false;

	// Controls whether the Tile and Cascade actions use 
	// Windows default behaviors (buggy) [true] or a custom enumeration of 
	// the windows that are visible and reflected on the taskbar [false]
	// Represents the check state of menu item under "Settings | Default Windows Tile/Cascade"
	bool m_bDefWndTile = false;

	// Windows handle for the task dialog created in OnSettingsLogging to inform the user 
	// where the log files are. It's used to close the task dialog if the user left- or 
	// right-clicks on the notification icon
	HWND m_hwndTaskDlg = nullptr;

	// Smart pointer to the FILE* stream used for logging
	SPFILE m_pLogFP;

	SPHMENU m_hMenu;
	HMENU m_hPopupMenu = nullptr;

	CLogViewer m_logViewer;

	static ClassicTileWnd s_classicTileWnd;
};

