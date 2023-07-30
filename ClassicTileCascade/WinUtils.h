#pragma once
/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `WinUtils.cpp` for details.
 */

namespace CTWinUtils
{
	using LPSHELLFUNC = HRESULT(STDMETHODCALLTYPE IShellDispatch::*)();

	void ShellHelper(LPSHELLFUNC lpShellFunc);
	UINT GetSubMenuPosByString(HMENU hMenu, const std::wstring& szString);
	bool CheckMenuItem(HMENU hMenu, UINT uID, bool bChecked);

	bool ShellExecInExplorerProcess(const std::wstring& szFile, const std::wstring& szArgs = L"", LPDWORD pDWProcID = nullptr);
	DWORD GetCurrModuleFileName(std::wstring& szCurrModFileName);
	void Wstring2string(std::string& szString, const std::wstring& szWString);
	void String2wstring(std::wstring& szWString, const std::string& szString);
	bool GetMenuStringSubMenu(HMENU hMenu, UINT uItem, bool bByPosition, std::wstring& szMenu, HMENU* phSubMenu = nullptr);

	template<class T>
	void RemoveFromNull(T& szRemove)
	{
		size_t nNullChar = szRemove.find(static_cast<T::value_type>(0));
		if (nNullChar != T::npos) {
			szRemove.erase(nNullChar);
		}
	}

	template<class T>
	bool PathCombineEx(T& szDest, const T& szDir, const T& szFile);

	bool GetFinalPathNameByFILE(FILE* pFile, std::wstring& szPath);
}
