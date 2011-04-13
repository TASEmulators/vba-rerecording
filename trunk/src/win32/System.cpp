// System.cpp : Defines the system behaviors for the emulator.
//
#include "stdafx.h"
#include "Sound.h"
#include "Input.h"
#include "IUpdate.h"
#include "ram_search.h"
#include "WinMiscUtil.h"
#include "WinResUtil.h"
#include "resource.h"
#include "VBA.h"
#include "../gba/GBA.h"
#include "../gba/GBAGlobals.h"
#include "../gba/GBASound.h"
#include "../gb/GB.h"
#include "../gb/gbGlobals.h"
//#include "../common/System.h"
#include "../common/movie.h"
#include "../common/vbalua.h"
#include "../common/Text.h"
#include "../common/Util.h"
#include "../common/nesvideos-piece.h"
#include "../version.h"
#include <cassert>

struct EmulatedSystem theEmulator;

u32	 RGB_LOW_BITS_MASK		 = 0;
int	 emulating				 = 0;
int	 systemCartridgeType	 = 0;
int	 systemSpeed			 = 0;
bool systemSoundOn			 = false;
u32	 systemColorMap32[0x10000];
u16	 systemColorMap16[0x10000];
u16	 systemGbPalette[24];
int	 systemRedShift			 = 0;
int	 systemBlueShift		 = 0;
int	 systemGreenShift		 = 0;
int	 systemColorDepth		 = 16;
int	 systemDebug			 = 0;
int	 systemVerbose			 = 0;
int	 systemFrameSkip		 = 0;
int	 systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

const int32 INITIAL_SENSOR_VALUE = 2047;

int32 sensorX = INITIAL_SENSOR_VALUE;
int32 sensorY = INITIAL_SENSOR_VALUE;
u16	  currentButtons [4] = { 0, 0, 0, 0 };	// constrain: never contains hacked buttons, only the lower 16 bits of each are used
u16   lastKeys = 0;

// static_assertion that BUTTON_REGULAR_RECORDING_MASK should be an u16 constant
namespace { const void * const s_STATIC_ASSERTION_(static_cast<void *>(BUTTON_REGULAR_RECORDING_MASK & 0xFFFF0000)); }

#define BMP_BUFFER_MAX_WIDTH (256)
#define BMP_BUFFER_MAX_HEIGHT (224)
#define BMP_BUFFER_MAX_DEPTH (4)
static u8 bmpBuffer[BMP_BUFFER_MAX_WIDTH * BMP_BUFFER_MAX_HEIGHT * BMP_BUFFER_MAX_DEPTH];

static int s_stockThrottleValues[] = {
	6, 15, 25, 25, 37, 50, 75, 87, 100, 112, 125, 150, 200, 300, 400, 600, 800, 1000
};

// systemXYZ: Win32 stuff

// input

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
	sensorX = sensorY = INITIAL_SENSOR_VALUE;
}

int32 systemGetSensorX()
{
	return sensorX;
}

int32 systemGetSensorY()
{
	return sensorY;
}

// handles motion sensor input
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

int systemGetDefaultJoypad()
{
	return theApp.joypadDefault;
}

void systemSetDefaultJoypad(int which)
{
	theApp.joypadDefault = which;
}

bool systemReadJoypads()
{
	// this function is called at every frame, even if vba is fast-forwarded.
	// so we try to limit the input frequency here just in case.
	static u32 lastTime = systemGetClock();
	if ((u32)(systemGetClock() - lastTime) < 10)
		return false;
	lastTime = systemGetClock();

	if (theApp.input)
		return theApp.input->readDevices();
	return false;
}

