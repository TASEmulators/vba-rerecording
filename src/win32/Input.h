// -*- C++ -*-
// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef VBA_WIN32_INPUT_H
#define VBA_WIN32_INPUT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/inputGlobal.h"

#define JOYCONFIG_MESSAGE (WM_USER + 1000)

class Input
{
public:
	Input() {};
	virtual ~Input() {};

	virtual bool initialize() = 0;

	virtual bool readDevices() = 0;
	virtual u32 readDevice(int which, bool sensor, bool free) = 0;
	virtual CString getKeyName(LONG_PTR key) = 0;
	virtual void checkKeys()    = 0;
	virtual void checkDevices() = 0;
	virtual void activate()     = 0;
	virtual void loadSettings() = 0;
	virtual void saveSettings() = 0;
};

#endif
