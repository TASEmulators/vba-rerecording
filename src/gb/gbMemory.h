#ifndef VBA_GB_MEMORY_H
#define VBA_GB_MEMORY_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../Port.h"

struct mapperMBC1
{
	int32 mapperRAMEnable;
	int32 mapperROMBank;
	int32 mapperRAMBank;
	int32 mapperMemoryModel;
	int32 mapperROMHighAddress;
	int32 mapperRAMAddress;
};

struct mapperMBC2
{
	int32 mapperRAMEnable;
	int32 mapperROMBank;
};

struct mapperMBC3
{
	int32  mapperRAMEnable;
	int32  mapperROMBank;
	int32  mapperRAMBank;
	int32  mapperRAMAddress;
	int32  mapperClockLatch;
	int32  mapperClockRegister;
	int32  mapperSeconds;
	int32  mapperMinutes;
	int32  mapperHours;
	int32  mapperDays;
	int32  mapperControl;
	int32  mapperLSeconds;
	int32  mapperLMinutes;
	int32  mapperLHours;
	int32  mapperLDays;
	int32  mapperLControl;
	time_t mapperLastTime;
};

struct mapperMBC5
{
	int32 mapperRAMEnable;
	int32 mapperROMBank;
	int32 mapperRAMBank;
	int32 mapperROMHighAddress;
	int32 mapperRAMAddress;
	int32 isRumbleCartridge;
};

struct mapperMBC7
{
	int32 mapperRAMEnable;
	int32 mapperROMBank;
	int32 mapperRAMBank;
	int32 mapperRAMAddress;
	int32 cs;
	int32 sk;
	int32 state;
	int32 buffer;
	int32 idle;
	int32 count;
	int32 code;
	int32 address;
	int32 writeEnable;
	int32 value;
};

struct mapperHuC1
{
	int32 mapperRAMEnable;
	int32 mapperROMBank;
	int32 mapperRAMBank;
	int32 mapperMemoryModel;
	int32 mapperROMHighAddress;
	int32 mapperRAMAddress;
};

struct mapperHuC3
{
	int32 mapperRAMEnable;
	int32 mapperROMBank;
	int32 mapperRAMBank;
	int32 mapperRAMAddress;
	int32 mapperAddress;
	int32 mapperRAMFlag;
	int32 mapperRAMValue;
	int32 mapperRegister1;
	int32 mapperRegister2;
	int32 mapperRegister3;
	int32 mapperRegister4;
	int32 mapperRegister5;
	int32 mapperRegister6;
	int32 mapperRegister7;
	int32 mapperRegister8;
};

extern mapperMBC1 gbDataMBC1;
extern mapperMBC2 gbDataMBC2;
extern mapperMBC3 gbDataMBC3;
extern mapperMBC5 gbDataMBC5;
extern mapperHuC1 gbDataHuC1;
extern mapperHuC3 gbDataHuC3;

void mapperMBC1ROM(u16, u8);
void mapperMBC1RAM(u16, u8);
void mapperMBC2ROM(u16, u8);
void mapperMBC2RAM(u16, u8);
void mapperMBC3ROM(u16, u8);
void mapperMBC3RAM(u16, u8);
u8   mapperMBC3ReadRAM(u16);
void mapperMBC5ROM(u16, u8);
void mapperMBC5RAM(u16, u8);
void mapperMBC7ROM(u16, u8);
void mapperMBC7RAM(u16, u8);
u8   mapperMBC7ReadRAM(u16);
void mapperHuC1ROM(u16, u8);
void mapperHuC1RAM(u16, u8);
void mapperHuC3ROM(u16, u8);
void mapperHuC3RAM(u16, u8);
u8   mapperHuC3ReadRAM(u16);

//extern void (*mapper)(u16,u8);
//extern void (*mapperRAM)(u16,u8);
//extern u8 (*mapperReadRAM)(u16);

extern void memoryUpdateMapMBC1();
extern void memoryUpdateMapMBC2();
extern void memoryUpdateMapMBC3();
extern void memoryUpdateMapMBC5();
extern void memoryUpdateMapMBC7();
extern void memoryUpdateMapHuC1();
extern void memoryUpdateMapHuC3();

#endif // VBA_GB_MEMORY
