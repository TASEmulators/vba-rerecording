// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

// VBA.cpp : Defines the class behaviors for the application.
//
#include "stdafx.h"
#include <mmsystem.h>
#include <assert.h>

#include "resource.h"
#include "VBA.h"
#include "AVIWrite.h"
#include "Input.h"
#include "IUpdate.h"
#include "LangSelect.h"
#include "MainWnd.h"
#include "Reg.h"
#include "Sound.h"
#include "WavWriter.h"
#include "WinResUtil.h"
#include "ramwatch.h"

//#include "../System.h"
#include "../GBA.h"
#include "../Globals.h"
#include "../agbprint.h"
#include "../gb/GB.h"
#include "../gb/gbGlobals.h"
#include "../gb/gbPrinter.h"
#include "../cheatSearch.h"
#include "../RTC.h"
#include "../Sound.h"
#include "../Util.h"
#include "../Text.h"
#include "../movie.h"
#include "../nesvideos-piece.h"
#include "../vbalua.h"

extern void Pixelate(u8*, u32, u8*, u8*, u32, int, int);
extern void Pixelate32(u8*, u32, u8*, u8*, u32, int, int);
extern void MotionBlur(u8*, u32, u8*, u8*, u32, int, int);
extern void MotionBlur32(u8*, u32, u8*, u8*, u32, int, int);
extern void _2xSaI(u8*, u32, u8*, u8*, u32, int, int);
extern void _2xSaI32(u8*, u32, u8*, u8*, u32, int, int);
extern void Super2xSaI(u8*, u32, u8*, u8*, u32, int, int);
extern void Super2xSaI32(u8*, u32, u8*, u8*, u32, int, int);
extern void SuperEagle(u8*, u32, u8*, u8*, u32, int, int);
extern void SuperEagle32(u8*, u32, u8*, u8*, u32, int, int);
extern void AdMame2x(u8*, u32, u8*, u8*, u32, int, int);
extern void AdMame2x32(u8*, u32, u8*, u8*, u32, int, int);
extern void Simple2x(u8*, u32, u8*, u8*, u32, int, int);
extern void Simple2x32(u8*, u32, u8*, u8*, u32, int, int);
extern void Bilinear(u8*, u32, u8*, u8*, u32, int, int);
extern void Bilinear32(u8*, u32, u8*, u8*, u32, int, int);
extern void BilinearPlus(u8*, u32, u8*, u8*, u32, int, int);
extern void BilinearPlus32(u8*, u32, u8*, u8*, u32, int, int);
extern void Scanlines(u8*, u32, u8*, u8*, u32, int, int);
extern void Scanlines32(u8*, u32, u8*, u8*, u32, int, int);
extern void ScanlinesTV(u8*, u32, u8*, u8*, u32, int, int);
extern void ScanlinesTV32(u8*, u32, u8*, u8*, u32, int, int);
extern void hq2x(u8*, u32, u8*, u8*, u32, int, int);
extern void hq2x32(u8*, u32, u8*, u8*, u32, int, int);
extern void hq2xS(u8*, u32, u8*, u8*, u32, int, int);
extern void hq2xS32(u8*, u32, u8*, u8*, u32, int, int);
extern void lq2x(u8*, u32, u8*, u8*, u32, int, int);
extern void lq2x32(u8*, u32, u8*, u8*, u32, int, int);
extern void hq3x(u8*, u32, u8*, u8*, u32, int, int);
extern void hq3x32(u8*, u32, u8*, u8*, u32, int, int);
extern void hq3xS(u8*, u32, u8*, u8*, u32, int, int);
extern void hq3xS32(u8*, u32, u8*, u8*, u32, int, int);

extern void SmartIB(u8*, u32, int, int);
extern void SmartIB32(u8*, u32, int, int);
extern void MotionBlurIB(u8*, u32, int, int);
extern void InterlaceIB(u8*, u32, int, int);
extern void MotionBlurIB32(u8*, u32, int, int);

extern void toolsLog(const char *);

extern IDisplay *newGDIDisplay();
extern IDisplay *newDirectDrawDisplay();
extern IDisplay *newDirect3DDisplay();
extern IDisplay *newOpenGLDisplay();

extern Input *newDirectInput();

extern ISound *newDirectSound();

extern void remoteStubSignal(int, int);
extern void remoteOutput(char *, u32);
extern void remoteStubMain();
extern void remoteSetProtocol(int);
extern void remoteCleanUp();
extern int remoteSocket;

extern void InterframeCleanup();

void winlog(const char *msg, ...);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

int  emulating         = 0;
bool debugger          = false;
int  RGB_LOW_BITS_MASK = 0;

int  systemFrameSkip = 0;
int  systemSpeed     = 0;
bool systemSoundOn   = false;
u32  systemColorMap32[0x10000];
u16  systemColorMap16[0x10000];
u16  systemGbPalette[24];
int  systemRedShift = 0;
int  systemBlueShift         = 0;
int  systemGreenShift        = 0;
int  systemColorDepth        = 16;
int  systemVerbose           = 0;
int  systemDebug             = 0;
int  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

#define BMP_BUFFER_MAX_WIDTH (256)
#define BMP_BUFFER_MAX_HEIGHT (224)
#define BMP_BUFFER_MAX_DEPTH (4)
static u8 bmpBuffer [BMP_BUFFER_MAX_WIDTH*BMP_BUFFER_MAX_HEIGHT*BMP_BUFFER_MAX_DEPTH];

char movieFileToPlay [1024];
bool playMovieFile         = false;
bool playMovieFileReadOnly = false;
char wavFileToOutput [1024];
bool outputWavFile  = false;
bool outputAVIFile  = false;
bool flagHideMenu   = false;
int  quitAfterTime  = -1;
int  pauseAfterTime = -1;

void winSignal(int, int);
void winOutput(char *, u32);

void (*dbgSignal)(int, int)    = winSignal;
void (*dbgOutput)(char *, u32) = winOutput;

#ifdef MMX
extern "C" bool cpu_mmx;
#endif

void directXMessage(const char *msg)
{
	systemMessage(
	    IDS_DIRECTX_7_REQUIRED,
	    "DirectX 7.0 or greater is required to run.\nDownload at http://www.microsoft.com/directx.\n\nError found at: %s",
	    msg);
}

void DrawTextMessages(u8 *dest, int pitch, int left, int bottom)
{
	for (int slot = 0; slot < SCREEN_MESSAGE_SLOTS; slot++)
	{
		if (theApp.screenMessage[slot])
		{
			if (((int)(GetTickCount() - theApp.screenMessageTime[slot]) < theApp.screenMessageDuration[slot]) &&
			    (!theApp.disableStatusMessage || slot == 1 || slot == 2))
			{
				drawText(dest,
				         pitch,
				         left,
				         bottom - 10*(slot+1),
				         theApp.screenMessageBuffer[slot],
				         theApp.screenMessageColorBuffer[slot]);
			}
			else
			{
				theApp.screenMessage[slot] = false;
			}
		}
	}
}

// draw Lua graphics in game screen
void DrawLuaGui()
{
	int copyX       = 240, copyY = 160;
	int screenX     = 240, screenY = 160;
	int copyOffsetX = 0, copyOffsetY = 0;
	int pitch;
	if (theApp.cartridgeType == 1)
	{
		if (gbBorderOn)
		{
			copyX       = 256, copyY = 224;
			copyOffsetX = 48, copyOffsetY = 40;
		}
		else
		{
			copyX = 160, copyY = 144;
		}
		screenX = 160, screenY = 144;
	}
	pitch = copyX*(systemColorDepth/8)+(systemColorDepth == 24 ? 0 : 4);

	copyOffsetY++; // don't know why it's needed

	VBALuaGui(&pix[copyOffsetY*pitch+copyOffsetX*(systemColorDepth/8)], copyX, screenX, screenY);
	VBALuaClearGui();
}

/////////////////////////////////////////////////////////////////////////////
// VBA

BEGIN_MESSAGE_MAP(VBA, CWinApp)
//{{AFX_MSG_MAP(VBA)
// NOTE - the ClassWizard will add and remove mapping macros here.
//    DO NOT EDIT what you see in these blocks of generated code!
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// VBA construction

