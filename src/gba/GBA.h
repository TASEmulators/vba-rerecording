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

#ifndef VBA_GBA_H
#define VBA_GBA_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zlib.h"
#include "../Port.h"

#if (defined(WIN32) && !defined(SDL))
#include "../win32/stdafx.h" // for HANDLE
//#include <windows.h> // for HANDLE
// NOTE: if you get this error:
// #error WINDOWS.H already included.  MFC apps must not #include <windows.h>
// it is probably because stdafx.h is getting included at the wrong place
// (i.e. after anything else) in a file, or your precompiled headers are otherwise wrong
#endif

#define SAVE_GAME_VERSION_1 1
#define SAVE_GAME_VERSION_2 2
#define SAVE_GAME_VERSION_3 3
#define SAVE_GAME_VERSION_4 4
#define SAVE_GAME_VERSION_5 5
#define SAVE_GAME_VERSION_6 6
#define SAVE_GAME_VERSION_7 7
#define SAVE_GAME_VERSION_8 8
#define SAVE_GAME_VERSION_9 9
#define SAVE_GAME_VERSION_10 10
#define SAVE_GAME_VERSION_11 11
#define SAVE_GAME_VERSION_12 12
#define SAVE_GAME_VERSION_13 13
#define SAVE_GAME_VERSION  SAVE_GAME_VERSION_13

#if (defined(WIN32) && !defined(SDL))
extern HANDLE mapROM;        // shared memory handles
extern HANDLE mapWORKRAM;
extern HANDLE mapBIOS;
extern HANDLE mapIRAM;
extern HANDLE mapPALETTERAM;
extern HANDLE mapVRAM;
extern HANDLE mapOAM;
extern HANDLE mapPIX;
extern HANDLE mapIOMEM;
#endif

/*
extern reg_pair reg[45];
extern u8       biosProtected[4];

extern bool8 N_FLAG;
extern bool8 Z_FLAG;
extern bool8 C_FLAG;
extern bool8 V_FLAG;
extern bool8 armIrqEnable;
extern bool8 armState;
extern int32 armMode;
*/
extern void  (*cpuSaveGameFunc)(u32, u8);

extern bool8 freezeWorkRAM[0x40000];
extern bool8 freezeInternalRAM[0x8000];
extern bool CPUReadGSASnapshot(const char *);
extern bool CPUWriteGSASnapshot(const char *, const char *, const char *, const char *);
extern bool CPUWriteBatteryFile(const char *);
extern bool CPUReadBatteryFile(const char *);
extern bool CPUWriteBatteryToStream(gzFile);
extern bool CPUReadBatteryFromStream(gzFile);
extern bool CPUExportEepromFile(const char *);
extern bool CPUImportEepromFile(const char *);
extern bool CPUWritePNGFile(const char *);
extern bool CPUWriteBMPFile(const char *);
extern void CPUCleanUp();
extern void CPUUpdateRender();
extern bool CPUReadMemState(char *, int);
extern bool CPUReadState(const char *);
extern bool CPUWriteMemState(char *, int);
extern bool CPUWriteState(const char *);
extern bool CPUReadStateFromStream(gzFile);
extern bool CPUWriteStateToStream(gzFile);
extern int CPULoadRom(const char *);
extern void CPUUpdateRegister(u32, u16);
extern void CPUWriteHalfWord(u32, u16);
extern void CPUWriteByte(u32, u8);
extern void CPUInit(const char *, bool);
extern void CPUReset(bool userReset = false);
extern void CPULoop(int);
extern void CPUCheckDMA(int, int);
#ifdef PROFILING
extern void cpuProfil(char *buffer, int, u32, int);
extern void cpuEnableProfiling(int hz);
#endif

extern struct EmulatedSystem GBASystem;

#define R13_IRQ  18
#define R14_IRQ  19
#define SPSR_IRQ 20
#define R13_USR  26
#define R14_USR  27
#define R13_SVC  28
#define R14_SVC  29
#define SPSR_SVC 30
#define R13_ABT  31
#define R14_ABT  32
#define SPSR_ABT 33
#define R13_UND  34
#define R14_UND  35
#define SPSR_UND 36
#define R8_FIQ   37
#define R9_FIQ   38
#define R10_FIQ  39
#define R11_FIQ  40
#define R12_FIQ  41
#define R13_FIQ  42
#define R14_FIQ  43
#define SPSR_FIQ 44

#endif // VBA_GBA_H
