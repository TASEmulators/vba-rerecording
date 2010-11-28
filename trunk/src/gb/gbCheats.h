#ifndef VBA_GB_CHEATS_H
#define VBA_GB_CHEATS_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct gbXxCheat
{
	char cheatDesc[100];
	char cheatCode[20];
};

struct gbCheat
{
	char cheatCode[20];
	char cheatDesc[32];
	u16  address;
	int  code;
	u8   compare;
	u8   value;
	bool enabled;
};

extern void gbCheatsSaveGame(gzFile);
extern void gbCheatsReadGame(gzFile, int);
extern void gbCheatsSaveCheatList(const char *);
extern bool gbCheatsLoadCheatList(const char *);
extern bool gbCheatReadGSCodeFile(const char *);

extern void gbAddGsCheat(const char *, const char *);
extern void gbAddGgCheat(const char *, const char *);
extern void gbCheatRemove(int);
extern void gbCheatRemoveAll();
extern void gbCheatEnable(int);
extern void gbCheatDisable(int);
extern u8 gbCheatRead(u16);

extern int     gbCheatNumber;
extern gbCheat gbCheatList[100];
extern bool    gbCheatMap[0x10000];

#endif // VBA_GB_CHEATS_H