u32 systemGetOriginalJoypad(int i, bool sensor)
{
	if (i < 0 || i > 3)
		i = 0;

	u32 res = 0;
	if (theApp.input)
		res = theApp.input->readDevice(i, sensor);

	// +auto input, XOR
	// maybe these should be moved into DirectInput.cpp
	if (theApp.autoFire || theApp.autoFire2)
	{
		res ^= (theApp.autoFireToggle ? theApp.autoFire : theApp.autoFire2);
		if (!theApp.autofireAccountForLag || !systemCounters.laggedLast)
		{
			theApp.autoFireToggle = !theApp.autoFireToggle;
		}
	}
	if (theApp.autoHold)
	{
		res ^= theApp.autoHold;
	}

	// filter buttons
	// maybe better elsewhere?
	if (!theApp.allowLeftRight)
	{
		// disallow L+R or U+D to being pressed at the same time
		if ((res & (BUTTON_MASK_RIGHT | BUTTON_MASK_LEFT)) == (BUTTON_MASK_RIGHT | BUTTON_MASK_LEFT))
			res &= ~BUTTON_MASK_RIGHT;  // leave only LEFT on
		if ((res & (BUTTON_MASK_DOWN | BUTTON_MASK_UP)) == (BUTTON_MASK_DOWN | BUTTON_MASK_UP))
			res &= ~BUTTON_MASK_DOWN;  // leave only UP on
	}

	if (!sensor)
	{
		if (res & BUTTON_MOTION_MASK)
			res &= ~BUTTON_MOTION_MASK;
	}

	if (systemCartridgeType != 0 && !gbSgbMode) // regular GB has no L/R buttons
	{
		if (res & (BUTTON_GBA_ONLY))
			res &= ~BUTTON_GBA_ONLY;
	}

	currentButtons[i] = res & BUTTON_REGULAR_RECORDING_MASK;

	return res;
}

