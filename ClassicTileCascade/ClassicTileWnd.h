/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ClassicTileWnd.cpp` for details.
 */
#pragma once

// Creates a Shell Notification Icon and encapsulates an invisible window that processes messages coming from 
// that Shell Notification icon. Caller instantiates a ClassicTileWnd, calls RegUnReg to determine if 
// command line parameters have been passed for the purposes of registering or unregistering the program 
// (these would be passed by Windows installer as part of a custom instal step). RegUnReg returns false if no
// command line parameters have been passed and then caller should call InitInstance, which will deserialize
// state from the registry, set up logging, and create the Shell Notification Icon. Once InitInstance returns 
// successfully, caller should start a Windows messaging loop.

class ClassicTileWnd
{
public:
	ClassicTileWnd();
	bool InitInstance(HINSTANCE hInstance);
	bool RegUnReg(bool& fSuccess);
	
	ClassicTileWnd(const ClassicTileWnd&) = delete;
	ClassicTileWnd(ClassicTileWnd&&) = delete;
	ClassicTileWnd& operator=(const ClassicTileWnd&) noexcept = delete;
	ClassicTileWnd& operator=(ClassicTileWnd&&) noexcept = delete;

protected:
	//helper classes & typedefs
	 
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
	
	using MenuId2MenuItem = std::map<UINT, File2DefaultStruct>;
	using MenuId2MenuItemPair = std::pair<UINT, File2DefaultStruct>;
	using TILE_CASCADE_FUNC = WORD(WINAPI*)(HWND, UINT, const RECT*, UINT, const HWND*);
	using HwndVector = std::vector<HWND>;
	
	//Helper functions

	bool AddTrayIcon(HWND hWnd);
	
	// Check whether tray icon exists and create it if it doesn't
	// This is necessary b/c there exist edge cases where this application 
	// will have been started before the toolbar is created. The message handler
	// for the class listens for the "TaskbarCreated" message and will create the
	// Notification Icon if it doesn't exist as of receiving that message
	bool CheckTrayIcon() const;
	
	// Set the hover-over tool tip for the notification icon using the selected value of the 
	// "Settings | Left click does" radio button submenu
	void GetToolTip();
	
	void TileCascadeHelper(TILE_CASCADE_FUNC, UINT uHow) const;
	void CloseTaskDlg();
	LRESULT CTWWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//Static helper functions
	//static  File2DefaultStruct FindMenuId2MenuItem(const MenuId2MenuItem& menuID2MenuItem, UINT uSought);
	static  const File2DefaultStruct& FindMenuId2MenuItem(const MenuId2MenuItem& menuID2MenuItem, UINT uSought);
	static  bool RegUnRegAsUser(const std::wstring& szFirstArg);
	static  bool Unregister();
	static  bool Register();
	static  void OpenTextFile(HWND hwnd, const std::wstring& szPath);

	//callback functions
	static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static HRESULT CALLBACK s_TaskDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData);
	static BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam);


	//Top-level msg handlers
	void OnSWMTrayMsg(HWND hwnd, WORD wNotifEvent, WORD wIconId, int x, int y);
	void OnCommand(HWND hwnd, int id, HWND, UINT);
	void OnClose(HWND hwnd);
	void OnDestroy(HWND);

	//SWM_TRAYMSG msg handlers
	void OnContextMenu(HWND hwnd, HWND, UINT xPos, UINT yPos);

	//WM_COMMAND menu msg handlers
	void OnHelp(HWND hwnd) const;
	void OnChangeDefault(int id);
	void OnSettingsAutoStart();
	void OnSettingsLogging(HWND hwnd);
	void OnSettingsDefWndTile();

	//Task Dialog msg handlers
	void OnHyperlinkClick(LPARAM lpszURL) const;

protected:
	//class constants

	// For use with NOTIFYICONDATA::uID
	constexpr static UINT TRAYICONID = 1;

	// Windows class name for invisible window that handles notification icon
	// messages
	const static std::wstring CLASS_NAME;

	// Accepts value of ::RegisterWindowMessageW(L"TaskbarCreated"). 
	// See comment above re: CheckTrayIcon
	const static UINT WM_TASKBARCREATED;

	// 2 maps to convert top level tile/cascade menu actions to the equivalent choices
	// on the "Settings | Left click does" submenu and vice versa
	const static MenuId2MenuItem Default2FileMap;
	const static MenuId2MenuItem File2DefaultMap;

	const static std::wstring CURR_MODULE_PATH;
	const static std::wstring LOG_PATH;
	const static std::string LOG_PATH_NARROW;

protected:
	//instance members


	// HINSTANCE handle for use in member functions that need to access 
	// resources, etc.
	HINSTANCE m_hInst = nullptr;

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
};

