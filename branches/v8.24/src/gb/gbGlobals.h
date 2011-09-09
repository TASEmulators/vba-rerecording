#ifndef VBA_GB_GLOBALS_H
#define VBA_GB_GLOBALS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

extern int32 gbRomSizeMask;
extern int32 gbRomSize;
extern int32 gbRamSize;
extern int32 gbRamSizeMask;
extern int32 gbTAMA5ramSize;

extern bool8 useBios;
extern bool8 skipBios;
extern u8 *	 bios;
extern bool8 skipSaveGameBattery;
extern bool8 skipSaveGameCheats;

extern u8 * gbRom;
extern u8 * gbRam;
extern u8 * gbVram;
extern u8 * gbWram;
extern u8 * gbMemory;
extern u16 *gbLineBuffer;
extern u8 * gbTAMA5ram;

extern u8 *gbMemoryMap[16];

inline u8 gbReadMemoryQuick(u16 address)
{
	if (address >= 0xe000 && address < 0xfe00)
	{
		address -= 0x2000;
	}
	return gbMemoryMap[address >> 12][address & 0xfff];
}

inline void gbWriteMemoryQuick(u16 address, u8 value)
{
	if (address >= 0xe000 && address < 0xfe00)
	{
		address -= 0x2000;
	}
	gbMemoryMap[address >> 12][address & 0xfff] = value;
}

// GB
static inline u8 gbReadMemoryQuick8(u16 addr)
{
	return gbReadMemoryQuick(addr);
}

static inline void gbWriteMemoryQuick8(u16 addr, u8 b)
{
	gbWriteMemoryQuick(addr, b);
}

static inline u16 gbReadMemoryQuick16(u16 addr)
{
	return (gbReadMemoryQuick(addr + 1) << 8) | gbReadMemoryQuick(addr);
}

static inline void gbWriteMemoryQuick16(u16 addr, u16 b)
{
	gbWriteMemoryQuick(addr, b & 0xff);
	gbWriteMemoryQuick(addr + 1, (b >> 8) & 0xff);
}

static inline u32 gbReadMemoryQuick32(u16 addr)
{
	return (gbReadMemoryQuick(addr + 3) << 24) |
	       (gbReadMemoryQuick(addr + 2) << 16) |
	       (gbReadMemoryQuick(addr + 1) << 8) |
	       gbReadMemoryQuick(addr);
}

static inline void gbWriteMemoryQuick32(u16 addr, u32 b)
{
	gbWriteMemoryQuick(addr, b & 0xff);
	gbWriteMemoryQuick(addr + 1, (b >> 8) & 0xff);
	gbWriteMemoryQuick(addr + 2, (b >> 16) & 0xff);
	gbWriteMemoryQuick(addr + 1, (b >> 24) & 0xff);
}

extern int	 gbFrameSkip;
extern u16	 gbColorFilter[32768];
extern int	 gbColorOption;
extern int32 gbPaletteOption;
extern int	 gbEmulatorType;
extern int32 gbBorderOn;
extern int	 gbBorderAutomatic;
extern int	 gbCgbMode;
extern int	 gbSgbMode;
extern int	 gbWindowLine;
extern int	 gbSpeed;
extern u8	 gbBgp[4];
extern u8	 gbObp0[4];
extern u8	 gbObp1[4];
extern u16	 gbPalette[128];
extern bool  gbScreenOn;
extern bool  gbDrawWindow;
extern u8	 gbSCYLine[300];
// gbSCXLine is used for the emulation (bug) of the SX change
// found in the Artic Zone game.
extern u8 gbSCXLine[300];
// gbBgpLine is used for the emulation of the
// Prehistorik Man's title screen scroller.
extern u8 gbBgpLine[300];
extern u8 gbObp0Line [300];
extern u8 gbObp1Line [300];
// gbSpritesTicks is used for the emulation of Parodius' Laser Beam.
extern u8 gbSpritesTicks[300];

extern u8 register_LCDC;
extern u8 register_LY;
extern u8 register_SCY;
extern u8 register_SCX;
extern u8 register_WY;
extern u8 register_WX;
extern u8 register_VBK;
extern u8 oldRegister_WY;

extern int	emulating;
extern bool genericflashcardEnable;

extern int gbBorderLineSkip;
extern int gbBorderRowSkip;
extern int gbBorderColumnSkip;
extern int gbDmaTicks;

extern void gbRenderLine();
extern void gbDrawSprites(bool);

extern u8 (*gbSerialFunction)(u8);

#endif // VBA_GB_GLOBALS_H
