#pragma once
/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `WinUtils.cpp` for details.
 */

// Library of helper utilities for calling various Windows functions
namespace CTWinUtils
{
	// Create IShellDispatch instance and call any passed-in IShellDispatch member function 
	// with no parameters
	using LPSHELLFUNC = HRESULT(STDMETHODCALLTYPE IShellDispatch::*)();
	void ShellHelper(LPSHELLFUNC lpShellFunc);

	//Windows menu object helpers
	bool CheckMenuItem(HMENU hMenu, UINT uID, bool bChecked);
	bool GetMenuStringSubMenu(HMENU hMenu, UINT uItem, bool bByPosition, std::wstring& szMenu, HMENU* phSubMenu = nullptr);
	UINT SetSubMenuDataFromItemData(HMENU hMenu);

	// Check whether user is running at escalated privilege level (admin) and start another process as non-admin user.
	// If user is not running as admin, simply start the process as current user
	bool ShellExecInExplorerProcess(std::wstring_view szFile, std::wstring_view szArgs = L"", LPDWORD pDWProcID = nullptr);
	
	// File/path manipulation routines
	// Wrapper for GetModuleFileNameW
	DWORD GetCurrModuleFileName(std::wstring& szCurrModFileName);

	// Wrapper for PathCombine. Works for both wstring and string
	template<class T, class TV>
	bool PathCombineEx(T& szDest, TV szDir, TV szFile);

	// String conversion routines
	void Wstring2string(std::string& szString, const std::wstring& szWString);
	void String2wstring(std::wstring& szWString, const std::string& szString);
	
	// Wrapper for CreateProcess
	bool CreateProcessHelper(std::wstring_view szCommand, std::wstring_view szArguments = L"", LPDWORD lpdwProcId = nullptr, const std::optional<int>& nShowWindow = {});


	bool PathQuoteSpacesW(std::wstring& szPath);

	// checks that file exists and is not a directory
	bool FileExists(std::wstring_view szPath);

	// Opens a text file using the default app based on app extension. If file type does not 
	// have a default, fallback to use notepad.exe
	void OpenTextFile(HWND hwnd, std::wstring_view szPath, std::wstring_view szAppName);
}
