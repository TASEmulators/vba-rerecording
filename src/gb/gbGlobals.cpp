#include "../Port.h"
#include "GB.h"

u8 *gbMemoryMap[16];

int32 gbRomSizeMask = 0;
int32 gbRomSize     = 0;
int32 gbRamSizeMask = 0;
int32 gbRamSize     = 0;

u8 * gbMemory     = NULL;
u8 * gbVram       = NULL;
u8 * gbRom        = NULL;
u8 * gbRam        = NULL;
u8 * gbWram       = NULL;
u16 *gbLineBuffer = NULL;

u16   gbPalette[128];
u8    gbBgp[4] = { 0, 1, 2, 3};
u8    gbObp0[4] = { 0, 1, 2, 3};
u8    gbObp1[4] = { 0, 1, 2, 3};
int32 gbWindowLine = -1;

int32 gbCgbMode = 0;

u16   gbColorFilter[32768];
int32 gbColorOption      = 0;
int32 gbPaletteOption    = 0;
int32 gbEmulatorType     = 0;
int32 gbBorderOn         = 1;
int32 gbBorderAutomatic  = 0;
int32 gbBorderLineSkip   = 160;
int32 gbBorderRowSkip    = 0;
int32 gbBorderColumnSkip = 0;
int32 gbDmaTicks         = 0;
bool8 gbNullInputHackEnabled	 = false;
bool8 gbNullInputHackTempEnabled = false;

u8 (*gbSerialFunction)(u8) = NULL;
