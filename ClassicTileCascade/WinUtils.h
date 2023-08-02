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
	UINT GetSubMenuPosByString(HMENU hMenu, const std::wstring& szString);
	bool CheckMenuItem(HMENU hMenu, UINT uID, bool bChecked);
	bool GetMenuStringSubMenu(HMENU hMenu, UINT uItem, bool bByPosition, std::wstring& szMenu, HMENU* phSubMenu = nullptr);

	// Check whether user is running at escalated privilege level (admin) and start another process as non-admin user.
	// If user is not running as admin, simply start the process as current user
	bool ShellExecInExplorerProcess(const std::wstring& szFile, const std::wstring& szArgs = L"", LPDWORD pDWProcID = nullptr);
	
	// File/path manipulation routines
	// Wrapper for GetModuleFileNameW
	DWORD GetCurrModuleFileName(std::wstring& szCurrModFileName);

	// Wrapper for PathCombine. Works for both wstring and string
	template<class T>
	bool PathCombineEx(T& szDest, const T& szDir, const T& szFile);

	// Given a FILE*, get the fully qualified path of the file 
	// pointed to by that FILE*
	bool GetFinalPathNameByFILE(FILE* pFile, std::wstring& szPath);

	// String conversion routines
	void Wstring2string(std::string& szString, const std::wstring& szWString);
	void String2wstring(std::wstring& szWString, const std::string& szString);
	
	// Given a string or wstring, remove all characters starting with the first
	// '\0' encountered. Frequently useful for instances where a large buffer has
	// been allocated using [w]string::resize and then that string's buffer has been
	// passed to an API function expecting a char*/wchar_t* using string::data.
	template<class T>
	void RemoveFromNull(T& szRemove)
	{
		size_t nNullChar = szRemove.find(static_cast<T::value_type>(0));
		if (nNullChar != T::npos) {
			szRemove.erase(nNullChar);
		}
	}

	bool CreateProcessHelper(const std::wstring& szCommand, const std::wstring& szArguments = L"", LPDWORD lpdwProcId = nullptr, int nShowWindow = -1);
}
