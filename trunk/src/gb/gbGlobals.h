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

#ifndef VBA_GB_GLOBALS_H
#define VBA_GB_GLOBALS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Port.h"

extern int32 gbRomSizeMask;
extern int32 gbRomSize;
extern int32 gbRamSize;
extern int32 gbRamSizeMask;

extern u8 * gbRom;
extern u8 * gbRam;
extern u8 * gbVram;
extern u8 * gbWram;
extern u8 * gbMemory;
extern u16 *gbLineBuffer;

extern u8 *gbMemoryMap[16];

inline u8 gbReadMemoryQuick(u16 address)
{
	extern int32 gbEchoRAMFixOn;
	if (gbEchoRAMFixOn)
	{
		if (address >= 0xe000 && address < 0xfe00)
		{
			address -= 0x2000;
		}
	}
	return gbMemoryMap[address>>12][address&0xfff];
}

inline void gbWriteMemoryQuick(u16 address, u8 value)
{
	extern int32 gbEchoRAMFixOn;
	if (gbEchoRAMFixOn)
	{
		if (address >= 0xe000 && address < 0xfe00)
		{
			address -= 0x2000;
		}
	}
	gbMemoryMap[address>>12][address&0xfff] = value;
}

extern int32 gbFrameSkip;
extern u16   gbColorFilter[32768];
extern int32 gbColorOption;
extern int32 gbPaletteOption;
extern int32 gbEmulatorType;
extern int32 gbBorderOn;
extern int32 gbBorderAutomatic;
extern int32 gbCgbMode;
extern int32 gbSgbMode;
extern int32 gbWindowLine;
extern int32 gbSpeed;
extern u8    gbBgp[4];
extern u8    gbObp0[4];
extern u8    gbObp1[4];
extern u16   gbPalette[128];

extern u8 register_LCDC;
extern u8 register_LY;
extern u8 register_SCY;
extern u8 register_SCX;
extern u8 register_WY;
extern u8 register_WX;
extern u8 register_VBK;

extern int emulating;

extern int32 gbBorderLineSkip;
extern int32 gbBorderRowSkip;
extern int32 gbBorderColumnSkip;
extern int32 gbDmaTicks;

extern void gbRenderLine();
extern void gbDrawSprites();

extern u8 (*gbSerialFunction)(u8);

#endif // VBA_GB_GLOBALS_H
