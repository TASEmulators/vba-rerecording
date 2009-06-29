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

#ifndef VBA_FLASH_H
#define VBA_FLASH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zlib.h"
#include "Port.h"

extern void flashSaveGame(gzFile gzFile);
extern void flashReadGame(gzFile gzFile, int version);
extern u8 flashRead(u32 address);
extern void flashWrite(u32 address, u8 byte);
extern u8 flashSaveMemory[0x20000 + 4];
extern void flashSaveDecide(u32 address, u8 byte);
extern void flashReset();
extern void flashErase();
extern void flashSetSize(int size);

extern int32 flashSize;

#endif // VBA_FLASH_H