VBA::VBA()
{
	mode320Available		= false;
	mode640Available		= false;
	mode800Available		= false;
	windowPositionX			= 0;
	windowPositionY			= 0;
	filterFunction			= NULL;
	ifbFunction				= NULL;
	ifbType					= 0;
	filterType				= 0;
	filterWidth				= 0;
	filterHeight			= 0;
	fsWidth					= 0;
	fsHeight				= 0;
	fsColorDepth			= 0;
	fsForceChange			= false;
	surfaceSizeX			= 0;
	surfaceSizeY			= 0;
	sizeX					= 0;
	sizeY					= 0;
	videoOption				= 0;
	fullScreenStretch		= false;
	disableStatusMessage	= false;
	showSpeed				= 1;
	showSpeedTransparent	= true;
	showRenderedFrames		= 0;
	for (int j = 0; j < SCREEN_MESSAGE_SLOTS; j++)
	{
		screenMessage[j]			= false;
		screenMessageTime[j]		= 0;
		screenMessageDuration[j]	= 0;
	}
	menuToggle = true;
	display = NULL;
	menu = NULL;
	popup = NULL;
	cartridgeType = 0;
	soundInitialized = false;
	useBiosFile = false;
	skipBiosFile = false;
	active = true;
	paused = false;
	recentFreeze = false;
	autoSaveLoadCheatList		= false;
	pauseDuringCheatSearch		= false;
	modelessCheatDialogIsOpen	= false;
//	winout						= NULL;
//	removeIntros				= false;
	autoIPS						= true;
	winGbBorderOn				= 0;
	hideMovieBorder				= false;
	winFlashSize				= 0x10000;
	winRtcEnable				= false;
	winSaveType					= 0;
	rewindMemory				= NULL;
	frameSearchMemory			= NULL;
	rewindPos					= 0;
	rewindTopPos				= 0;
	rewindCounter				= 0;
	rewindCount					= 0;
	rewindSaveNeeded			= false;
	rewindTimer					= 0;
	captureFormat				= 0;
	tripleBuffering				= true;
	autoHideMenu				= false;
	throttle					= 100;
	throttleLastTime			= 0;
///  autoFrameSkipLastTime		= 0;
///  autoFrameSkip				= false;
	vsync						= false;
	changingVideoSize			= false;
	pVideoDriverGUID			= NULL;
	renderMethod				= DIRECT_DRAW;
	iconic						= false;
	ddrawEmulationOnly			= false;
	ddrawUsingEmulationOnly		= false;
	ddrawDebug				= false;
	ddrawUseVideoMemory		= false;
	d3dFilter				= 0;
	glFilter				= 0;
	glType					= 0;
	regEnabled				= false;
	pauseWhenInactive		= true;
	filenamePreference		= true;
	frameCounter			= false;
	lagCounter				= false;
	inputDisplay			= false;
	movieReadOnly			= true;
	speedupToggle			= false;
	useOldSync				= false;
	useOldGBTiming			= false;
	allowLeftRight			= false;
	muteFrameAdvance		= false;
	frameAdvanceMuteNow		= false;
	winGbPrinterEnabled		= false;
	threadPriority			= 2;
	disableMMX				= false;
	languageOption			= 0;
	languageModule			= NULL;
	languageName			= "";
	renderedFrames			= 0;
	input					= NULL;
	joypadDefault			= 0;
	autoFire				= 0;
	autoFire2				= 0;
	autoHold				= 0;
	autoFireToggle			= false;
	winPauseNextFrame		= false;
	soundRecording			= false;
	soundRecorder			= NULL;
	sound					= NULL;
	aviRecording			= false;
	aviRecorder				= NULL;
	painting				= false;
	sensorX					= 2047;
	sensorY					= 2047;
	mouseCounter			= 0;
	wasPaused				= false;
	fsMaxScale				= 0;
	romSize					= 0;
	autoLoadMostRecent		= false;
	loadMakesRecent			= false;
	loadMakesCurrent		= false;
	saveMakesCurrent		= false;
	currentSlot				= 0;
	showSlotTime			= false;
	frameSearchLoadValid	= false;
	frameSearching			= false;
	frameSearchSkipping		= false;
	nvVideoLog				= false;
	nvAudioLog				= false;
	LoggingEnabled			= 0;
///  FPS = 60;

	updateCount = 0;

	systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

	ZeroMemory(&emulator, sizeof(emulator));

	hAccel = NULL;

	for (int i = 0; i < 24;)
	{
		systemGbPalette[i++] = (0x1f) | (0x1f << 5) | (0x1f << 10);
		systemGbPalette[i++] = (0x15) | (0x15 << 5) | (0x15 << 10);
		systemGbPalette[i++] = (0x0c) | (0x0c << 5) | (0x0c << 10);
		systemGbPalette[i++] = 0;
	}

	VBAMovieInit();
}

