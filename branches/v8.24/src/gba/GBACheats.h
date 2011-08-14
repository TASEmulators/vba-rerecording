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
	u32 rawaddress;
	u32  address;
	u32  value;
	u32  oldValue;
	char codestring[20];
	char desc[32];
};

void cheatsAdd(const char *codeStr, const char *desc, u32 rawaddress, u32 address, u32 value, int code, int size);
void cheatsAddCheatCode(const char *code, const char *desc);
void cheatsAddGSACode(const char *code, const char *desc, bool v3);
void cheatsAddCBACode(const char *code, const char *desc);
bool cheatsImportGSACodeFile(const char *name, int game, bool v3);
void cheatsDelete(int number, bool restore);
void cheatsDeleteAll(bool restore);
void cheatsEnable(int number);
void cheatsDisable(int number);
void cheatsSaveGame(gzFile file);
void cheatsReadGame(gzFile file, int version);
void cheatsReadGameSkip(gzFile file, int version);
void cheatsSaveCheatList(const char *file);
bool cheatsLoadCheatList(const char *file);
void cheatsWriteMemory(u32 address, u32 value);
void cheatsWriteHalfWord(u32 address, u16 value);
void cheatsWriteByte(u32 address, u8 value);
int	 cheatsCheckKeys(u32 keys, u32 extended);

extern int        cheatsNumber;
extern CheatsData cheatsList[100];

#define CHEAT_IS_HEX(a) (((a) >= 'A' && (a) <= 'F') || ((a) >= '0' && (a) <= '9'))

#define CHEAT_PATCH_ROM_16BIT(a, v) \
    WRITE16LE(((u16 *)&rom[(a) & 0x1ffffff]), v);

#define CHEAT_PATCH_ROM_32BIT(a, v) \
    WRITE32LE(((u32 *)&rom[(a) & 0x1ffffff]), v);

#endif // GBA_CHEATS_H
