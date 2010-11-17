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

#ifndef VBA_SYSTEM_H
#define VBA_SYSTEM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zlib.h"
#include "../Port.h"

// c++ lacks a way to implement Smart Referrences or Delphi-Style Properties
// in order to maintain consistency, value-copied things should not be modified too often
struct EmulatedSystem
{
	// main emulation function
	void (*emuMain)(int);
	// reset emulator
	void (*emuReset)(bool);
	// clean up memory
	void (*emuCleanUp)();
	// load battery file
	bool (*emuReadBattery)(const char *);
	// write battery file
	bool (*emuWriteBattery)(const char *);
	// load battery file from stream
	bool (*emuReadBatteryFromStream)(gzFile);
	// write battery file to stream
	bool (*emuWriteBatteryToStream)(gzFile);
	// load state
	bool (*emuReadState)(const char *);
	// save state
	bool (*emuWriteState)(const char *);
	// load state from stream
	bool (*emuReadStateFromStream)(gzFile);
	// save state to stream
	bool (*emuWriteStateToStream)(gzFile);
	// load memory state (rewind)
	bool (*emuReadMemState)(char *, int);
	// write memory state (rewind)
	bool (*emuWriteMemState)(char *, int);
	// write PNG file
	bool (*emuWritePNG)(const char *);
	// write BMP file
	bool (*emuWriteBMP)(const char *);
	// emulator update CPSR (ARM only)
	void (*emuUpdateCPSR)();
	// emulator has debugger
	bool emuHasDebugger;
	// clock ticks to emulate
	int emuCount;
};

// why not convert the value type only when doing I/O?
struct EmulatedSystemCounters
{
	int32 frameCount;
	int32 lagCount;
	bool8 lagged;
	bool8 laggedLast;
};

extern struct EmulatedSystem theEmulator;
extern struct EmulatedSystemCounters systemCounters;

extern void log(const char *, ...);

extern void systemGbPrint(u8 *, int, int, int, int);
extern int  systemScreenCapture(int);
extern void systemRefreshScreen();
extern void systemRenderFrame();
extern void systemRedrawScreen();
extern void systemUpdateListeners();
// updates the joystick data
extern void systemSetSensorX(int32);
extern void systemSetSensorY(int32);
extern void systemResetSensor();
extern int32 systemGetSensorX();
extern int32 systemGetSensorY();
extern void systemUpdateMotionSensor(int);
extern int  systemGetDefaultJoypad();
extern void systemSetDefaultJoypad(int);
extern bool systemReadJoypads();
// return information about the given joystick, -1 for default joystick... the bool is for if motion sensor should be handled
// too
extern u32  systemGetOriginalJoypad(int, bool);
extern u32  systemGetJoypad(int, bool);
extern void systemSetJoypad(int, u32);
extern void systemClearJoypads();
extern void systemMessage(int, const char *, ...);
extern void systemSetTitle(const char *);
extern void systemWriteDataToSoundBuffer();
extern void systemSoundShutdown();
extern void systemSoundPause();
extern void systemSoundResume();
extern bool systemSoundIsPaused();
extern void systemSoundClearBuffer();
extern void systemSoundReset();
extern bool systemSoundInit();
extern void systemScreenMessage(const char *msg, int slot = 0, int duration = 3000, const char *colorList = NULL);
extern bool systemCanChangeSoundQuality();
extern bool systemSetSoundQuality(int quality);
extern void systemShowSpeed(int);
extern u32  systemGetClock();
extern void systemIncreaseThrottle();
extern void systemDecreaseThrottle();
extern void systemSetThrottle(int);
extern int  systemGetThrottle();
extern void systemFrame();
extern bool systemIsEmulating();
extern void systemGbBorderOn();
extern bool systemIsRunningGBA();
extern bool systemIsSpedUp();
extern bool systemIsPaused();
extern void systemSetPause(bool pause);
extern bool systemPauseOnFrame();

extern int	systemCartridgeType;
extern int  systemSpeed;
extern bool systemSoundOn;
extern u16  systemColorMap16[0x10000];
extern u32  systemColorMap32[0x10000];
extern u16  systemGbPalette[24];
extern int  systemRedShift;
extern int  systemGreenShift;
extern int  systemBlueShift;
extern int  systemColorDepth;
extern int  systemDebug;
extern int  systemVerbose;
extern int  systemFrameSkip;
extern int  systemSaveUpdateCounter;

#define SYSTEM_SAVE_UPDATED 30
#define SYSTEM_SAVE_NOT_UPDATED 0

#endif // VBA_SYSTEM_H