VBA::~VBA()
{
	InterframeCleanup();

	saveSettings();
/*
   if(moviePlaying) {
    if(movieFile != NULL) {
      fclose(movieFile);
      movieFile = NULL;
    }
    moviePlaying = false;
    movieLastJoypad = 0;
   }
 */
	if (VBAMovieActive())
		VBAMovieStop(true);
/*
   if(movieRecording) {
    if(movieFile != NULL) {
      // record the last joypad change so that the correct time can be
      // recorded
      fwrite(&movieFrame, 1, sizeof(int32), movieFile);
      fwrite(&movieLastJoypad, 1, sizeof(u32), movieFile);
      fclose(movieFile);
      movieFile = NULL;
    }
    movieRecording = false;
    moviePlaying = false;
    movieLastJoypad = 0;
   }
 */
	if (aviRecorder)
	{
		delete aviRecorder;
		aviRecorder  = NULL;
		aviRecording = false;
	}

	if (soundRecorder)
	{
		delete soundRecorder;
		soundRecorder = NULL;
	}
	soundRecording = false;
	soundPause();
	soundShutdown();

	if (gbRom != NULL || rom != NULL)
	{
		if (autoSaveLoadCheatList)
			((MainWnd *)m_pMainWnd)->winSaveCheatListDefault();
		((MainWnd *)m_pMainWnd)->writeBatteryFile();
		cheatSearchCleanup(&cheatSearchData);
		emulator.emuCleanUp();

		if (VBAMovieActive())
			VBAMovieStop(false);
	}

	if (input)
		delete input;

	shutdownDisplay();

	if (rewindMemory)
		free(rewindMemory);

	if (frameSearchMemory)
		free(frameSearchMemory);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only VBA object

VBA theApp;
#include <afxdisp.h>
/////////////////////////////////////////////////////////////////////////////
// VBA initialization

// code from SDL_main.c for Windows
/* Parse a command line buffer into arguments */
static int parseCommandLine(char *cmdline, char **argv)
{
	char *bufp;
	int   argc;

	argc = 0;
	for (bufp = cmdline; *bufp;)
	{
		/* Skip leading whitespace */
		while (isspace(*bufp))
		{
			++bufp;
		}
		/* Skip over argument */
		if (*bufp == '"')
		{
			++bufp;
			if (*bufp)
			{
				if (argv)
				{
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while (*bufp && (*bufp != '"'))
			{
				++bufp;
			}
		}
		else
		{
			if (*bufp)
			{
				if (argv)
				{
					argv[argc] = bufp;
				}
				++argc;
			}
			/* Skip over word */
			while (*bufp && !isspace(*bufp))
			{
				++bufp;
			}
		}
		if (*bufp)
		{
			if (argv)
			{
				*bufp = '\0';
			}
			++bufp;
		}
	}
	if (argv)
	{
		argv[argc] = NULL;
	}
	return(argc);
}

static void CorrectPath(CString & path)
{
	CString tempStr;
	FILE *  tempFile;

	if (tempFile = fopen(path, "rb"))
	{
		fclose(tempFile);
	}
	else
	{
		for (int i = 0; i < 11; i++)
		{
			switch (i)
			{
			case 0:
			{
				char curDir [_MAX_PATH];
				GetCurrentDirectory(_MAX_PATH, curDir);
				curDir[_MAX_PATH-1] = '\0';
				tempStr = curDir;
			}   break;
			case 1:
				tempStr = regQueryStringValue(IDS_ROM_DIR, "."); break;
			case 2:
				tempStr = regQueryStringValue(IDS_GBXROM_DIR, "."); break;
			case 3:
				tempStr = regQueryStringValue(IDS_BATTERY_DIR, "."); break;
			case 4:
				tempStr = regQueryStringValue(IDS_SAVE_DIR, "."); break;
			case 5:
				tempStr = regQueryStringValue(IDS_MOVIE_DIR, "."); break;
			case 6:
				tempStr = regQueryStringValue(IDS_CHEAT_DIR, "."); break;
			case 7:
				tempStr = regQueryStringValue(IDS_IPS_DIR, "."); break;
			case 8:
				tempStr = regQueryStringValue(IDS_AVI_DIR, "."); break;
			case 9:
				tempStr = regQueryStringValue(IDS_WAV_DIR, "."); break;
			case 10:
				tempStr = regQueryStringValue(IDS_CAPTURE_DIR, "."); break;
			/*
			                // what do these do?
			                case 11: tempStr = "C:";
			                case 12: {
			                    tempStr = regQueryStringValue(IDS_ROM_DIR,".");
			                    char * slash = (char*)strrchr(tempStr, '\\'); // should use member func intstead
			                    if(slash)
			                        slash[0] = '\0';
			                }	break;
			 */
			default:
				break;
			}
			tempStr += "\\";
			tempStr += path;

			if (tempFile = fopen(tempStr, "rb"))
			{
				fclose(tempFile);
				path = tempStr;
				break;
			}
		}
	}
}

static void CorrectPath(char *path)
{
	CString pathCStr = path;
	CorrectPath(pathCStr);
	strcpy(path, pathCStr);
}

void debugSystemScreenMessage1(const char *msg)
{
	systemScreenMessage(msg, 3, 60);
}

void debugSystemScreenMessage2(const char *msg)
{
	systemScreenMessage(msg, 4, 60);
}

BOOL VBA::InitInstance()
{
	AfxEnableControlContainer();
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

//#ifdef _AFXDLL
//  Enable3dControls();      // Call this when using MFC in a shared DLL
//#else
//  Enable3dControlsStatic();  // Call this when linking to MFC statically
//#endif

	SetRegistryKey(_T("VBA"));

	remoteSetProtocol(0);

	systemVerbose = GetPrivateProfileInt("config",
	                                     "verbose",
	                                     0,
	                                     "VBA.ini");

	systemDebug = GetPrivateProfileInt("config",
	                                   "debug",
	                                   0,
	                                   "VBA.ini");
	ddrawDebug = GetPrivateProfileInt("config",
	                                  "ddrawDebug",
	                                  0,
	                                  "VBA.ini") ? true : false;

	wndClass = AfxRegisterWndClass(0, LoadCursor(IDC_ARROW), (HBRUSH)GetStockObject(BLACK_BRUSH), LoadIcon(IDI_ICON));

	char winBuffer[2048];

	GetModuleFileName(NULL, winBuffer, 2048);
	char *p = strrchr(winBuffer, '\\');
	if (p)
		*p = 0;

	regInit(winBuffer);

	loadSettings();

	if (!initInput())
		return FALSE;

	if (!initDisplay())
	{
		if (videoOption >= VIDEO_320x240)
		{
			regSetDwordValue("video", VIDEO_1X);
			if (pVideoDriverGUID)
				regSetDwordValue("defaultVideoDriver", TRUE);
		}
		return FALSE;
	}

	hAccel = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR));

	winAccelMgr.Connect((MainWnd *)m_pMainWnd);

	extern void winAccelAddCommands(CAcceleratorManager&);

	winAccelAddCommands(winAccelMgr);

	winAccelMgr.CreateDefaultTable();

	winAccelMgr.Load();

	winAccelMgr.UpdateWndTable();

	winAccelMgr.UpdateMenu(menu);

	if (m_lpCmdLine[0])
	{
		int    argc = parseCommandLine(m_lpCmdLine, NULL);
		char **argv = (char * *)malloc((argc+1)*sizeof(char *));
		parseCommandLine(m_lpCmdLine, argv);

		bool gotFlag = false, enoughArgs = false;
		for (int i = 0; i < argc; i++)
		{
			if (argv[i][0] == '-' || gotFlag)
			{
				if (!gotFlag)
					loadSettings();
				gotFlag = true;
				if (_stricmp(argv[i], "-rom") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					szFile = argv[++i];
					CorrectPath(szFile);
					filename = szFile;
				}
				else if (_stricmp(argv[i], "-bios") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					biosFileName = argv[++i];
					CorrectPath(biosFileName);
					extern void loadBIOS();
					loadBIOS();
				}
				else if (_stricmp(argv[i], "-frameskip") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					frameSkip = atoi(argv[++i]);
					if (frameSkip < 0)
						frameSkip = 0;
					if (frameSkip > 9)
						frameSkip = 9;
					gbFrameSkip = frameSkip;
				}
				else if (_stricmp(argv[i], "-throttle") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					throttle = atoi(argv[++i]);
					if (throttle < 5)
						throttle = 5;
					if (throttle > 1000)
						throttle = 1000;
				}
				else if (_stricmp(argv[i], "-throttleKeepPitch") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					accuratePitchThrottle = atoi(argv[++i]) != 0;
				}
				else if (_stricmp(argv[i], "-synchronize") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					synchronize = atoi(argv[++i]) != 0;
				}
				else if (_stricmp(argv[i], "-hideborder") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					hideMovieBorder = atoi(argv[++i]) != 0;
				}
				else if (_stricmp(argv[i], "-play") == 0)
				{
					playMovieFile = true;
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					strcpy(movieFileToPlay, argv[++i]);
					CorrectPath(movieFileToPlay);
					if (i+1 >= argc || argv[i+1][0] == '-') {--i; goto invalidArgument;}
					playMovieFileReadOnly = atoi(argv[++i]) != 0;
				}
				else if (_stricmp(argv[i], "-videoLog") == 0)
				{
					nvVideoLog     = true;
					nvAudioLog     = true;
					LoggingEnabled = 2;
					if (i+1 >= argc || argv[i+1][0] == '-') {}
					else
						NESVideoSetVideoCmd(argv[++i]);
				}
				else if (_stricmp(argv[i], "-logDebug") == 0)
				{
					NESVideoEnableDebugging(debugSystemScreenMessage1, debugSystemScreenMessage2);
				}
				else if (_stricmp(argv[i], "-logToFile") == 0)
				{
					NESVideoSetFileFuncs(fopen, fclose);
				}
				else if (_stricmp(argv[i], "-outputWAV") == 0)
				{
					outputWavFile = true;
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					strcpy(wavFileToOutput, argv[++i]);
				}
				else if (_stricmp(argv[i], "-outputAVI") == 0)
				{
					outputAVIFile = true;
				}
				else if (_stricmp(argv[i], "-quitAfter") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					quitAfterTime = atoi(argv[++i]);
				}
				else if (_stricmp(argv[i], "-pauseAt") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					pauseAfterTime = atoi(argv[++i]);
				}
				else if (_stricmp(argv[i], "-videoScale") == 0)
				{
					if (i+1 >= argc || argv[i+1][0] == '-')
						goto invalidArgument;
					int size = atoi(argv[++i]);
					if (size < 1)
						size = 1;
					if (size > 4)
						size = 4;
					switch (size)
					{
					case 1:
						videoOption = VIDEO_1X; break;
					case 2:
						videoOption = VIDEO_2X; break;
					case 3:
						videoOption = VIDEO_3X; break;
					case 4:
						videoOption = VIDEO_4X; break;
					}
				}
				else if (_stricmp(argv[i], "-hideMenu") == 0)
				{
					flagHideMenu = true;
				}
				else
				{
					enoughArgs = true;
invalidArgument:
					char str [1024];
					strcpy(str, "");
					if (_stricmp(argv[i], "-h") != 0)
						if (enoughArgs)
							sprintf(str, "Invalid commandline argument %d: %s\n", i, argv[i]);
						else
							sprintf(str, "Not enough arguments for arg %d: %s\n", i, argv[i]);
					strcat(str, "Valid commands:\n"
					            "-h \t\t\t displays this help\n"
					            "-rom filename \t\t opens the given ROM\n"
					            "-bios filename \t\t use the given GBA BIOS\n"
					            "-play filename val \t\t plays the given VBM movie (val: 1 = read-only, 0 = editable)\n"
					            "-outputWAV filename \t outputs WAV audio to the given file\n"
					            "-outputAVI \t\t outputs an AVI (you are prompted for location and codec)\n"
					            "-frameskip val \t\t sets the frameskip amount to the given value\n"
					            "-synchronize val \t\t limits running speed to sound playing speed, (0 = off, 1 = on)\n"
					            "-throttle val \t\t sets the throttle speed to the given percentage\n"
					            "-hideborder val \t\t hides SGB border, if any (0 = show, 1 = hide)\n"
					            "-throttleKeepPitch val \t if throttle and synch, don't change sound freq (0 = off, 1 = on)\n"
					            "-quitAfter val \t\t close program when frame counter == val\n"
					            "-pauseAt val \t\t pause (movie) once when frame counter == val\n"
					            "-videoScale val \t\t sets the video size (val = 1 for 1X, 2 for 2X, 3 for 3X, or 4 for 4X)\n"
					            "-hideMenu \t\t hides the menu until program exit\n"
					            "\n"
					            "-videoLog args \t does (nesvideos) video+audio logging with the given arguments\n"
					            "-logToFile \t tells logging to use fopen/fclose of args, if logging is enabled\n"
					            "-logDebug  \t tells logging to output debug info to screen, if logging is enabled\n"
					       );
					theApp.winCheckFullscreen();
					AfxGetApp()->m_pMainWnd->MessageBox(str, "Commandline Help", MB_OK|MB_ICONINFORMATION);
					exit(0);
				}
			}
			else
			{
				// assume anything else is a ROM, for backward compatibility
				szFile   = argv[i++];
				filename = szFile;
				loadSettings();
			}
		}
		int index = filename.ReverseFind('.');

		if (index != -1)
			filename = filename.Left(index);

		if (szFile.GetLength() > 0)
		{
			if (((MainWnd *)m_pMainWnd)->FileRun())
				emulating = true;
			else
				emulating = false;
		}
		free(argv);
	}

	return TRUE;
}

void VBA::adjustDestRect()
{
	POINT point;

	point.x = 0;
	point.y = 0;

	m_pMainWnd->ClientToScreen(&point);
	dest.top  = point.y;
	dest.left = point.x;

	point.x = surfaceSizeX;
	point.y = surfaceSizeY;

	m_pMainWnd->ClientToScreen(&point);
	dest.bottom = point.y;
	dest.right  = point.x;

	// make sure that dest rect lies in the monitor
	if (videoOption >= VIDEO_320x240)
	{
		dest.top    -= windowPositionY;
		dest.left   -= windowPositionX;
		dest.bottom -= windowPositionY;
		dest.right  -= windowPositionX;
	}

	int menuSkip = 0;

	if (videoOption >= VIDEO_320x240 && menuToggle)
	{
		int m = GetSystemMetrics(SM_CYMENU);
		menuSkip     = m;
		dest.bottom -= m;
	}

	if (videoOption > VIDEO_4X)
	{
		int top  = (fsHeight - surfaceSizeY) / 2;
		int left = (fsWidth - surfaceSizeX) / 2;
		dest.top    += top;
		dest.bottom += top;
		dest.left   += left;
		dest.right  += left;
		if (fullScreenStretch)
		{
			dest.top    = 0+menuSkip;
			dest.left   = 0;
			dest.right  = fsWidth;
			dest.bottom = fsHeight;
		}
	}
}

void VBA::updateIFB()
{
	if (systemColorDepth == 16)
	{
		switch (ifbType)
		{
		case 0:
		default:
			ifbFunction = NULL;
			break;
		case 1:
			ifbFunction = MotionBlurIB;
			break;
		case 2:
			ifbFunction = SmartIB;
			break;
		}
	}
	else if (systemColorDepth == 32)
	{
		switch (ifbType)
		{
		case 0:
		default:
			ifbFunction = NULL;
			break;
		case 1:
			ifbFunction = MotionBlurIB32;
			break;
		case 2:
			ifbFunction = SmartIB32;
			break;
		}
	}
	else
		ifbFunction = NULL;
}

void VBA::updateFilter()
{
	filterWidth  = sizeX;
	filterHeight = sizeY;

	if (systemColorDepth == 16 && (videoOption > VIDEO_1X &&
	                               videoOption != VIDEO_320x240))
	{
		switch (filterType)
		{
		default:
		case 0:
			filterFunction = NULL;
			break;
		case 1:
			filterFunction = ScanlinesTV;
			break;
		case 2:
			filterFunction = _2xSaI;
			break;
		case 3:
			filterFunction = Super2xSaI;
			break;
		case 4:
			filterFunction = SuperEagle;
			break;
		case 5:
			filterFunction = Pixelate;
			break;
		case 6:
			filterFunction = MotionBlur;
			break;
		case 7:
			filterFunction = AdMame2x;
			break;
		case 8:
			filterFunction = Simple2x;
			break;
		case 9:
			filterFunction = Bilinear;
			break;
		case 10:
			filterFunction = BilinearPlus;
			break;
		case 11:
			filterFunction = Scanlines;
			break;
		case 12:
			filterFunction = hq2xS;
			break;
		case 13:
			filterFunction = hq2x;
			break;
		case 14:
			filterFunction = lq2x;
			break;
		case 15:
			filterFunction = hq3xS;
			break;
		case 16:
			filterFunction = hq3x;
			break;
		}
		switch (filterType)
		{
		case 0: // normal -> 1x texture
			rect.right  = sizeX;
			rect.bottom = sizeY;
			break;
		default: // other -> 2x texture
			rect.right  = sizeX*2;
			rect.bottom = sizeY*2;
			memset(delta, 255, sizeof(delta));
			break;
		case 15: // hq3x -> 3x texture
		case 16:
			rect.right  = sizeX*3;
			rect.bottom = sizeY*3;
			memset(delta, 255, sizeof(delta));
			break;
		}
	}
	else
	{
		if (systemColorDepth == 32 && videoOption > VIDEO_1X &&
		    videoOption != VIDEO_320x240)
		{
			switch (filterType)
			{
			default:
			case 0:
				filterFunction = NULL;
				break;
			case 1:
				filterFunction = ScanlinesTV32;
				break;
			case 2:
				filterFunction = _2xSaI32;
				break;
			case 3:
				filterFunction = Super2xSaI32;
				break;
			case 4:
				filterFunction = SuperEagle32;
				break;
			case 5:
				filterFunction = Pixelate32;
				break;
			case 6:
				filterFunction = MotionBlur32;
				break;
			case 7:
				filterFunction = AdMame2x32;
				break;
			case 8:
				filterFunction = Simple2x32;
				break;
			case 9:
				filterFunction = Bilinear32;
				break;
			case 10:
				filterFunction = BilinearPlus32;
				break;
			case 11:
				filterFunction = Scanlines32;
				break;
			case 12:
				filterFunction = hq2xS32;
				break;
			case 13:
				filterFunction = hq2x32;
				break;
			case 14:
				filterFunction = lq2x32;
				break;
			case 15:
				filterFunction = hq3xS32;
				break;
			case 16:
				filterFunction = hq3x32;
				break;
			}
			switch (filterType)
			{
			case 0: // normal -> 1x texture
				rect.right  = sizeX;
				rect.bottom = sizeY;
				break;
			default: // other -> 2x texture
				rect.right  = sizeX*2;
				rect.bottom = sizeY*2;
				memset(delta, 255, sizeof(delta));
				break;
			case 15: // hq3x -> 3x texture
			case 16:
				rect.right  = sizeX*3;
				rect.bottom = sizeY*3;
				memset(delta, 255, sizeof(delta));
				break;
			}
		}
		else
			filterFunction = NULL;
	}

	if (display)
		display->changeRenderSize(rect.right, rect.bottom);
}

void VBA::updateMenuBar()
{
	if (flagHideMenu)
		return;

	if (menu != NULL)
	{
		if (m_pMainWnd)
			m_pMainWnd->SetMenu(NULL);
		m_menu.Detach();
		DestroyMenu(menu);
	}

	if (popup != NULL)
	{
		// force popup recreation if language changed
		DestroyMenu(popup);
		popup = NULL;
	}

	m_menu.Attach(winResLoadMenu(MAKEINTRESOURCE(IDR_MENU)));
	menu = (HMENU)m_menu;

	if (m_pMainWnd)
		m_pMainWnd->SetMenu(&m_menu);
}

void winlog(const char *msg, ...)
{
	CString buffer;
	va_list valist;

	va_start(valist, msg);
	buffer.FormatV(msg, valist);

	FILE *winout = fopen("vba-trace.log", "w");

	fputs(buffer, winout);

	fclose(winout);

	va_end(valist);
}

void log(const char *msg, ...)
{
	CString buffer;
	va_list valist;

	va_start(valist, msg);
	buffer.FormatV(msg, valist);

	toolsLog(buffer);

	va_end(valist);
}

bool systemReadJoypads()
{
	if (theApp.input)
		return theApp.input->readDevices();
	return false;
}

u32 systemReadJoypad(int which, bool sensor)
{
	if (theApp.input /* || VBALuaUsingJoypad(which)*/)
		return theApp.input->readDevice(which, sensor, false);
	return 0;
}

extern bool vbaShuttingDown;
extern long linearSoundFrameCount;
long        linearFrameCount = 0;

void systemDrawScreen()
{
	if (vbaShuttingDown)
		return;

	if (!theApp.painting)
	{
		linearFrameCount++;
		if (!theApp.sound)
		{
			if (linearFrameCount > 10000)
				linearFrameCount -= 10000;
			linearSoundFrameCount = linearFrameCount;
		}
	}
	else
	{
		static bool updatingFrameCount = false;
		if (!updatingFrameCount) // recursion-preventing paranoia
		{
			updatingFrameCount = true;
			VBAUpdateFrameCountDisplay();
			updatingFrameCount = false;
		}
	}

	if (theApp.display == NULL)
		return;

	theApp.renderedFrames++;

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

	// This piece was moved from src/win32/DirectDraw.cpp.
	// However, it is redundant from those in such as Direct3D.cpp.
	// So, tentatively reverted to v19.3
	//
	// theApp.display->render() is called after video logging,
	// text messages cannot be recorded to the video, of course.
	//
	// "in-game" text rendering
	if (textMethod == 0 && !theApp.painting) // transparent text shouldn't be painted twice
	{
		int copyX = 240, copyY = 160;
		int pitch;
		if (theApp.cartridgeType == 1)
			if (gbBorderOn)
				copyX = 256, copyY = 224;
			else
				copyX = 160, copyY = 144;
		pitch = copyX*(systemColorDepth/8)+(systemColorDepth == 24 ? 0 : 4);

		DrawLuaGui();
		DrawTextMessages((u8 *)pix, pitch, 0, copyY);
	}

	if (!theApp.painting)
	{
		int width  = 240;
		int height = 160;
		switch (theApp.cartridgeType)
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
		linearFrameCount--;
		do
		{
			linearFrameCount++;

			if (theApp.aviRecording && (!theApp.altAviRecordMethod || (theApp.altAviRecordMethod && !firstFrameLogged)))
			{
				// usually aviRecorder is created when vba starts avi recording, though
				if (theApp.aviRecorder == NULL)
				{
					theApp.aviRecorder = new AVIWrite();

					theApp.aviRecorder->SetFPS(60);

					BITMAPINFOHEADER bi;
					memset(&bi, 0, sizeof(bi));
					bi.biSize      = 0x28;
					bi.biPlanes    = 1;
					bi.biBitCount  = 24;
					bi.biWidth     = width;
					bi.biHeight    = height;
					bi.biSizeImage = 3*width*height;
					theApp.aviRecorder->SetVideoFormat(&bi);
					if (!theApp.aviRecorder->Open(theApp.aviRecordName))
					{
						delete theApp.aviRecorder;
						theApp.aviRecorder  = NULL;
						theApp.aviRecording = false;
					}
				}

				if (theApp.aviRecorder != NULL)
				{
					assert(
					    width <= BMP_BUFFER_MAX_WIDTH && height <= BMP_BUFFER_MAX_HEIGHT && systemColorDepth <=
					    BMP_BUFFER_MAX_DEPTH*8);
					utilWriteBMP(bmpBuffer, width, height, systemColorDepth, pix);
					theApp.aviRecorder->AddFrame(bmpBuffer);
				}
			}

			if (theApp.nvVideoLog)
			{
				// convert from whatever bit depth to 16-bit, while stripping away extra pixels
				assert(width <= BMP_BUFFER_MAX_WIDTH && height <= BMP_BUFFER_MAX_HEIGHT && 16 <= BMP_BUFFER_MAX_DEPTH*8);
				utilWriteBMP(bmpBuffer, width, -height, 16, pix);
				NESVideoLoggingVideo((u8 *)bmpBuffer, width, height, 0x1000000 * 60);
			}

			firstFrameLogged = true;
		}
		while (linearFrameCount < linearSoundFrameCount); // compensate for frames lost due to frame skip being nonzero, etc.
	}

	// draw Lua graphics in-game but after video logging
	if (textMethod != 0 && !theApp.painting) // transparent text shouldn't be painted twice
	{
		DrawLuaGui();
	}

	if (theApp.ifbFunction)
	{
		if (systemColorDepth == 16)
			theApp.ifbFunction(pix+theApp.filterWidth*2+4, theApp.filterWidth*2+4,
			                   theApp.filterWidth, theApp.filterHeight);
		else
			theApp.ifbFunction(pix+theApp.filterWidth*4+4, theApp.filterWidth*4+4,
			                   theApp.filterWidth, theApp.filterHeight);
	}

	theApp.display->render();
}

void systemScreenCapture(int captureNumber)
{
	if (theApp.m_pMainWnd)
		((MainWnd *)theApp.m_pMainWnd)->screenCapture(captureNumber);
}

u32 systemGetClock()
{
	return timeGetTime();
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

	AfxGetApp()->m_pMainWnd->MessageBox(buffer, winResLoadString(IDS_ERROR), MB_OK|MB_ICONERROR);

	va_end(valist);
}

void systemSetTitle(const char *title)
{
	if (theApp.m_pMainWnd != NULL)
	{
		AfxGetApp()->m_pMainWnd->SetWindowText(title);
	}
}

void systemShowSpeed(int speed)
{
	systemSpeed = speed;
	theApp.showRenderedFrames = theApp.renderedFrames;
	theApp.renderedFrames     = 0;
	if (theApp.videoOption <= VIDEO_4X && theApp.showSpeed)
	{
		CString buffer;
		if (theApp.showSpeed == 1)
			buffer.Format(MAINWND_TITLE_STRING "-%3d%%", systemSpeed);
		else
			buffer.Format(MAINWND_TITLE_STRING "-%3d%%(%d, %d fps)", systemSpeed,
			              systemFrameSkip,
			              theApp.showRenderedFrames);

		systemSetTitle(buffer);
	}
}

void systemFrame(int rate)
{
	if (theApp.altAviRecordMethod && theApp.aviRecording)
	{
		if (theApp.aviRecorder)
		{
			if (!theApp.aviRecorder->IsSoundAdded())
			{
				WAVEFORMATEX wfx;
				memset(&wfx, 0, sizeof(wfx));
				wfx.wFormatTag      = WAVE_FORMAT_PCM;
				wfx.nChannels       = 2;
				wfx.nSamplesPerSec  = 44100 / soundQuality;
				wfx.wBitsPerSample  = 16;
				wfx.nBlockAlign     = (wfx.wBitsPerSample/8) * wfx.nChannels;
				wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
				wfx.cbSize = 0;
				theApp.aviRecorder->SetSoundFormat(&wfx);
			}
			theApp.aviRecorder->AddSound((u8 *)soundFrameSound, soundFrameSoundWritten * 2);
		}
	}

	// stupid updates :(
	struct EmulatedSystem &emu = (theApp.cartridgeType == 0) ? GBASystem : GBSystem;
	theApp.emulator.frameCount = emu.frameCount;
	theApp.emulator.lagCount   = emu.lagCount;
	theApp.emulator.lagged     = emu.lagged;
	theApp.emulator.laggedLast = emu.laggedLast;

	if (VBALuaRunning())
	{
		VBALuaFrameBoundary();
	}

	if (quitAfterTime >= 0 && theApp.emulator.frameCount == quitAfterTime)
	{
		VBAMovieStop(true);
		AfxPostQuitMessage(0);
	}

	u32 time = systemGetClock();

	// change the sound speed, or set it to normal - must always do this or it won't get reset after a change, but that's OK
	// because it's inexpensive
	if (theApp.sound)
		theApp.sound->setSpeed(
		    theApp.throttle != 100 && !speedup && !theApp.winPauseNextFrame && synchronize && !theApp.accuratePitchThrottle &&
		    !theApp.useOldSync ? (float)theApp.throttle / 100.0f : 1.0f);

//  bool deadSound = !theApp.sound->isPlaying();

	// if a throttle speed is set and we're not allowed to change the sound frequency to achieve it,
	// sleep for a certain amount each time we get here to approximate the necessary slowdown
	if (((theApp.accuratePitchThrottle || !theApp.sound || !synchronize)
	     && (theApp.throttle != 100 || !synchronize))
	    || theApp.throttle < 6 && !theApp.winPauseNextFrame)
	{
		/// FIXME: this is still a horrible way of achieving a certain frame time
		///        (look at what Snes9x does - it's complicated but much much better)

		static float sleepAmt = 0.0f; // variable to smooth out the sleeping amount so it doesn't oscillate so fast
//	  if(!theApp.wasPaused) {
		if (!speedup)
		{
			u32 diff = time - theApp.throttleLastTime;
			if (theApp.wasPaused)
				diff = 0;

			int target = (100000/(rate*theApp.throttle));
			int d      = (target - diff);

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

	if (theApp.emulator.frameCount % 10 == 0)
	{
		if (theApp.rewindMemory)
		{
			if (++theApp.rewindCounter >= (theApp.rewindTimer))
			{
				theApp.rewindSaveNeeded = true;
				theApp.rewindCounter    = 0;
			}
		}
		if (systemSaveUpdateCounter)
		{
			if (--systemSaveUpdateCounter <= SYSTEM_SAVE_NOT_UPDATED)
			{
				((MainWnd *)theApp.m_pMainWnd)->writeBatteryFile();
				systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
			}
		}
	}

	theApp.wasPaused = false;
///  theApp.autoFrameSkipLastTime = time;
}

void systemScreenMessage(const char *msg, int slot, int duration, const char *colorList)
{
	if (slot < 0 || slot > SCREEN_MESSAGE_SLOTS)
		return;

	if (slot == 0 && (theApp.paused || (theApp.frameSearching)))
		theApp.m_pMainWnd->PostMessage(WM_PAINT, NULL, NULL); // update the display when a main-slot message appears while the
                                                              // game is paused

	theApp.screenMessage[slot] = true;
	theApp.screenMessageTime[slot]        = GetTickCount();
	theApp.screenMessageDuration[slot]    = duration;
	theApp.screenMessageBuffer[slot]      = msg;
	theApp.screenMessageColorBuffer[slot] = colorList ? colorList : "";

	if (theApp.screenMessageBuffer[slot].GetLength() > 40)
		theApp.screenMessageBuffer[slot] = theApp.screenMessageBuffer[slot].Left(40);
}

int systemGetSensorX()
{
	return theApp.sensorX;
}

int systemGetSensorY()
{
	return theApp.sensorY;
}

bool systemSoundInit()
{
	if (theApp.sound)
		delete theApp.sound;

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
}

void systemSoundReset()
{
	if (theApp.sound)
		theApp.sound->reset();
}

void systemSoundResume()
{
	if (theApp.sound)
		theApp.sound->resume();
}

void systemWriteDataToSoundBuffer()
{
	if (theApp.sound)
		theApp.sound->write();
}

bool systemCanChangeSoundQuality()
{
	return true;
}

bool systemPauseOnFrame()
{
	if (theApp.winPauseNextFrame)
	{
		theApp.paused = true;
		theApp.winPauseNextFrame = false;
		return true;
	}
	return false;
}

void systemGbBorderOn()
{
	if (emulating && theApp.cartridgeType == 1 && gbBorderOn)
	{
		theApp.updateWindowSize(theApp.videoOption);
	}
}

void VBA::saveRewindStateIfNecessary()
{
	if (rewindSaveNeeded && rewindMemory && emulator.emuWriteMemState)
	{
		rewindCount++;
		if (rewindCount > rewindSlots)
			rewindCount = rewindSlots;
		assert(rewindPos >= 0 && rewindPos < rewindSlots);
		if (emulator.emuWriteMemState(&rewindMemory[rewindPos*REWIND_SIZE], REWIND_SIZE))
		{
			rewindPos = ++rewindPos % rewindSlots;
			assert(rewindPos >= 0 && rewindPos < rewindSlots);
			if (rewindCount == rewindSlots)
				rewindTopPos = ++rewindTopPos % rewindSlots;
		}
	}

	// also update/cache some frame search stuff
	if (theApp.frameSearching)
	{
		extern SMovie Movie;
		int curFrame = (Movie.state == MOVIE_STATE_NONE) ? theApp.emulator.frameCount : Movie.currentFrame;
		int endFrame = theApp.frameSearchStart + theApp.frameSearchLength;
		theApp.frameSearchSkipping  = (curFrame < endFrame);
		theApp.frameSearchFirstStep = false;

		if (curFrame == endFrame)
		{
			// cache intermediate state to speed up searching forward
			theApp.emulator.emuWriteMemState(&theApp.frameSearchMemory[REWIND_SIZE*1], REWIND_SIZE);
		}

		if (curFrame == endFrame + 1)
		{
			theApp.emulator.emuWriteMemState(&theApp.frameSearchMemory[REWIND_SIZE*2], REWIND_SIZE);
			theApp.frameSearchLoadValid = true;
		}
	}
	else
	{
		theApp.frameSearchFirstStep = false;

		assert(!theApp.frameSearchSkipping);
		// just in case
		theApp.frameSearchSkipping = false;
	}
}

BOOL VBA::OnIdle(LONG lCount)
{
	if (emulating && debugger)
	{
		MSG msg;
		remoteStubMain();
		if (debugger)
			return TRUE; // continue loop
		return !::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE);
	}
	else if (emulating && active && !paused)
	{
///    for(int i = 0; i < 2; i++)
		{
			emulator.emuMain(emulator.emuCount);

			// save the state for rewinding, if necessary
			saveRewindStateIfNecessary();

			rewindSaveNeeded = false;
		}

		if (mouseCounter)
		{
			if (--mouseCounter == 0)
			{
				SetCursor(NULL);
			}
		}
		return TRUE;
	}

	return FALSE;

	//  return CWinApp::OnIdle(lCount);
}

void VBA::addRecentFile(CString file)
{
	// Do not change recent list if frozen
	if (recentFreeze)
		return;
	int i = 0;
	for (i = 0; i < 10; i++)
	{
		if (recentFiles[i].GetLength() == 0)
			break;

		if (recentFiles[i].Compare(file) == 0)
		{
			if (i == 0)
				return;
			CString p = recentFiles[i];
			for (int j = i; j > 0; j--)
			{
				recentFiles[j] = recentFiles[j-1];
			}
			recentFiles[0] = p;
			return;
		}
	}
	int num = 0;
	for (i = 0; i < 10; i++)
	{
		if (recentFiles[i].GetLength() != 0)
			num++;
	}
	if (num == 10)
	{
		num--;
	}

	for (i = num; i >= 1; i--)
	{
		recentFiles[i] = recentFiles[i-1];
	}
	recentFiles[0] = file;
}

void VBA::loadSettings()
{
	CString buffer;

	languageOption = regQueryDwordValue("language", 1);
	if (languageOption < 0 || languageOption > 2)
		languageOption = 1;

	buffer = regQueryStringValue("languageName", "");
	if (!buffer.IsEmpty())
	{
		languageName = buffer.Left(3);
	}
	else
		languageName = "";

	winSetLanguageOption(languageOption, true);

	frameSkip = regQueryDwordValue("frameSkip", /*2*/ 0);
	if (frameSkip < 0 || frameSkip > 9)
		frameSkip = 1;

	gbFrameSkip = regQueryDwordValue("gbFrameSkip", 0);
	if (gbFrameSkip < 0 || gbFrameSkip > 9)
		gbFrameSkip = 0;

///  autoFrameSkip = regQueryDwordValue("autoFrameSkip", FALSE) ? TRUE : FALSE;
	accuratePitchThrottle = regQueryDwordValue("accuratePitchThrottle", FALSE) ? TRUE : FALSE;

	vsync = regQueryDwordValue("vsync", false) ? true : false ;
	synchronize       = regQueryDwordValue("synchronize", 1) ? true : false;
	fullScreenStretch = regQueryDwordValue("stretch", 0) ? true : false;

	videoOption = regQueryDwordValue("video", 0);

	if (videoOption < 0 || videoOption > VIDEO_OTHER)
		videoOption = 0;

	bool defaultVideoDriver = regQueryDwordValue("defaultVideoDriver", true) ?
	                          true : false;

	if (!regQueryBinaryValue("videoDriverGUID", (char *)&videoDriverGUID,
	                         sizeof(GUID)))
	{
		defaultVideoDriver = TRUE;
	}

	if (defaultVideoDriver)
		pVideoDriverGUID = NULL;
	else
		pVideoDriverGUID = &videoDriverGUID;

	fsWidth      = regQueryDwordValue("fsWidth", 0);
	fsHeight     = regQueryDwordValue("fsHeight", 0);
	fsColorDepth = regQueryDwordValue("fsColorDepth", 0);

	if (videoOption == VIDEO_OTHER)
	{
		if (fsWidth < 0 || fsWidth > 4095 || fsHeight < 0 || fsHeight > 4095)
			videoOption = 0;
		if (fsColorDepth != 16 && fsColorDepth != 24 && fsColorDepth != 32)
			videoOption = 0;
	}

	renderMethod = (DISPLAY_TYPE)regQueryDwordValue("renderMethod", DIRECT_DRAW);

	if (renderMethod < GDI || renderMethod > OPENGL)
		renderMethod = DIRECT_DRAW;

	windowPositionX = regQueryDwordValue("windowX", 0);
	if (windowPositionX < 0)
		windowPositionX = 0;
	windowPositionY = regQueryDwordValue("windowY", 0);
	if (windowPositionY < 0)
		windowPositionY = 0;

	useBiosFile = regQueryDwordValue("useBios", 0) ? true : false;
	skipBiosFile = regQueryDwordValue("skipBios", 0) ? true : false;
	buffer = regQueryStringValue("biosFile", "");
	if (!buffer.IsEmpty())
	{
		biosFileName = buffer;
	}

	memLagEnabled     = regQueryDwordValue("memLagEnabled", false) ? true : false;
	memLagTempEnabled = memLagEnabled;

	useOldGBTiming = regQueryDwordValue("useOldGBTiming", false) ? true : false;

	allowLeftRight = regQueryDwordValue("allowLeftRight", false) ? true : false;

	int res = regQueryDwordValue("soundEnable", 0x30f);

	soundEnable(res);
	soundDisable(~res);

	soundOffFlag = (regQueryDwordValue("soundOff", 0)) ? true : false;
	soundQuality = regQueryDwordValue("soundQuality", 2);
	soundEcho = regQueryDwordValue("soundEcho", 0) ? true : false;
	soundLowPass = regQueryDwordValue("soundLowPass", 0) ? true : false;
	soundReverse = regQueryDwordValue("soundReverse", 0) ? true : false;
	soundVolume = regQueryDwordValue("soundVolume", 0);
	if (soundVolume < 0 || soundVolume > 5)
		soundVolume = 0;

	ddrawEmulationOnly  = regQueryDwordValue("ddrawEmulationOnly", false) ? true : false;
	ddrawUseVideoMemory = regQueryDwordValue("ddrawUseVideoMemory", false) ? true : false;
	tripleBuffering     = regQueryDwordValue("tripleBuffering", true) ? true : false;

	d3dFilter = regQueryDwordValue("d3dFilter", 0);
	if (d3dFilter < 0 || d3dFilter > 1)
		d3dFilter = 0;
	glFilter = regQueryDwordValue("glFilter", 0);
	if (glFilter < 0 || glFilter > 1)
		glFilter = 0;
	glType = regQueryDwordValue("glType", 0);
	if (glType < 0 || glType > 1)
		glType = 0;

	filterType = regQueryDwordValue("filter", 0);
	if (filterType < 0 || filterType > 16)
		filterType = 0;

	disableMMX = regQueryDwordValue("disableMMX", 0) ? true : false;

	disableStatusMessage = regQueryDwordValue("disableStatus", 0) ? true : false;

	showSpeed = regQueryDwordValue("showSpeed", 1);
	if (showSpeed < 0 || showSpeed > 2)
		showSpeed = 1;

	showSpeedTransparent = regQueryDwordValue("showSpeedTransparent", TRUE) ?
	                       TRUE : FALSE;

	winGbPrinterEnabled = regQueryDwordValue("gbPrinter", false) ? true : false;

	if (winGbPrinterEnabled)
		gbSerialFunction = gbPrinterSend;
	else
		gbSerialFunction = NULL;

	pauseWhenInactive = regQueryDwordValue("pauseWhenInactive", 1) ? true : false;

	filenamePreference = regQueryDwordValue("filenamePreference", 0);

	frameCounter  = regQueryDwordValue("frameCounter", false) ? true : false;
	lagCounter    = regQueryDwordValue("lagCounter", false) ? true : false;
	inputDisplay  = regQueryDwordValue("inputDisplay", false) ? true : false;
	movieReadOnly = regQueryDwordValue("movieReadOnly", false) ? true : false;

	altAviRecordMethod = regQueryDwordValue("altAviRecordMethod", false) ? true : false;

	useOldSync = regQueryDwordValue("useOldSync", 0) ? TRUE : FALSE;

	muteFrameAdvance = regQueryDwordValue("muteFrameAdvance", 0) ? TRUE : FALSE;

	captureFormat = regQueryDwordValue("captureFormat", 0);

//	removeIntros = regQueryDwordValue("removeIntros", false) ? true : false;

	recentFreeze = regQueryDwordValue("recentFreeze", false) ? true : false;

	autoIPS = regQueryDwordValue("autoIPS", true) ? true : false;

	cpuDisableSfx = regQueryDwordValue("disableSfx", 0) ? true : false;

	winSaveType = regQueryDwordValue("saveType", 0);
	if (winSaveType < 0 || winSaveType > 5)
		winSaveType = 0;

	cpuEnhancedDetection = regQueryDwordValue("enhancedDetection", 1) ? true :
	                       false;

	ifbType = regQueryDwordValue("ifbType", 0);
	if (ifbType < 0 || ifbType > 2)
		ifbType = 0;

	winFlashSize = regQueryDwordValue("flashSize", 0x10000);
	if (winFlashSize != 0x10000 && winFlashSize != 0x20000)
		winFlashSize = 0x10000;

	agbPrintEnable(regQueryDwordValue("agbPrint", 0) ? true : false);

	winRtcEnable = regQueryDwordValue("rtcEnabled", 0) ? true : false;
	rtcEnable(winRtcEnable);

	autoHideMenu = regQueryDwordValue("autoHideMenu", 0) ? true : false;

	switch (videoOption)
	{
	case VIDEO_320x240 :
		fsWidth      = 320;
		fsHeight     = 240;
		fsColorDepth = 16;
		break;
	case VIDEO_640x480:
		fsWidth      = 640;
		fsHeight     = 480;
		fsColorDepth = 16;
		break;
	case VIDEO_800x600:
		fsWidth      = 800;
		fsHeight     = 600;
		fsColorDepth = 16;
		break;
	}

	winGbBorderOn     = regQueryDwordValue("borderOn", 0);
	gbBorderAutomatic = regQueryDwordValue("borderAutomatic", 0);
	gbEmulatorType    = regQueryDwordValue("emulatorType", 0);
	if (gbEmulatorType < 0 || gbEmulatorType > 5)
		gbEmulatorType = 1;
	gbColorOption = regQueryDwordValue("colorOption", 0);

	outlinedText    = regQueryDwordValue("outlinedText", TRUE) != 0;
	transparentText = regQueryDwordValue("transparentText", FALSE) != 0;
	textColor       = regQueryDwordValue("textColor", 0);
	textMethod      = regQueryDwordValue("textMethod", 1);

	threadPriority = regQueryDwordValue("priority", 2);

	if (threadPriority < 0 || threadPriority > 3)
		threadPriority = 2;
	updatePriority();

	autoSaveLoadCheatList = regQueryDwordValue("autoSaveCheatList", 0) ? true : false;

	pauseDuringCheatSearch = regQueryDwordValue("pauseDuringCheatSearch2", 0) ? true : false;

	gbPaletteOption = regQueryDwordValue("gbPaletteOption", 0);
	if (gbPaletteOption < 0)
		gbPaletteOption = 0;
	if (gbPaletteOption > 2)
		gbPaletteOption = 2;

	regQueryBinaryValue("gbPalette", (char *)systemGbPalette, 24*sizeof(u16));

	rewindTimer = regQueryDwordValue("rewindTimer", 0);
	rewindSlots = regQueryDwordValue("rewindSlots", 64);

	if (rewindTimer < 0 || rewindTimer > 600)
		rewindTimer = 0;

	if (rewindSlots <= 0)
		rewindTimer = rewindSlots = 0;
	if (rewindSlots > MAX_REWIND_SLOTS)
		rewindSlots = MAX_REWIND_SLOTS;

	if (rewindTimer != 0)
	{
		if (rewindMemory == NULL)
			rewindMemory = (char *)malloc(rewindSlots*REWIND_SIZE);
	}

	if (frameSearchMemory == NULL)
		frameSearchMemory = (char *)malloc(3*REWIND_SIZE);

	for (int i = 0; i < 10; i++)
	{
		buffer.Format("recent%d", i);
		char *s = regQueryStringValue(buffer, NULL);
		if (s == NULL)
			break;
		recentFiles[i] = s;
	}

	joypadDefault = regQueryDwordValue("joypadDefault", 0);
	if (joypadDefault < 0 || joypadDefault > 3)
		joypadDefault = 0;

	autoLoadMostRecent = regQueryDwordValue("autoLoadMostRecent", false) ? true :
	                     false;

	loadMakesRecent = regQueryDwordValue("loadMakesRecent", false) ? true : false;

	loadMakesCurrent = regQueryDwordValue("loadMakesCurrent", false) ? true : false;
	saveMakesCurrent = regQueryDwordValue("saveMakesCurrent", false) ? true : false;
	currentSlot      = regQueryDwordValue("currentSlot", 0);
	showSlotTime     = regQueryDwordValue("showSlotTime", 0);

	cheatsEnabled = regQueryDwordValue("cheatsEnabled", true) ? true : false;

	fsMaxScale = regQueryDwordValue("fsMaxScale", 0);

	throttle = regQueryDwordValue("throttle", 0);
	if (throttle < 5 || throttle > 1000)
		throttle = 100;

	movieOnEndBehavior = regQueryDwordValue("movieOnEndBehavior", 0);
	movieOnEndPause    = regQueryDwordValue("movieOnEndPause", 0) ? true : false;
	//RamWatch Settings
	AutoRWLoad = regQueryDwordValue(AUTORWLOAD, false);
	RWSaveWindowPos = regQueryDwordValue(RWSAVEPOS, false);
	ramw_x = regQueryDwordValue(RAMWX, 0);
	ramw_y = regQueryDwordValue(RAMWY, 0);
	char str[2048];
	for (int i = 0; i < MAX_RECENT_WATCHES; i++)
	{
		sprintf(str, "Recent Watch %d", i+1);
//		strcpy(rw_recent_files[i], regQueryStringValue(str, NULL));
	}
}

void VBA::updateFrameSkip()
{
	switch (cartridgeType)
	{
	case 0:
		systemFrameSkip = frameSkip;
		break;
	case 1:
		systemFrameSkip = gbFrameSkip;
		break;
	}
}

void VBA::updateVideoSize(UINT id)
{
	int  value       = 0;
	bool forceUpdate = false;

	switch (id)
	{
	case ID_OPTIONS_VIDEO_X1:
		value       = VIDEO_1X;
		forceUpdate = true;
		break;
	case ID_OPTIONS_VIDEO_X2:
		value       = VIDEO_2X;
		forceUpdate = true;
		break;
	case ID_OPTIONS_VIDEO_X3:
		value       = VIDEO_3X;
		forceUpdate = true;
		break;
	case ID_OPTIONS_VIDEO_X4:
		value       = VIDEO_4X;
		forceUpdate = true;
		break;
	case ID_OPTIONS_VIDEO_FULLSCREEN320X240:
		value        = VIDEO_320x240;
		fsWidth      = 320;
		fsHeight     = 240;
		fsColorDepth = 16;
		break;
	case ID_OPTIONS_VIDEO_FULLSCREEN640X480:
		value        = VIDEO_640x480;
		fsWidth      = 640;
		fsHeight     = 480;
		fsColorDepth = 16;
		break;
	case ID_OPTIONS_VIDEO_FULLSCREEN800X600:
		value        = VIDEO_800x600;
		fsWidth      = 800;
		fsHeight     = 600;
		fsColorDepth = 16;
		break;
	case ID_OPTIONS_VIDEO_FULLSCREEN:
		value       = VIDEO_OTHER;
		forceUpdate = true;
		break;
	}

	if (videoOption != value || forceUpdate)
		updateWindowSize(value);
}

typedef BOOL (WINAPI *GETMENUBARINFO)(HWND, LONG, LONG, PMENUBARINFO);

static void winCheckMenuBarInfo(int& winSizeX, int& winSizeY)
{
	HINSTANCE hinstDll;
	DWORD     dwVersion = 0;

	hinstDll = /**/ ::LoadLibrary("USER32.DLL");

	if (hinstDll)
	{
		GETMENUBARINFO func = (GETMENUBARINFO)GetProcAddress(hinstDll,
		                                                     "GetMenuBarInfo");

		if (func)
		{
			MENUBARINFO info;
			info.cbSize = sizeof(info);

			func(AfxGetMainWnd()->GetSafeHwnd(), OBJID_MENU, 0, &info);

			int menuHeight = GetSystemMetrics(SM_CYMENU);

			if ((info.rcBar.bottom - info.rcBar.top) > menuHeight)
			{
				winSizeY += (info.rcBar.bottom - info.rcBar.top) - menuHeight + 1;
				theApp.m_pMainWnd->SetWindowPos(
				    0,                     //HWND_TOPMOST,
				    theApp.windowPositionX,
				    theApp.windowPositionY,
				    winSizeX,
				    winSizeY,
				    SWP_NOMOVE | SWP_SHOWWINDOW);
			}
		}
		/**/ ::FreeLibrary(hinstDll);
	}
}

void VBA::updateWindowSize(int value)
{
	regSetDwordValue("video", value);

	if (value == VIDEO_OTHER)
	{
		regSetDwordValue("fsWidth", fsWidth);
		regSetDwordValue("fsHeight", fsHeight);
		regSetDwordValue("fsColorDepth", fsColorDepth);
	}

	if (((value >= VIDEO_320x240) &&
	     (videoOption != value)) ||
	    (videoOption >= VIDEO_320x240 &&
	     value <= VIDEO_4X) ||
	    fsForceChange)
	{
		fsForceChange     = false;
		changingVideoSize = true;
		shutdownDisplay();
		if (input)
		{
			delete input;
			input = NULL;
		}
		m_pMainWnd->DragAcceptFiles(FALSE);
		CWnd *pWnd = m_pMainWnd;
		m_pMainWnd = NULL;
		pWnd->DestroyWindow();
		delete pWnd;
		videoOption = value;
		if (!initDisplay())
		{
			if (videoOption == VIDEO_320x240 ||
			    videoOption == VIDEO_640x480 ||
			    videoOption == VIDEO_800x600 ||
			    videoOption == VIDEO_OTHER)
			{
				regSetDwordValue("video", VIDEO_1X);
				if (pVideoDriverGUID)
					regSetDwordValue("defaultVideoDriver", TRUE);
			}
			changingVideoSize = false;
			AfxPostQuitMessage(0);
			return;
		}
		if (!initInput())
		{
			changingVideoSize = false;
			AfxPostQuitMessage(0);
			return;
		}
		input->checkKeys();
		updateMenuBar();
		changingVideoSize = FALSE;
		updateWindowSize(videoOption);
		return;
	}

	sizeX = 240;
	sizeY = 160;

	videoOption = value;

	if (cartridgeType == 1)
	{
		if (gbBorderOn)
		{
			sizeX = 256;
			sizeY = 224;
			gbBorderLineSkip   = 256;
			gbBorderColumnSkip = 48;
			gbBorderRowSkip    = 40;
		}
		else
		{
			sizeX = 160;
			sizeY = 144;
			gbBorderLineSkip   = 160;
			gbBorderColumnSkip = 0;
			gbBorderRowSkip    = 0;
		}
	}

	surfaceSizeX = sizeX;
	surfaceSizeY = sizeY;

	switch (videoOption)
	{
	case VIDEO_1X:
		surfaceSizeX = sizeX;
		surfaceSizeY = sizeY;
		break;
	case VIDEO_2X:
		surfaceSizeX = sizeX * 2;
		surfaceSizeY = sizeY * 2;
		break;
	case VIDEO_3X:
		surfaceSizeX = sizeX * 3;
		surfaceSizeY = sizeY * 3;
		break;
	case VIDEO_4X:
		surfaceSizeX = sizeX * 4;
		surfaceSizeY = sizeY * 4;
		break;
	case VIDEO_320x240:
	case VIDEO_640x480:
	case VIDEO_800x600:
	case VIDEO_OTHER:
	{
		int scaleX = 1;
		int scaleY = 1;
		scaleX = (fsWidth / sizeX);
		scaleY = (fsHeight / sizeY);
		int min = scaleX < scaleY ? scaleX : scaleY;
		if (fsMaxScale)
			min = min > fsMaxScale ? fsMaxScale : min;
		surfaceSizeX = min * sizeX;
		surfaceSizeY = min * sizeY;
		if ((fullScreenStretch && (display != NULL &&
		                           (display->getType() != DIRECT_3D)))
		    || (display != NULL && display->getType() >= DIRECT_3D))
		{
			surfaceSizeX = fsWidth;
			surfaceSizeY = fsHeight;
		}
		break;
	}
	}

	rect.right  = sizeX;
	rect.bottom = sizeY;

	int winSizeX = sizeX;
	int winSizeY = sizeY;

	if (videoOption <= VIDEO_4X)
	{
		dest.left   = 0;
		dest.top    = 0;
		dest.right  = surfaceSizeX;
		dest.bottom = surfaceSizeY;

		DWORD style = WS_POPUP | WS_VISIBLE;

		style |= WS_OVERLAPPEDWINDOW;

		menuToggle = TRUE;
		AdjustWindowRectEx(&dest, style, flagHideMenu ? FALSE : TRUE, 0); //WS_EX_TOPMOST);

		winSizeX = dest.right-dest.left;
		winSizeY = dest.bottom-dest.top;

		m_pMainWnd->SetWindowPos(0, //HWND_TOPMOST,
		                         windowPositionX,
		                         windowPositionY,
		                         winSizeX,
		                         winSizeY,
		                         SWP_NOMOVE | SWP_SHOWWINDOW);

		winCheckMenuBarInfo(winSizeX, winSizeY);
	}

	adjustDestRect();

	updateIFB();
	updateFilter();

	m_pMainWnd->RedrawWindow(NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
}

bool VBA::initDisplay()
{
	return updateRenderMethod(false);
}

bool VBA::updateRenderMethod(bool force)
{
	bool res = updateRenderMethod0(force);

	while (!res && renderMethod > 0)
	{
		if (renderMethod == OPENGL)
			renderMethod = DIRECT_3D;
		else if (renderMethod == DIRECT_3D)
			renderMethod = DIRECT_DRAW;
		else if (renderMethod == DIRECT_DRAW)
		{
			if (videoOption > VIDEO_4X)
			{
				videoOption = VIDEO_2X;
				force       = true;
			}
			else
				renderMethod = GDI;
		}

		res = updateRenderMethod(force);
	}
	return res;
}

bool VBA::updateRenderMethod0(bool force)
{
	bool initInput = false;

	if (display)
	{
		if (display->getType() != renderMethod || force)
		{
			initInput         = true;
			changingVideoSize = true;
			shutdownDisplay();
			if (input)
			{
				delete input;
				input = NULL;
			}
			CWnd *pWnd = m_pMainWnd;

			m_pMainWnd = NULL;
			pWnd->DragAcceptFiles(FALSE);
			pWnd->DestroyWindow();
			delete pWnd;

			display = NULL;
			regSetDwordValue("renderMethod", renderMethod);
		}
	}
	if (display == NULL)
	{
		switch (renderMethod)
		{
		case GDI:
			display = newGDIDisplay();
			break;
		case DIRECT_DRAW:
			display = newDirectDrawDisplay();
			break;
		case DIRECT_3D:
			display = newDirect3DDisplay();
			break;
		case OPENGL:
			display = newOpenGLDisplay();
			break;
		}

		if (display->initialize())
		{
			adjustDestRect();
			updateMenuBar();
			if (initInput)
			{
				if (!this->initInput())
				{
					changingVideoSize = false;
					AfxPostQuitMessage(0);
					return false;
				}
				input->checkKeys();
				updateMenuBar();
				changingVideoSize = false;
				updateWindowSize(videoOption);

				m_pMainWnd->ShowWindow(SW_SHOW);
				m_pMainWnd->UpdateWindow();
				m_pMainWnd->SetFocus();

				return true;
			}
			else
			{
				changingVideoSize = false;
				return true;
			}
		}
		changingVideoSize = false;
	}
	return true;
}

void VBA::winCheckFullscreen()
{
	if (videoOption > VIDEO_4X && tripleBuffering)
	{
		if (display)
			display->checkFullScreen();
	}
}

void VBA::shutdownDisplay()
{
	if (display != NULL)
	{
		display->cleanup();
		delete display;
		display = NULL;
	}
}

void VBA::directXMessage(const char *msg)
{
	systemMessage(
	    IDS_DIRECTX_7_REQUIRED,
	    "DirectX 7.0 or greater is required to run.\nDownload at http://www.microsoft.com/directx.\n\nError found at: %s",
	    msg);
}

void VBA::updatePriority()
{
	switch (threadPriority)
	{
	case 0:
		SetThreadPriority(THREAD_PRIORITY_HIGHEST);
		break;
	case 1:
		SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);
		break;
	case 3:
		SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
		break;
	default:
		SetThreadPriority(THREAD_PRIORITY_NORMAL);
	}
}

#ifdef MMX
bool VBA::detectMMX()
{
	bool support = false;
	char brand[13];

	// check for Intel chip
	__try {
		__asm {
			mov eax, 0;
			cpuid;
			mov [dword ptr brand+0], ebx;
			mov [dword ptr brand+4], edx;
			mov [dword ptr brand+8], ecx;
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
		{
			return false;
		}
		return false;
	}
	// Check for Intel or AMD CPUs
	if (strncmp(brand, "GenuineIntel", 12))
	{
		if (strncmp(brand, "AuthenticAMD", 12))
		{
			return false;
		}
	}

	__asm {
		mov eax, 1;
		cpuid;
		test edx, 00800000h;
		jz   NotFound;
		mov [support], 1;
NotFound:
	}
	return support;
}

#endif

void VBA::winSetLanguageOption(int option, bool force)
{
	if (((option == languageOption) && option != 2) && !force)
		return;
	switch (option)
	{
	case 0:
	{
		char lbuffer[10];

		if (GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SABBREVLANGNAME,
		                  lbuffer, 10))
		{
			HINSTANCE l = winLoadLanguage(lbuffer);
			if (l == NULL)
			{
				LCID locIdBase = MAKELCID(MAKELANGID(PRIMARYLANGID(GetSystemDefaultLangID()), SUBLANG_NEUTRAL), SORT_DEFAULT);
				if (GetLocaleInfo(locIdBase, LOCALE_SABBREVLANGNAME,
				                  lbuffer, 10))
				{
					l = winLoadLanguage(lbuffer);
					if (l == NULL)
					{
						systemMessage(IDS_FAILED_TO_LOAD_LIBRARY,
						              "Failed to load library %s",
						              lbuffer);
						return;
					}
				}
			}
			AfxSetResourceHandle(l);
			if (languageModule != NULL)
				/**/ ::FreeLibrary(languageModule);
			languageModule = l;
		}
		else
		{
			systemMessage(IDS_FAILED_TO_GET_LOCINFO,
			              "Failed to get locale information");
			return;
		}
		break;
	}
	case 1:
		if (languageModule != NULL)
			/**/ ::FreeLibrary(languageModule);
		languageModule = NULL;
		AfxSetResourceHandle(AfxGetInstanceHandle());
		break;
	case 2:
	{
		if (!force)
		{
			LangSelect dlg;
			if (dlg.DoModal())
			{
				HINSTANCE l = winLoadLanguage(languageName);
				if (l == NULL)
				{
					systemMessage(IDS_FAILED_TO_LOAD_LIBRARY,
					              "Failed to load library %s",
					              languageName);
					return;
				}
				AfxSetResourceHandle(l);
				if (languageModule != NULL)
					/**/ ::FreeLibrary(languageModule);
				languageModule = l;
			}
		}
		else
		{
			if (languageName.IsEmpty())
				return;
			HINSTANCE l = winLoadLanguage(languageName);
			if (l == NULL)
			{
				systemMessage(IDS_FAILED_TO_LOAD_LIBRARY,
				              "Failed to load library %s",
				              languageName);
				return;
			}
			AfxSetResourceHandle(l);
			if (languageModule != NULL)
				FreeLibrary(languageModule);
			languageModule = l;
		}
		break;
	}
	}
	languageOption = option;
	updateMenuBar();
}

HINSTANCE VBA::winLoadLanguage(const char *name)
{
	CString buffer;

	buffer.Format("vba_%s.dll", name);

	HINSTANCE l = /**/ ::LoadLibrary(buffer);

	if (l == NULL)
	{
		if (strlen(name) == 3)
		{
			char buffer2[3];
			buffer2[0] = name[0];
			buffer2[1] = name[1];
			buffer2[2] = 0;
			buffer.Format("vba_%s.dll", buffer2);

			return /**/ ::LoadLibrary(buffer);
		}
	}
	return l;
}

bool VBA::initInput()
{
	if (input)
		delete input;
	input = newDirectInput();
	if (input->initialize())
	{
		input->loadSettings();
		input->checkKeys();
		return true;
	}
	delete input;
	return false;
}

void VBA::winAddUpdateListener(IUpdateListener *l)
{
	updateList.AddTail(l);
	updateCount++;
}

void VBA::winRemoveUpdateListener(IUpdateListener *l)
{
	POSITION pos = updateList.Find(l);
	if (pos)
	{
		updateList.RemoveAt(pos);
		updateCount--;
		if (updateCount < 0)
			updateCount = 0;
	}
}

CString VBA::winLoadFilter(UINT id)
{
	CString res = winResLoadString(id);
	res.Replace('_', '|');

	return res;
}

void VBA::saveSettings()
{
	regSetDwordValue("language", languageOption);
	regSetStringValue("languageName", languageName);

	regSetDwordValue("frameSkip", frameSkip);
	regSetDwordValue("gbFrameSkip", gbFrameSkip);

///  regSetDwordValue("autoFrameSkip", autoFrameSkip);
	regSetDwordValue("accuratePitchThrottle", accuratePitchThrottle);

	regSetDwordValue("vsync", vsync);
	regSetDwordValue("synchronize", synchronize);
	regSetDwordValue("stretch", fullScreenStretch);

	regSetDwordValue("video", videoOption);

	regSetDwordValue("defaultVideoDriver", pVideoDriverGUID == NULL);

	if (pVideoDriverGUID)
	{
		regSetBinaryValue("videoDriverGUID", (char *)&videoDriverGUID,
		                  sizeof(GUID));
	}

	regSetDwordValue("fsWidth", fsWidth);
	regSetDwordValue("fsHeight", fsHeight);
	regSetDwordValue("fsColorDepth", fsColorDepth);

	regSetDwordValue("renderMethod", renderMethod);

	regSetDwordValue("windowX", windowPositionX);
	regSetDwordValue("windowY", windowPositionY);

	regSetDwordValue("useBios", useBiosFile);
	regSetDwordValue("skipBios", skipBiosFile);
	if (!biosFileName.IsEmpty())
		regSetStringValue("biosFile", biosFileName);

	regSetDwordValue("memLagEnabled", memLagEnabled);
	regSetDwordValue("useOldGBTiming", useOldGBTiming);

	regSetDwordValue("allowLeftRight", allowLeftRight);

	regSetDwordValue("soundEnable", soundGetEnable() & 0x30f);
	regSetDwordValue("soundOff", soundOffFlag);
	regSetDwordValue("soundQuality", soundQuality);
	regSetDwordValue("soundEcho", soundEcho);
	regSetDwordValue("soundLowPass", soundLowPass);
	regSetDwordValue("soundReverse", soundReverse);
	regSetDwordValue("soundVolume", soundVolume);

	regSetDwordValue("ddrawEmulationOnly", ddrawEmulationOnly);
	regSetDwordValue("ddrawUseVideoMemory", ddrawUseVideoMemory);
	regSetDwordValue("tripleBuffering", tripleBuffering);

	regSetDwordValue("d3dFilter", d3dFilter);
	regSetDwordValue("glFilter", glFilter);
	regSetDwordValue("glType", glType);

	regSetDwordValue("filter", filterType);
	regSetDwordValue("disableMMX", disableMMX);

	regSetDwordValue("disableStatus", disableStatusMessage);

	regSetDwordValue("showSpeed", showSpeed);
	regSetDwordValue("showSpeedTransparent", showSpeedTransparent);

	regSetDwordValue("gbPrinter", winGbPrinterEnabled);

	regSetDwordValue("pauseWhenInactive", pauseWhenInactive);

	regSetDwordValue("filenamePreference", filenamePreference);

	regSetDwordValue("frameCounter", frameCounter);
	regSetDwordValue("lagCounter", lagCounter);
	regSetDwordValue("inputDisplay", inputDisplay);
	regSetDwordValue("movieReadOnly", movieReadOnly);

	regSetDwordValue("altAviRecordMethod", altAviRecordMethod);

	regSetDwordValue("useOldSync", useOldSync);

	regSetDwordValue("muteFrameAdvance", muteFrameAdvance);

	regSetDwordValue("captureFormat", captureFormat);

//	regSetDwordValue("removeIntros", removeIntros);

	regSetDwordValue("recentFreeze", recentFreeze);

	regSetDwordValue("autoIPS", autoIPS);

	regSetDwordValue("disableSfx", cpuDisableSfx);

	regSetDwordValue("saveType", winSaveType);

	regSetDwordValue("enhancedDetection", cpuEnhancedDetection);

	regSetDwordValue("ifbType", ifbType);

	regSetDwordValue("flashSize", winFlashSize);

	regSetDwordValue("agbPrint", agbPrintIsEnabled());

	regSetDwordValue("rtcEnabled", winRtcEnable);

	regSetDwordValue("autoHideMenu", autoHideMenu);

	regSetDwordValue("borderOn", winGbBorderOn);
	regSetDwordValue("borderAutomatic", gbBorderAutomatic);
	regSetDwordValue("emulatorType", gbEmulatorType);
	regSetDwordValue("colorOption", gbColorOption);

	regSetDwordValue("outlinedText", outlinedText);
	regSetDwordValue("transparentText", transparentText);
	regSetDwordValue("textColor", textColor);
	regSetDwordValue("textMethod", textMethod);

	regSetDwordValue("priority", threadPriority);

	regSetDwordValue("autoSaveCheatList", autoSaveLoadCheatList);

	regSetDwordValue("pauseDuringCheatSearch2", pauseDuringCheatSearch);

	regSetDwordValue("gbPaletteOption", gbPaletteOption);

	regSetBinaryValue("gbPalette", (char *)systemGbPalette, 24*sizeof(u16));

	regSetDwordValue("rewindTimer", rewindTimer);
	regSetDwordValue("rewindSlots", rewindSlots);

	CString buffer;
	for (int i = 0; i < 10; i++)
	{
		buffer.Format("recent%d", i);
		regSetStringValue(buffer, recentFiles[i]);
	}

	regSetDwordValue("joypadDefault", joypadDefault);
	regSetDwordValue("autoLoadMostRecent", autoLoadMostRecent);
	regSetDwordValue("loadMakesRecent", loadMakesRecent);
	regSetDwordValue("loadMakesCurrent", loadMakesCurrent);
	regSetDwordValue("saveMakesCurrent", saveMakesCurrent);
	regSetDwordValue("currentSlot", currentSlot);
	regSetDwordValue("showSlotTime", showSlotTime);

	regSetDwordValue("cheatsEnabled", cheatsEnabled);
	regSetDwordValue("fsMaxScale", fsMaxScale);
	regSetDwordValue("throttle", throttle);
	regSetDwordValue("movieOnEndBehavior", movieOnEndBehavior);
	regSetDwordValue("movieOnEndPause", movieOnEndPause);
}

void winSignal(int, int)
{}

#define CPUReadByteQuick(addr) \
    map[(addr)>>24].address[(addr) & map[(addr)>>24].mask]

void winOutput(char *s, u32 addr)
{
	if (s)
	{
		toolsLog(s);
	}
	else
	{
		CString str;
		char    c;

		c = CPUReadByteQuick(addr);
		addr++;
		while (c)
		{
			str += c;
			c    = CPUReadByteQuick(addr);
			addr++;
		}
		toolsLog(str);
	}
}

