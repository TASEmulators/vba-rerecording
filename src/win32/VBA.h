// -*- C++ -*-
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

// VBA.h : main header file for the VBA application
//

#if !defined(AFX_VBA_H__57514A10_49F9_4B83_A928_0D8A4A7306A3__INCLUDED_)
#define AFX_VBA_H__57514A10_49F9_4B83_A928_0D8A4A7306A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include <afxtempl.h>

#include "AcceleratorManager.h"
#include "resource.h"       // main symbols
#include "Display.h"
#include "Input.h"
#include "IUpdate.h"
#include "Sound.h"
#include "../System.h"

/////////////////////////////////////////////////////////////////////////////
// VBA:
// See VBA.cpp for the implementation of this class
//

enum {
  VIDEO_1X, VIDEO_2X, VIDEO_3X, VIDEO_4X,
  VIDEO_320x240, VIDEO_640x480, VIDEO_800x600, VIDEO_OTHER
};

#define REWIND_SIZE 400000
#define MAX_REWIND_SLOTS 256
#define SCREEN_MESSAGE_SLOTS 8

#define MAINWND_TITLE_STRING "VBA rerecording v21+ (interim)"

class AVIWrite;
class WavWriter;

class VBA : public CWinApp
{
 public:
  CMenu m_menu;
  HMENU menu;
  HMENU popup;
  bool mode320Available;
  bool mode640Available;
  bool mode800Available;
  int windowPositionX;
  int windowPositionY;
  void (*filterFunction)(u8*,u32,u8*,u8*,u32,int,int);
  void (*ifbFunction)(u8*,u32,int,int);
  int ifbType;
  int filterType;
  int filterWidth;
  int filterHeight;
  int fsWidth;
  int fsHeight;
  int fsColorDepth;
  bool fsForceChange;
  int sizeX;
  int sizeY;
  int surfaceSizeX;
  int surfaceSizeY;
  int videoOption;
  bool fullScreenStretch;
  bool disableStatusMessage;
  int showSpeed;
  BOOL showSpeedTransparent;
  int showRenderedFrames;
  bool screenMessage [SCREEN_MESSAGE_SLOTS];
  CString screenMessageBuffer [SCREEN_MESSAGE_SLOTS];
  DWORD screenMessageTime [SCREEN_MESSAGE_SLOTS];
  int screenMessageDuration [SCREEN_MESSAGE_SLOTS];
  CString screenMessageColorBuffer [SCREEN_MESSAGE_SLOTS];
  u8 *delta[257*244*4];
  bool menuToggle;
  IDisplay *display;
  int cartridgeType;
  bool soundInitialized;
  bool useBiosFile;
  bool skipBiosFile;
  CString biosFileName;
  bool useOldGBTiming;
  bool allowLeftRight;
  bool active;
  bool paused;
  CString recentFiles[10];
  bool recentFreeze;
  bool autoSaveLoadCheatList;
  bool pauseDuringCheatSearch;
  bool modelessCheatDialogIsOpen;
  FILE *winout;
  bool removeIntros;
  bool autoIPS;
  int winGbBorderOn;
  bool hideMovieBorder;
  int winFlashSize;
  bool winRtcEnable;
  int winSaveType;
  char *rewindMemory;
  int rewindPos;
  int rewindTopPos;
  int rewindCounter;
  int rewindCount;
  bool rewindSaveNeeded;
  int rewindTimer;
  int rewindSlots;
  int captureFormat;
  bool tripleBuffering;
  bool autoHideMenu;
  int throttle;
  u32 throttleLastTime;
///  u32 autoFrameSkipLastTime;
///  bool autoFrameSkip;
  bool accuratePitchThrottle;
  bool vsync;
  bool changingVideoSize;
  GUID videoDriverGUID;
  GUID *pVideoDriverGUID;
  DISPLAY_TYPE renderMethod;
  bool iconic;
  bool ddrawEmulationOnly;
  bool ddrawUsingEmulationOnly;
  bool ddrawDebug;
  bool ddrawUseVideoMemory;
  int d3dFilter;
  int glFilter;
  int glType;
  bool pauseWhenInactive;
  bool speedupToggle;
  bool useOldSync;
  bool muteFrameAdvance;
  bool frameAdvanceMuteNow;
  bool winGbPrinterEnabled;
  int threadPriority;
  bool disableMMX;
  int languageOption;
  CString languageName;
  HINSTANCE languageModule;
  int renderedFrames;
  Input *input;
  int joypadDefault;
  int autoFire, autoFire2;
  int autoHold;
  bool autoFireToggle;
  bool winPauseNextFrame;
  bool frameCounter;
  bool lagCounter;
  bool inputDisplay;
  bool movieReadOnly;
  bool movieOnEndPause;
  int movieOnEndBehavior;
  bool soundRecording;
  WavWriter *soundRecorder;
  CString soundRecordName;
  ISound *sound;
  bool aviRecording;
  AVIWrite *aviRecorder;
  CString aviRecordName;
  bool altAviRecordMethod;
  bool nvVideoLog;
  bool nvAudioLog;
  bool painting;
  int sensorX;
  int sensorY;
  int mouseCounter;
  bool wasPaused;
  int frameskipadjust;
  bool autoLoadMostRecent;
  bool loadMakesRecent;
  int fsMaxScale;
  int romSize;
  bool loadMakesCurrent;
  bool saveMakesCurrent;
  int currentSlot;
  bool frameSearching;
  bool frameSearchSkipping;
  bool frameSearchFirstStep;
  bool frameSearchLoadValid;
  int frameSearchLength;
  int frameSearchStart;
  u32 frameSearchOldInput [4];
  char *frameSearchMemory;
 
