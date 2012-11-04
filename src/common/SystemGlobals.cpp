#include "SystemGlobals.h"

// FIXME: it must be admitted that the naming schema is a whole mess
EmulatedSystem theEmulator;

EmulatedSystemCounters systemCounters =
{
	// frameCount
	0,
	// lagCount
	0,
	// extraCount
	0,
	// lagged
	true,
	// laggedLast
	true,
};

int	emulating = 0;

u8 *bios = NULL;
u8 *pix	 = NULL;

u16	  currentButtons[4] = { 0, 0, 0, 0 };	// constrain: never contains hacked buttons, only the lower 16 bits of each are used
u16   lastKeys = 0;
int32 sensorX = 0;
int32 sensorY = 0;

bool	  newFrame			   = true;
bool8	  speedup			   = false;
u32		  extButtons		   = 0;
bool8	  capturePrevious	   = false;
int32	  captureNumber		   = 0;

// settings
bool8	  synchronize		 = true;
int32	  gbFrameSkip		 = 0;
int32	  frameSkip			 = 0;

bool8	  cpuDisableSfx		   = false;
int32	  layerSettings		   = 0xff00;

#ifdef USE_GB_CORE_V7
bool8	  gbNullInputHackEnabled	 = false;
bool8	  gbNullInputHackTempEnabled = false;
#endif

#ifdef USE_GBA_CORE_V7
bool8	  memLagEnabled		   = false;
bool8	  memLagTempEnabled	   = false;
#endif

bool8	  useOldFrameTiming	   = false;
bool8	  useBios			   = false;
bool8	  skipBios			   = false;
bool8	  skipSaveGameBattery  = false;
bool8	  skipSaveGameCheats   = false;
bool8	  cheatsEnabled		   = true;
bool8	  mirroringEnable	   = false;

bool8	  cpuEnhancedDetection = true;

bool tempSaveSafe	  = true;
int	 tempSaveID		  = 0;
int	 tempSaveAttempts = 0;
