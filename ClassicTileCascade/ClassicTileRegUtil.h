/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ClassicTileRegUtil.cpp` for details.
 */
#pragma once

namespace ClassicTileRegUtil
{
	LONG CheckRegAppPath();
	LONG DeleteRegAppPath();
	LONG GetRegLeftClickAction(DWORD& dwLeftClickAction);
	LONG SetRegLeftClickAction(DWORD dwLeftClickAction);
	LONG GetRegLogging(DWORD& dwLogging);
	LONG SetRegLogging(DWORD dwLogging);
	LONG CheckRegRun();
	LONG SetRegRun();
	LONG DeleteRegRun();
	LONG GetRegDefWndTile(DWORD& dwDefWndTile);
	LONG SetRegDefWndTile(DWORD dwDefWndTile);

}