u32 systemGetJoypad(int i, bool sensor)
{
	if (i < 0 || i > 3)
		i = 0;

	// input priority: original+auto < Lua < frame search < movie, correct this if wrong

	// get original+auto input
	u32 hackedButtons = systemGetOriginalJoypad(i, sensor) & BUTTON_NONRECORDINGONLY_MASK;
	u32 res = currentButtons[i];

	// since movie input has the highest priority, there's no point to read from other input
	if (VBAMoviePlaying())
	{
		// VBAMovieRead() overwrites currentButtons[i]
		VBAMovieRead(i, sensor);
		res = currentButtons[i];
	}
	else
	{
		// Lua input, shouldn't have any side effect within them
		if (VBALuaUsingJoypad(i))
			res = VBALuaReadJoypad(i);

		// override input above
		if (theApp.frameSearchSkipping)
			res = theApp.frameSearchOldInput[i];

		// flush non-hack buttons into the "current buttons" input buffer, which will be read by the movie routine
		currentButtons[i] = res & BUTTON_REGULAR_RECORDING_MASK;
		VBAMovieWrite(i, sensor);
	}

	return res | hackedButtons;
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

// screen

// delayed repaint
void systemRefreshScreen()
{
	if (theApp.m_pMainWnd)
	{
		theApp.m_pMainWnd->PostMessage(WM_PAINT, NULL, NULL);
	}
}

extern bool vbaShuttingDown;

void systemRenderFrame()
{
	extern long linearSoundFrameCount;
	extern long linearFrameCount;

	if (vbaShuttingDown)
		return;

	++theApp.renderedFrames;

	VBAUpdateFrameCountDisplay();
	VBAUpdateButtonPressDisplay();

	// "in-game" text rendering
	if (textMethod == 0) // transparent text can only be painted once, so timed messages will not be updated
	{
		extern void DrawLuaGui();
		DrawLuaGui();

		int copyX = 240, copyY = 160;
		if (systemCartridgeType == 1)
			if (gbBorderOn)
				copyX = 256, copyY = 224;
			else
				copyX = 160, copyY = 144;
		int pitch = copyX * (systemColorDepth / 8) + (systemColorDepth == 24 ? 0 : 4);  // FIXME: sure?

		DrawTextMessages((u8 *)pix, pitch, 0, copyY);
	}

	++linearFrameCount;
	if (!theApp.sound)
	{
		if (linearFrameCount > 10000)
			linearFrameCount -= 10000;
		linearSoundFrameCount = linearFrameCount;
	}

	// record avi
	int width  = 240;
	int height = 160;
	switch (systemCartridgeType)
	{
	case 0:
		width  = 240;
		height = 160;
		break;
	case 1:
		if (gbBorderOn)
		{
			width  = 256;
			height = 224;
		}
		else
		{
			width  = 160;
			height = 144;
		}
		break;
	}

	bool firstFrameLogged = false;
	--linearFrameCount;
	do
	{
		++linearFrameCount;

		if (theApp.aviRecording && (!theApp.altAviRecordMethod || (theApp.altAviRecordMethod && !firstFrameLogged)))
		{
			// usually aviRecorder is created when vba starts avi recording, though
			if (theApp.aviRecorder == NULL)
			{
				theApp.aviRecorder = new AVIWrite();

				theApp.aviRecorder->SetFPS(60);

				BITMAPINFOHEADER bi;
				memset(&bi, 0, sizeof(bi));
				bi.biSize	   = 0x28;
				bi.biPlanes	   = 1;
				bi.biBitCount  = 24;
				bi.biWidth	   = width;
				bi.biHeight	   = height;
				bi.biSizeImage = 3 * width * height;
				theApp.aviRecorder->SetVideoFormat(&bi);
				if (!theApp.aviRecorder->Open(theApp.aviRecordName))
				{
					delete theApp.aviRecorder;
					theApp.aviRecorder	= NULL;
					theApp.aviRecording = false;
				}
			}

			if (theApp.aviRecorder != NULL && !theApp.aviRecorder->IsPaused())
			{
				assert(
				    width <= BMP_BUFFER_MAX_WIDTH && height <= BMP_BUFFER_MAX_HEIGHT && systemColorDepth <=
				    BMP_BUFFER_MAX_DEPTH * 8);
				utilWriteBMP(bmpBuffer, width, height, systemColorDepth, pix);
				theApp.aviRecorder->AddFrame(bmpBuffer);
			}
		}

		if (theApp.nvVideoLog)
		{
			// convert from whatever bit depth to 16-bit, while stripping away extra pixels
			assert(width <= BMP_BUFFER_MAX_WIDTH && height <= BMP_BUFFER_MAX_HEIGHT && 16 <= BMP_BUFFER_MAX_DEPTH * 8);
			utilWriteBMP(bmpBuffer, width, -height, 16, pix);
			NESVideoLoggingVideo((u8 *)bmpBuffer, width, height, 0x1000000 * 60);
		}

		firstFrameLogged = true;
	}
	while (linearFrameCount < linearSoundFrameCount); // compensate for frames lost due to frame skip being nonzero, etc.

	if (textMethod != 0) // do not draw Lua HUD to a video dump
	{
		extern void DrawLuaGui();
		DrawLuaGui();
	}

	// interframe blending
	if (theApp.ifbFunction)
	{
		if (systemColorDepth == 16)
			theApp.ifbFunction(pix + theApp.filterWidth * 2 + 4, theApp.filterWidth * 2 + 4,
			                   theApp.filterWidth, theApp.filterHeight);
		else
			theApp.ifbFunction(pix + theApp.filterWidth * 4 + 4, theApp.filterWidth * 4 + 4,
			                   theApp.filterWidth, theApp.filterHeight);
	}

	systemRedrawScreen();
}

void systemRedrawScreen()
{
	if (vbaShuttingDown)
		return;

	if (theApp.display)
		theApp.display->render();

	systemUpdateListeners();
}

void systemUpdateListeners()
{
	if (vbaShuttingDown)
		return;

	Update_RAM_Search(); // updates RAM search and RAM watch

	// update viewers etc.
	if (theApp.updateCount)
	{
		POSITION pos = theApp.updateList.GetHeadPosition();
		while (pos)
		{
			IUpdateListener *up = theApp.updateList.GetNext(pos);
			if (up)
				up->update();
		}
	}
}

int systemScreenCapture(int captureNumber)
{
	return winScreenCapture(captureNumber);
}

void systemMessage(int number, const char *defaultMsg, ...)
{
	CString buffer;
	va_list valist;
	CString msg = defaultMsg;
	if (number)
		msg = winResLoadString(number);

	va_start(valist, defaultMsg);
	buffer.FormatV(msg, valist);

	theApp.winCheckFullscreen();
	systemSoundClearBuffer();
	AfxGetApp()->m_pMainWnd->MessageBox(buffer, winResLoadString(IDS_ERROR), MB_OK | MB_ICONERROR);

	va_end(valist);
}

void systemScreenMessage(const char *msg, int slot, int duration, const char *colorList)
{
	if (slot < 0 || slot > SCREEN_MESSAGE_SLOTS)
		return;

	theApp.screenMessage[slot] = true;
	theApp.screenMessageTime[slot]		  = GetTickCount();
	theApp.screenMessageDuration[slot]	  = duration;
	theApp.screenMessageBuffer[slot]	  = msg;
	theApp.screenMessageColorBuffer[slot] = colorList ? colorList : "";

	if (theApp.screenMessageBuffer[slot].GetLength() > 40)
		theApp.screenMessageBuffer[slot] = theApp.screenMessageBuffer[slot].Left(40);

	// update the display when a main slot message appears while the game is paused
	if (slot == 0 && (theApp.paused || (theApp.frameSearching)))
		systemRefreshScreen();
}

void systemShowSpeed(int speed)
{
	systemSpeed = speed;
	theApp.showRenderedFrames = theApp.renderedFrames;
	theApp.renderedFrames	  = 0;
	if (theApp.videoOption <= VIDEO_4X && theApp.showSpeed)
	{
		CString buffer;
		if (theApp.showSpeed == 1)
			buffer.Format(VBA_NAME_AND_VERSION " %3d%%", systemSpeed);
		else
			buffer.Format(VBA_NAME_AND_VERSION " %3d%% (%d fps | %d skipped)",
			              systemSpeed,
			              theApp.showRenderedFrames,
			              systemFrameSkip);

		systemSetTitle(buffer);
	}
}

void systemSetTitle(const char *title)
{
	if (theApp.m_pMainWnd != NULL)
	{
		AfxGetApp()->m_pMainWnd->SetWindowText(title);
	}
}

// timing/speed

u32 systemGetClock()
{
	return timeGetTime();
}

void systemIncreaseThrottle()
{
	int throttle = theApp.throttle;

	if (throttle < 6)
		++throttle;
	else if (throttle < s_stockThrottleValues[_countof(s_stockThrottleValues) - 1])
	{
		int i = 0;
		while (throttle >= s_stockThrottleValues[i])
		{
			++i;
		}
		throttle = s_stockThrottleValues[i];
	}

	systemSetThrottle(throttle);
}

void systemDecreaseThrottle()
{
	int throttle = theApp.throttle;

	if (throttle > 6)
	{
		int i = _countof(s_stockThrottleValues) - 1;
		while (throttle <= s_stockThrottleValues[i])
		{
			--i;
		}
		throttle = s_stockThrottleValues[i];
	}
	else if (throttle > 1)
		--throttle;

	systemSetThrottle(throttle);
}

void systemSetThrottle(int throttle)
{
	theApp.throttle = throttle;
	char str[256];
	sprintf(str, "%d%% throttle speed", theApp.throttle);
	systemScreenMessage(str);
}

int systemGetThrottle()
{
	return theApp.throttle;
}

void systemFrame()
{
	if (theApp.altAviRecordMethod && theApp.aviRecording)
	{
		if (theApp.aviRecorder)
		{
			if (!theApp.aviRecorder->IsSoundAdded())
			{
				WAVEFORMATEX wfx;
				memset(&wfx, 0, sizeof(wfx));
				wfx.wFormatTag		= WAVE_FORMAT_PCM;
				wfx.nChannels		= 2;
				wfx.nSamplesPerSec	= 44100 / soundQuality;
				wfx.wBitsPerSample	= 16;
				wfx.nBlockAlign		= (wfx.wBitsPerSample / 8) * wfx.nChannels;
				wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
				wfx.cbSize = 0;
				theApp.aviRecorder->SetSoundFormat(&wfx);
			}
			theApp.aviRecorder->AddSound((u8 *)soundFrameSound, soundFrameSoundWritten * 2);
		}
	}

	soundFrameSoundWritten = 0;

	// no more stupid updates :)

	extern int quitAfterTime;                   // from VBA.cpp
	void	   VBAMovieStop(bool8 suppress_message); // from ../movie.cpp
	if (quitAfterTime >= 0 && systemCounters.frameCount == quitAfterTime)
	{
		VBAMovieStop(true);
		AfxPostQuitMessage(0);
	}

	// change the sound speed, or set it to normal - must always do this or it won't get reset after a change, but that's OK
	// because it's inexpensive
	if (theApp.sound)
	{
		theApp.sound->setSpeed(
		    speedup || theApp.winPauseNextFrame || !synchronize || theApp.accuratePitchThrottle || theApp.useOldSync
			? 1.0f : (float)theApp.throttle / 100.0f);
	}

	// if a throttle speed is set and we're not allowed to change the sound frequency to achieve it,
	// sleep for a certain amount each time we get here to approximate the necessary slowdown
	if (synchronize && (theApp.accuratePitchThrottle || !theApp.sound || theApp.throttle < 6) /*&& !theApp.winPauseNextFrame*/)
	{
		/// FIXME: this is still a horrible way of achieving a certain frame time
		///        (look at what Snes9x does - it's complicated but much much better)

		static float sleepAmt = 0.0f; // variable to smooth out the sleeping amount so it doesn't oscillate so fast
//		if(!theApp.wasPaused) {
		if (!speedup)
		{
			u32 time = systemGetClock();
			u32 diff = time - theApp.throttleLastTime;
			if (theApp.wasPaused)
				diff = 0;

			int target = (100000 / (60 * theApp.throttle));
			int d	   = (target - diff);

			if (d > 1000) // added to avoid 500-day waits for vba to start emulating.
				d = 1000;  // I suspect most users aren't that patient, and would find 1 second to be a more reasonable delay.

			sleepAmt = 0.8f * sleepAmt + 0.2f * (float)d;
			if (d - sleepAmt <= 1.5f && d - sleepAmt >= -1.5f)
				d = (int)(sleepAmt);

			if (d > 0)
			{
				Sleep(d);
			}
		}
		theApp.throttleLastTime = systemGetClock();
		//}
		//else
		//{
		// Sleep(100);
		//}
	}

	if (systemCounters.frameCount % 10 == 0)
	{
		if (theApp.rewindMemory)
		{
			if (++theApp.rewindCounter >= (theApp.rewindTimer))
			{
				theApp.rewindSaveNeeded = true;
				theApp.rewindCounter	= 0;
			}
		}
		if (systemSaveUpdateCounter)
		{
			if (--systemSaveUpdateCounter <= SYSTEM_SAVE_NOT_UPDATED)
			{
				winWriteBatteryFile();
				systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
			}
		}
	}

	theApp.wasPaused = false;
///  theApp.autoFrameSkipLastTime = time;
}

int systemFramesToSkip()
{
	int framesToSkip = systemFrameSkip;

	bool fastForward = speedup;

#if (defined(WIN32) && !defined(SDL))
	fastForward = (fastForward || theApp.frameSearchSkipping);
	int throttle = theApp.throttle;
	if (theApp.frameSearching && throttle < 100)
		throttle = 100;
#else
	extern int throttle;
#endif

#if (defined(WIN32) && !defined(SDL))
	if (theApp.aviRecording || theApp.nvVideoLog)
	{
		framesToSkip = 0; // render all frames
	}
	else
	{
		if (fastForward)
			framesToSkip = 9;  // try 6 FPS during speedup
		else if (throttle != 100)
			framesToSkip = (framesToSkip * throttle) / 100;
	}
#endif

	return framesToSkip;
}

// sound

bool systemSoundInit()
{
	if (theApp.sound)
		delete theApp.sound;

	extern ISound *newDirectSound();
	theApp.sound = newDirectSound();
	return theApp.sound->init();
}

void systemSoundShutdown()
{
	if (theApp.sound)
		delete theApp.sound;
	theApp.sound = NULL;
}

void systemSoundPause()
{
	if (theApp.sound)
		theApp.sound->pause();
	soundPaused = 1;
}

void systemSoundResume()
{
	if (theApp.sound)
		theApp.sound->resume();
	soundPaused = 0;
}

bool systemSoundIsPaused()
{
//	return soundPaused;
	return !(theApp.sound && theApp.sound->isPlaying());
}

void systemSoundClearBuffer()
{
	if (theApp.sound)
		theApp.sound->clearAudioBuffer();
}

void systemSoundReset()
{
	if (theApp.sound)
		theApp.sound->reset();
}

void systemSoundWriteToBuffer()
{
	if (theApp.sound)
		theApp.sound->write();
}

bool systemSoundCanChangeQuality()
{
	return true;
}

bool systemSoundSetQuality(int quality)
{
	if (systemCartridgeType == 0)
		soundSetQuality(quality);
	else
		gbSoundSetQuality(quality);

	return true;
}

// emulation

bool systemIsEmulating()
{
	return emulating != 0;
}

void systemGbBorderOn()
{
	if (vbaShuttingDown)
		return;

	if (emulating && systemCartridgeType == 1)
	{
		theApp.updateWindowSize(theApp.videoOption);
	}
}

bool systemIsRunningGBA()
{
	return (systemCartridgeType == 0);
}

bool systemIsSpedUp()
{
	return theApp.speedupToggle;
}

bool systemIsPaused()
{
	return theApp.paused;
}

void systemSetPause(bool pause)
{
	if (pause)
	{
		capturePrevious	 = false;
		theApp.wasPaused = true;
		theApp.paused	 = true;
		theApp.speedupToggle = false;
		theApp.winPauseNextFrame = false;
		systemSoundPause();
		systemRefreshScreen();;
	}
	else
	{
		theApp.paused	 = false;
		systemSoundResume();
	}
}

// aka. frame advance
bool systemPauseOnFrame()
{
	if (theApp.winPauseNextFrame)
	{
		if (!theApp.nextframeAccountForLag || !systemCounters.laggedLast)
		{
			theApp.winPauseNextFrame   = false;
			return true;
		}
	}

	return false;
}

bool systemLoadBIOS(const char *biosFileName, bool useBiosFile)
{
	bool use = false;
	if (systemCartridgeType == 0)
		use = CPULoadBios(biosFileName, useBiosFile);
	else
		use = false;
	return use;
}

// FIXME: now platform-independant stuff
// it should be admitted that the naming schema/code organization is a whole mess
// these things should be moved somewhere else

EmulatedSystemCounters systemCounters =
{
	// frameCount
	0,
	// lagCount
	0,
	// lagged
	true,
	// laggedLast
	true,
};

// VBAxyz stuff are not part of the core.

void VBAOnEnteringFrameBoundary()
{
	CallRegisteredLuaFunctions(LUACALL_AFTEREMULATION);

	if (VBALuaRunning())
	{
		VBALuaFrameBoundary();
	}

	VBAMovieUpdateState();
}

void VBAOnExitingFrameBoundary()
{
	;
}

//////////////////////////////////////////////
// ultility

extern void toolsLog(const char *);

void log(const char *msg, ...)
{
	CString buffer;
	va_list valist;

	va_start(valist, msg);
	buffer.FormatV(msg, valist);

	toolsLog(buffer);

	va_end(valist);
}
