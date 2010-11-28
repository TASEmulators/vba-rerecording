#ifndef VBA_GBA_SOUND_H
#define VBA_GBA_SOUND_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zlib.h"
#include "../Port.h"

#define NR10 0x60
#define NR11 0x62
#define NR12 0x63
#define NR13 0x64
#define NR14 0x65
#define NR21 0x68
#define NR22 0x69
#define NR23 0x6c
#define NR24 0x6d
#define NR30 0x70
#define NR31 0x72
#define NR32 0x73
#define NR33 0x74
#define NR34 0x75
#define NR41 0x78
#define NR42 0x79
#define NR43 0x7c
#define NR44 0x7d
#define NR50 0x80
#define NR51 0x81
#define NR52 0x84
#define SGCNT0_H 0x82
#define FIFOA_L 0xa0
#define FIFOA_H 0xa2
#define FIFOB_L 0xa4
#define FIFOB_H 0xa6

extern void soundTick();
extern void soundShutdown();
extern bool soundInit();
extern void soundPause();
extern void soundResume();
extern void soundToggle(int);
extern void soundEnable(int);
extern void soundDisable(int);
extern void soundSetMuted(bool isMuted);
extern int  soundGetEnable();
extern int  soundGetMuted();
extern void soundReset();
extern void soundSaveGame(gzFile);
extern void soundReadGame(gzFile, int);
extern void soundEvent(u32, u8);
extern void soundEvent(u32, u16);
extern void soundTimerOverflow(int);
extern void soundSetQuality(int);

typedef int32 soundtick_t;

extern soundtick_t SOUND_CLOCK_TICKS;
extern soundtick_t soundTicks;
extern int32       soundPaused;
extern bool8       soundOffFlag;
extern int32       soundQuality;
extern u32         soundBufferLen;
extern u32         soundBufferTotalLen;
extern u32         soundNextPosition;
extern u16         soundFinalWave[1470];
extern u16         soundFrameSound[735*30*2];
extern int32       soundFrameSoundWritten;
extern int32       soundVolume;

extern bool8 soundEcho;
extern bool8 soundLowPass;
extern bool8 soundReverse;

#endif // VBA_GBA_SOUND_H
