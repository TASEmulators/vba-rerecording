#include "System.h"
#include "SystemGlobals.h"
#include "inputGlobal.h"
#include "../gb/gbGlobals.h"
#include "../gba/GBA.h"
#include "../common/movie.h"
#include "../common/vbalua.h"

// evil static variables
static u32		lastFrameTime	= 0;
static int32	frameSkipCount	= 0;
static int32	frameCount		= 0;
static bool		pauseAfterFrameAdvance = false;

// systemABC stuff are core-related

// motion sensor
void systemSetSensorX(int32 x)
{
	sensorX = x;
}

void systemSetSensorY(int32 y)
{
	sensorY = y;
}

void systemResetSensor()
{
	sensorX = sensorY = SYSTEM_SENSOR_INIT_VALUE;
}

int32 systemGetSensorX()
{
	return sensorX;
}

int32 systemGetSensorY()
{
	return sensorY;
}

void systemUpdateMotionSensor(int i)
{
	if (i < 0 || i > 3)
		i = 0;

	if (currentButtons[i] & BUTTON_MASK_LEFT_MOTION)
	{
		sensorX += 3;
		if (sensorX > 2197)
			sensorX = 2197;
		if (sensorX < 2047)
			sensorX = 2057;
	}
	else if (currentButtons[i] & BUTTON_MASK_RIGHT_MOTION)
	{
		sensorX -= 3;
		if (sensorX < 1897)
			sensorX = 1897;
		if (sensorX > 2047)
			sensorX = 2037;
	}
	else if (sensorX > 2047)
	{
		sensorX -= 2;
		if (sensorX < 2047)
			sensorX = 2047;
	}
	else
	{
		sensorX += 2;
		if (sensorX > 2047)
			sensorX = 2047;
	}

	if (currentButtons[i] & BUTTON_MASK_UP_MOTION)
	{
		sensorY += 3;
		if (sensorY > 2197)
			sensorY = 2197;
		if (sensorY < 2047)
			sensorY = 2057;
	}
	else if (currentButtons[i] & BUTTON_MASK_DOWN_MOTION)
	{
		sensorY -= 3;
		if (sensorY < 1897)
			sensorY = 1897;
		if (sensorY > 2047)
			sensorY = 2037;
	}
	else if (sensorY > 2047)
	{
		sensorY -= 2;
		if (sensorY < 2047)
			sensorY = 2047;
	}
	else
	{
		sensorY += 2;
		if (sensorY > 2047)
			sensorY = 2047;
	}
}

// joypads
u32 systemGetJoypad(int i, bool sensor)
{
	if (i < 0 || i > 3)
		i = 0;

	// input priority: original = raw+auto < Lua < movie, correct this if wrong

	// get original+auto input
	u32 res = systemGetOriginalJoypad(i, sensor);

	// Lua input, shouldn't have any side effect within it
	if (VBALuaUsingJoypad(i))
		res = VBALuaReadJoypad(i);

	// therefore, lua is currently allowed to modify the extbuttons...
	u32 extButtons = res & BUTTON_NONRECORDINGONLY_MASK;

	// movie input has the highest priority
	if (VBAMoviePlaying())
	{
		// VBAMovieRead() overwrites currentButtons[i]
		VBAMovieRead(i, sensor);
	}
	else
	{
		currentButtons[i] = res & BUTTON_REGULAR_RECORDING_MASK;
		if (VBAMovieRecording())
		{
			// the "current buttons" input buffer will be read by the movie routine
			VBAMovieWrite(i, sensor);
		}
	}

	return currentButtons[i] | extButtons;
}

void systemSetJoypad(int which, u32 buttons)
{
	if (which < 0 || which > 3)
		which = 0;

	currentButtons[which] = buttons;

	lastKeys = 0;
}

void systemClearJoypads()
{
	for (int i = 0; i < 3; ++i)
		currentButtons[i] = 0;

	lastKeys = 0;
}

