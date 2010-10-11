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

#ifndef VBA_GB_H
#define VBA_GB_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zlib.h"
#include "../Port.h"

// FIXME: constant (GB) or boolean (GBA)?!
#define C_FLAG 0x10
#define H_FLAG 0x20
#define N_FLAG 0x40
#define Z_FLAG 0x80

typedef union
{
	struct
	{
#ifdef WORDS_BIGENDIAN
		u8 B1, B0;
#else
		u8 B0, B1;
#endif
	} B;
	u16 W;
} gbRegister;

extern bool gbLoadRom(const char *);
extern void gbEmulate(int);
extern bool gbIsGameboyRom(const char *);
extern void gbSoundReset();
extern void gbSoundSetQuality(int);
extern void gbReset(bool userReset = false);
extern void gbCleanUp();
extern bool gbWriteBatteryFile(const char *);
extern bool gbWriteBatteryFile(const char *, bool);
extern bool gbWriteBatteryToStream(gzFile);
extern bool gbReadBatteryFile(const char *);
extern bool gbReadBatteryFromStream(gzFile);
extern bool gbWriteSaveState(const char *);
extern bool gbWriteMemSaveState(char *, int);
extern bool gbReadSaveState(const char *);
extern bool gbReadMemSaveState(char *, int);
extern bool gbReadSaveStateFromStream(gzFile);
extern bool gbWriteSaveStateToStream(gzFile);
extern void gbSgbRenderBorder();
extern bool gbWritePNGFile(const char *);
extern bool gbWriteBMPFile(const char *);
extern bool gbReadGSASnapshot(const char *);

extern struct EmulatedSystem GBSystem;
extern struct EmulatedSystemCounters &GBSystemCounters;

#endif // VBA_GB_H
