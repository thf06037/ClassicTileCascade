/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `pch.cpp` for details.
 */
// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include <Windowsx.h>
#include <commctrl.h>
#pragma comment(lib,"comctl32.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include <Shellapi.h>
#include <Shlwapi.h>
#pragma comment(lib,"Shlwapi.lib")
#include <comdef.h>
#include <comutil.h>
#include <Shldisp.h>
#include <shlobj_core.h>
#include <psapi.h>
#include <vector>
#include <map>
#include <format>
#include <dwmapi.h>
#pragma comment(lib,"dwmapi.lib")
#define PATHCCH_NO_DEPRECATE
#include <pathcch.h>
#pragma comment(lib,"Pathcch.lib")
#include <htmlhelp.h>
#pragma comment(lib,"htmlhelp.lib")
#include <string>
#include <memory>
#include <regex>
#include <algorithm>
#endif //PCH_H
