#ifndef _VBALUA_H
#define _VBALUA_H


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

void VBALuaGui(uint8 *screen, int ppl, int width, int height, int depth);
void VBALuaClearGui();

// And some interesting REVERSE declarations!
char *VBAGetFreezeFilename(int slot);

// See getset.h!
//inline void VBALuaWriteInform(uint32 address);


#endif
