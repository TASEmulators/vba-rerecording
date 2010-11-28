#ifndef VBA_FLASH_H
#define VBA_FLASH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zlib.h"
#include "../Port.h"

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
