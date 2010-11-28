#ifndef VBA_GBA_CHEATS_H
#define VBA_GBA_CHEATS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "zlib.h"
#include "../Port.h"

struct CheatsData
{
	int  code;
	int  size;
	int  status;
	bool enabled;
	u32  address;
	u32  value;
	u32  oldValue;
	char codestring[20];
	char desc[32];
};

extern void cheatsAdd(const char *, const char *, u32, u32, int, int);
extern void cheatsAddCheatCode(const char *code, const char *desc);
extern void cheatsAddGSACode(const char *code, const char *desc, bool v3);
extern void cheatsAddCBACode(const char *code, const char *desc);
extern bool cheatsImportGSACodeFile(const char *name, int game, bool v3);
extern void cheatsDelete(int number, bool restore);
extern void cheatsDeleteAll(bool restore);
extern void cheatsEnable(int number);
extern void cheatsDisable(int number);
extern void cheatsSaveGame(gzFile file);
extern void cheatsReadGame(gzFile file);
extern void cheatsSaveCheatList(const char *file);
extern bool cheatsLoadCheatList(const char *file);
extern void       cheatsWriteMemory(u32 *, u32, u32);
extern void       cheatsWriteHalfWord(u16 *, u16, u16);
extern void       cheatsWriteByte(u8 *, u8);
extern int        cheatsCheckKeys(u32, u32);
extern int        cheatsNumber;
extern CheatsData cheatsList[100];

#endif // GBA_CHEATS_H
