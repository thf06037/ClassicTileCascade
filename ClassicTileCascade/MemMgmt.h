/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `MemMgmt.cpp` for details.
 */
#pragma once
#include "win_log.h"


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

using SPHKEY = std::unique_ptr<HKEY, HKEY_deleter>;
using SPHMENU = std::unique_ptr<HMENU, HMENU_deleter>;
using SPHWND = std::unique_ptr<HWND, HWND_deleter>;
using SPHICON = std::unique_ptr<HICON, HICON_deleter>;
using SPFILE = std::unique_ptr<FILE, FILE_deleter>;
using SPFILE_SHARED = std::shared_ptr<FILE>;
using SPHANDLE_EX = std::unique_ptr<HANDLE, HANDLE_deleter>;

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