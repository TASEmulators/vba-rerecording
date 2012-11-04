#ifndef VBA_SYSTEM_GLOBALS_H
#define VBA_SYSTEM_GLOBALS_H

#include "../Port.h"
#include "Util.h"

// c++ lacks a way to implement Smart Referrences or Delphi-Style Properties
// in order to maintain consistency, value-copied things should not be modified too often
struct EmulatedSystem
{
	// main emulation function
	void (*emuMain)(int);
	// reset emulator
	void (*emuReset)();
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
	int32 extraCount;
	bool8 lagged;
	bool8 laggedLast;
};

extern struct EmulatedSystem theEmulator;
extern struct EmulatedSystemCounters systemCounters;

extern int emulating;

extern u8 *bios;
extern u8 *pix;

extern u16 currentButtons[4];
extern u16 lastKeys;

extern int32 sensorX, sensorY;

extern bool	 newFrame;
extern bool8 speedup;
extern u32	 extButtons;
extern bool8 capturePrevious;
extern int32 captureNumber;

typedef int32 soundtick_t;

extern soundtick_t USE_TICKS_AS;
extern soundtick_t soundTickStep;
extern soundtick_t soundTicks;

extern u32	 soundIndex;
extern int32 soundPaused;
extern int32 soundPlay;
extern u32	 soundNextPosition;

extern u8	 soundBuffer[6][735];
extern u32	 soundBufferLen;
extern u32	 soundBufferTotalLen;
extern u32	 soundBufferIndex;

extern u16	 soundFinalWave[1470];
extern u16	 soundFrameSound[735 * 30 * 2];
extern int32 soundFrameSoundWritten;

extern bool	 tempSaveSafe;
extern int	 tempSaveID;
extern int	 tempSaveAttempts;

//settings
extern bool8 synchronize;
extern int32 gbFrameSkip;
extern int32 frameSkip;

extern bool8 cpuDisableSfx;
extern int32 layerSettings;

#ifdef USE_GB_CORE_V7
extern bool8 gbNullInputHackEnabled;
extern bool8 gbNullInputHackTempEnabled;
#endif

#ifdef USE_GBA_CORE_V7
extern bool8 memLagEnabled;
extern bool8 memLagTempEnabled;
#endif

extern bool8 useOldFrameTiming;
extern bool8 useBios;
extern bool8 skipBios;
extern bool8 skipSaveGameBattery; // skip battery data when reading save states
extern bool8 skipSaveGameCheats; // skip cheat list data when reading save states
extern bool8 cheatsEnabled;
extern bool8 mirroringEnable;

extern bool8 cpuEnhancedDetection;

extern int32 soundVolume;
extern int32 soundQuality;
extern bool8 soundEcho;
extern bool8 soundLowPass;
extern bool8 soundReverse;
extern bool8 soundOffFlag;
extern int32 soundEnableFlag;

#endif