// emulation
void systemCleanUp()
{
	// frame counting
	frameCount	   = 0;
	frameSkipCount = systemFramesToSkip();
	lastFrameTime  = systemGetClock();
	pauseAfterFrameAdvance = false;

	extButtons = 0;
	newFrame   = true;

	systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

	systemResetSensor();

	systemCounters.frameCount = 0;
	systemCounters.extraCount = 0;
	systemCounters.lagCount	  = 0;
	systemCounters.lagged	  = true;
	systemCounters.laggedLast = true;

	systemClearJoypads();
}

void systemReset()
{
	// frame counting
	frameCount	   = 0;
	frameSkipCount = systemFramesToSkip();
	lastFrameTime  = systemGetClock();
	pauseAfterFrameAdvance = false;

	extButtons = 0;
	newFrame   = true;

	systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

	systemResetSensor();

	if (!VBAMovieActive())
	{
		systemCounters.frameCount = 0;
		systemCounters.extraCount = 0;
		systemCounters.lagCount	  = 0;
		systemCounters.lagged	  = true;
		systemCounters.laggedLast = true;
	}
}

bool systemFrameDrawingRequired()
{
	return frameSkipCount >= systemFramesToSkip() || pauseAfterFrameAdvance;
}

void systemFrameBoundaryWork()
{
	newFrame = true;

	systemFrame();

	++frameCount;
	u32 currentTime = systemGetClock();
	if (currentTime - lastFrameTime >= 1000)
	{
		systemShowSpeed(int(float(frameCount) * 100000 / (float(currentTime - lastFrameTime) * 60) + .5f));
		lastFrameTime = currentTime;
		frameCount = 0;
	}

	++systemCounters.frameCount;
	if (systemCounters.lagged)
	{
		++systemCounters.lagCount;
	}
	systemCounters.laggedLast = systemCounters.lagged;
	systemCounters.lagged	 = true;

	pauseAfterFrameAdvance = systemPauseOnFrame();

	if (systemFrameDrawingRequired())
	{
		systemRenderFrame();
		frameSkipCount = 0;

		bool capturePressed = (extButtons & 2) != 0;
		if (capturePressed && !capturePrevious)
		{
			captureNumber = systemScreenCapture(captureNumber);
		}
		capturePrevious = capturePressed && !pauseAfterFrameAdvance;
	}
	else
	{
		++frameSkipCount;
	}

	CallRegisteredLuaFunctions(LUACALL_AFTEREMULATION);

	if (VBALuaRunning())
	{
		VBALuaFrameBoundary();
	}

	VBAMovieUpdateState();

	if (pauseAfterFrameAdvance)
	{
		systemSetPause(true);
	}
}

// BIOS, only GBA BIOS supported at present
bool systemLoadBIOS(const char *biosFileName, bool useBiosFile)
{
	useBios = false;
	if (useBiosFile)
	{
		useBios = utilLoadBIOS(bios, biosFileName, 4);
	}
	if (!useBios)
	{
		CPULoadInternalBios();
	}
	return useBios;
}

// graphics
// overloads #0
int32 systemGetLCDSizeType()
{
	switch (systemCartridgeType)
	{
	case IMAGE_GBA:
		return GBA_NDR;
	case IMAGE_GB:
		return gbBorderOn ? SGB_NDR : GB_NDR;
	default:
		return UNKNOWN_NDR;
	}
}

void systemGetLCDResolution(int32 &width, int32& height)
{
	switch (systemGetLCDSizeType())
	{
	default:
		width = 240;
		height = 160;
		break;
	case GB_NDR:
		width = 160;
		height = 144;
		break;
	case SGB_NDR:
		width = 256;
		height = 224;
		break;
	}
}

void systemGetLCDBaseSize(int32 &width, int32& height)
{
	switch (systemGetLCDSizeType())
	{
	default:
		width = 240;
		height = 160;
		break;
	case GB_NDR:
	case SGB_NDR:
		width = 160;
		height = 144;
		break;
	}
}

void systemGetLCDBaseOffset(int32 &xofs, int32 &yofs)
{
	switch (systemGetLCDSizeType())
	{
	default:
		xofs = 0;
		yofs = 0;
		break;
	case SGB_NDR:
		xofs = 48;
		yofs = 40;
		break;
	}
}
