/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `MemMgmt.cpp` for details.
 */
#pragma once
#include "win_log.h"

// Type definitions of smart pointers, with custom deleters for various Win32 object types
// Also includes a utility class (CCoInitialize) for automatically calling CoInitialize and 
// CoUnitialize at entry/exit of scope

// Custom deleters
struct HKEY_deleter {
    void operator()(HKEY hKey)
    {
        if (hKey) {
            ::RegCloseKey(hKey); 
        }
    }

    using pointer = HKEY;
};

struct HMENU_deleter {
    void operator()(HMENU hMenu)
    {
        if (hMenu) {
            ::DestroyMenu(hMenu);
        }
    }

    using pointer = HMENU;
};

struct HWND_deleter {
    void operator()(HWND hwnd)
    {
        if (hwnd) {
            ::DestroyWindow(hwnd);
        }
    }

    using pointer = HWND;
};

struct HICON_deleter {
    void operator()(HICON hIcon)
    {
        if (hIcon) {
            ::DestroyIcon(hIcon);
        }
    }

    using pointer = HICON;
};

struct FILE_deleter {
    void operator()(FILE* pFile)
    {
        if (pFile) {
            ::fclose(pFile);
        }
    }
};

struct HANDLE_deleter {
    void operator()(HANDLE h)
    {
        if (h) {
            ::CloseHandle(h);
        }
    }

    using pointer = HANDLE;
};


// Type definitions of various smart pointers for use with Win32 object types
using SPHKEY = std::unique_ptr<HKEY, HKEY_deleter>;
using SPHMENU = std::unique_ptr<HMENU, HMENU_deleter>;
using SPHWND = std::unique_ptr<HWND, HWND_deleter>;
using SPHICON = std::unique_ptr<HICON, HICON_deleter>;
using SPFILE = std::unique_ptr<FILE, FILE_deleter>;
using SPHANDLE_EX = std::unique_ptr<HANDLE, HANDLE_deleter>;

// Utility class (CCoInitialize) for automatically calling CoInitialize and 
// CoUnitialize at entry/exit of scope
struct CCoInitialize
{
    CCoInitialize()
    {
        eval_error_hr(::CoInitialize(NULL));
    }

    ~CCoInitialize()
    {
        ::CoUninitialize();
    }
};