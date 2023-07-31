/**
 * Copyright (c) 2023 thf
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `ClassicTileRegUtil.cpp` for details.
 */
#pragma once

// Library of registry functions for serializing state of notification item settings 
namespace ClassicTileRegUtil
{
	LONG CheckRegAppPath();
	LONG DeleteRegAppPath();
	LONG GetRegLeftClickAction(DWORD& dwLeftClickAction);
	LONG SetRegLeftClickAction(DWORD dwLeftClickAction);
	LONG GetRegLogging(bool& bLogging);
	LONG SetRegLogging(bool bLogging);
	LONG CheckRegRun();
	LONG SetRegRun();
	LONG DeleteRegRun();
	LONG GetRegDefWndTile(bool& bDefWndTile);
	LONG SetRegDefWndTile(bool bDefWndTile);
	
}