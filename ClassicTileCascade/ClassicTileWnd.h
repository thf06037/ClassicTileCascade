/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ClassicTileWnd.cpp` for details.
 */
#pragma once

class ClassicTileWnd
{
public:
	//ClassicTileWnd(SPFILE_SHARED pLogFP);
	ClassicTileWnd(FILE* pLogFP);
	bool InitInstance(HINSTANCE hInstance);
	bool RegUnReg(bool& fSuccess);
	
	ClassicTileWnd() = delete;
	ClassicTileWnd(const ClassicTileWnd&) = delete;
	ClassicTileWnd(ClassicTileWnd&&) = delete;
	ClassicTileWnd& operator=(const ClassicTileWnd&) noexcept = delete;
	ClassicTileWnd& operator=(ClassicTileWnd&&) noexcept = delete;

protected:
	//helper classes & typedefs
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
	bool CheckTrayIcon() const;
	void GetToolTip();
	void TileCascadeHelper(TILE_CASCADE_FUNC, UINT uHow) const;
	void CloseTaskDlg();
	LRESULT CTWWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	//Static helper functions
	static  File2DefaultStruct FindMenuId2MenuItem(const MenuId2MenuItem& menuID2MenuItem, UINT uSought);
	static  bool Unregister();

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

protected:
	//class constants
	constexpr static UINT TRAYICONID = 1;
	const static std::wstring CLASS_NAME;
	const static UINT WM_TASKBARCREATED;
	const static MenuId2MenuItem Default2FileMap;
	const static MenuId2MenuItem File2DefaultMap;

protected:
	//instance members
	HINSTANCE m_hInst = nullptr;
	NOTIFYICONDATA	m_niData = { 0 };
	DWORD m_nLeftClick = ID_FILE_CASCADEWINDOWS;
	bool m_bLogging = false;
	bool m_bAutoStart = false;
	bool m_bDefWndTile = false;
	HWND m_hwndTaskDlg = nullptr;
	FILE* m_pLogFP = nullptr;
};

