#ifndef VBA_LUA_H
#define VBA_LUA_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum LuaCallID
{
	LUACALL_BEFOREEMULATION,
	LUACALL_AFTEREMULATION,
	LUACALL_BEFOREEXIT,
	LUACALL_COUNT
};
void CallRegisteredLuaFunctions(LuaCallID calltype);

// Just forward function declarations

void VBALuaWrite(uint32 addr);
void VBALuaFrameBoundary();
int VBALoadLuaCode(const char *filename);
int VBAReloadLuaCode();
void VBALuaStop();
int VBALuaRunning();

int VBALuaUsingJoypad(int);
int VBALuaReadJoypad(int);
int VBALuaSpeed();
bool8 VBALuaRerecordCountSkip();

void VBALuaGui(uint8 *screen, int ppl, int width, int height);
void VBALuaClearGui();

// And some interesting REVERSE declarations!
char *VBAGetFreezeFilename(int slot);

void VBALuaWriteInform(uint32 addr);

#endif // VBA_LUA_H