  CList<IUpdateListener *, IUpdateListener*&> updateList;
  int updateCount;

  CAcceleratorManager winAccelMgr;
  HACCEL hAccel;
  
  RECT rect;
  RECT dest;

  struct EmulatedSystem emulator;

  CString szFile;
  CString filename;
  CString dir;

  CString wndClass;

 public:
  VBA();
  ~VBA();

  void adjustDestRect();
  void updateIFB();
  void updateFilter();
  void updateMenuBar();
  void winAddUpdateListener(IUpdateListener *l);
  void winRemoveUpdateListener(IUpdateListener *l);
  CString winLoadFilter(UINT id);

  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(VBA)
 public:
  virtual BOOL InitInstance();
  virtual BOOL OnIdle(LONG lCount);
  //}}AFX_VIRTUAL

  // Implementation

 public:
  void saveSettings();
  bool	initInput();
  HINSTANCE winLoadLanguage(const char *name);
  void winSetLanguageOption(int option, bool force);
  bool detectMMX();
#ifdef MMX
#endif
  void updatePriority();
  void directXMessage(const char *msg);
  void shutdownDisplay();
  void winCheckFullscreen();
  bool updateRenderMethod0(bool force);
  bool updateRenderMethod(bool force);
  bool initDisplay();
  void updateWindowSize(int value);
  void updateVideoSize(UINT id);
  void updateFrameSkip();
  void loadSettings();
  void addRecentFile(CString file);
  void saveRewindStateIfNecessary();
  //{{AFX_MSG(VBA)
  afx_msg void OnAppAbout();
  // NOTE - the ClassWizard will add and remove member functions here.
  //    DO NOT EDIT what you see in these blocks of generated code !
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
    };

    extern VBA theApp;

#ifdef MMX
    extern "C" bool cpu_mmx;
#endif

	extern void DrawTextMessages(u8 * dest, int pitch, int left, int bottom);

    /////////////////////////////////////////////////////////////////////////////

    //{{AFX_INSERT_LOCATION}}
    // Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VBA_H__57514A10_49F9_4B83_A928_0D8A4A7306A3__INCLUDED_)
