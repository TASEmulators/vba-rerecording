#ifndef VBA_GBAINLINE_H
#define VBA_GBAINLINE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Port.h"

// moved from GBA.h
typedef struct
{
	u8 *address;
	u32 mask;
} memoryMap;

extern memoryMap map[256];

#if 0

#define CPUReadByteQuick(addr) \
    map[(addr) >> 24].address[(addr) & map[(addr) >> 24].mask]

#define CPUReadHalfWordQuick(addr) \
    READ16LE(((u16 *)&map[(addr) >> 24].address[(addr) & map[(addr) >> 24].mask]))

#define CPUReadMemoryQuick(addr) \
    READ32LE(((u32 *)&map[(addr) >> 24].address[(addr) & map[(addr) >> 24].mask]))

//inline u32 CPUReadMemoryQuick(u32 addr)
//{
//	u32 addrShift = (addr)>>24;
//	u32 rt = (addr) & map[addrShift].mask;
//	return READ32LE(((u32*)&map[addrShift].address[rt]));
//}

#else

// GBA memory I/O functions copied from win32/MemoryViewerDlg.cpp
static inline u8 CPUReadByteQuick(u32 addr)
{
	return ::map[addr >> 24].address[addr & ::map[addr >> 24].mask];
}

static inline u16 CPUReadHalfWordQuick(u32 addr)
{
	return READ16LE(&::map[addr >> 24].address[addr & ::map[addr >> 24].mask]);
}

static inline u32 CPUReadMemoryQuick(u32 addr)
{
	return READ32LE(&::map[addr >> 24].address[addr & ::map[addr >> 24].mask]);
}

static inline void CPUWriteByteQuick(u32 addr, u8 b)
{
	::map[addr >> 24].address[addr & ::map[addr >> 24].mask] = b;
}

static inline void CPUWriteHalfWordQuick(u32 addr, u16 b)
{
	WRITE16LE(&::map[addr >> 24].address[addr & ::map[addr >> 24].mask], b);
}

static inline void CPUWriteMemoryQuick(u32 addr, u32 b)
{
	WRITE32LE(&::map[addr >> 24].address[addr & ::map[addr >> 24].mask], b);
}

#endif

#endif // VBA_GBAINLINE_H
