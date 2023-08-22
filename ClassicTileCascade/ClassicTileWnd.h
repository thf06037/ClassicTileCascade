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
	ClassicTileWnd() = default;
	bool InitInstance(HINSTANCE hInstance);
	
	ClassicTileWnd(const ClassicTileWnd&) = delete;
	ClassicTileWnd(ClassicTileWnd&&) = delete;
	ClassicTileWnd& operator=(const ClassicTileWnd&) noexcept = delete;
	ClassicTileWnd& operator=(ClassicTileWnd&&) noexcept = delete;

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
	//Helper functions
	//////////////////
	bool AddTrayIcon(HWND hWnd);
	// Set the hover-over tool tip for the notification icon using the selected value of the 
	// "Settings | Left click does" radio button submenu
	void GetToolTip();
	void CloseTaskDlg();
	LRESULT CTWWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void EnableLogging();

	/////////////////////////
	//Static helper functions
	/////////////////////////
	static  const File2DefaultStruct& FindMenuId2MenuItem(MenuId2MenuItemDir menuID2MenuItemDir, UINT uSought);

	////////////////////
	//callback functions
	////////////////////
	static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static HRESULT CALLBACK s_TaskDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LONG_PTR lpRefData);
	static BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam);

	////////////////////////
	//Top-level msg handlers
	////////////////////////
	void OnSWMTrayMsg(HWND hwnd, WORD wNotifEvent, WORD wIconId, int x, int y);
	void OnCommand(HWND hwnd, int id, HWND, UINT);
	void OnClose(HWND hwnd);
	void OnDestroy(HWND);
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

	//////////////////////////
	//Task Dialog msg handlers
	//////////////////////////
	void OnHyperlinkClick(LPARAM lpszURL) const;

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
	const static std::wstring APP_NAME;


protected:
	//////////////////
	//instance members
	//////////////////

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

	SPHMENU m_hMenu;
	HMENU m_hPopupMenu = nullptr;
};

