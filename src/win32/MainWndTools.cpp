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

#include "stdafx.h"

#include "resource.h"
#include "MainWnd.h"
#include "AccelEditor.h"
#include "AVIWrite.h"
#include "Disassemble.h"
#include "FileDlg.h"
#include "GBDisassemble.h"
#include "GBMapView.h"
#include "GBMemoryViewerDlg.h"
#include "GBOamView.h"
#include "GBPaletteView.h"
#include "GBTileView.h"
#include "GDBConnection.h"
#include "IOViewer.h"
#include "MapView.h"
#include "MemoryViewerDlg.h"
#include "MovieOpen.h"
#include "MovieCreate.h"
#include "OamView.h"
#include "PaletteView.h"
#include "Reg.h"
#include "TileView.h"
#include "WavWriter.h"
#include "WinResUtil.h"
#include "WinMiscUtil.h"
#include "VBA.h"

#include "../gba/GBA.h"
#include "../gba/GBAGlobals.h"
#include "../gb/GB.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern int32 gbBorderOn;
extern int32 soundQuality;

extern bool debugger;
extern int  emulating;
extern int  remoteSocket;

extern void remoteCleanUp();
extern void remoteSetSockets(SOCKET, SOCKET);
extern void toolsLogging();

void MainWnd::OnToolsDisassemble()
{
	if (theApp.cartridgeType == 0)
	{
		Disassemble *dlg = new Disassemble();
		dlg->Create(IDD_DISASSEMBLE, this);
		dlg->ShowWindow(SW_SHOW);
	}
	else
	{
		GBDisassemble *dlg = new GBDisassemble();
		dlg->Create(IDD_GB_DISASSEMBLE, this);
		dlg->ShowWindow(SW_SHOW);
	}
}

void MainWnd::OnUpdateToolsDisassemble(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X);
}

void MainWnd::OnToolsLogging()
{
	toolsLogging();
}

void MainWnd::OnUpdateToolsLogging(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X);
}

void MainWnd::OnToolsIoviewer()
{
	IOViewer *dlg = new IOViewer;
	dlg->Create(IDD_IO_VIEWER, this);
	dlg->ShowWindow(SW_SHOW);
}

void MainWnd::OnUpdateToolsIoviewer(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X && theApp.cartridgeType == 0);
}

void MainWnd::OnToolsMapview()
{
	if (theApp.cartridgeType == 0)
	{
		MapView *dlg = new MapView;
		dlg->Create(IDD_MAP_VIEW, this);
		dlg->ShowWindow(SW_SHOW);
	}
	else
	{
		GBMapView *dlg = new GBMapView;
		dlg->Create(IDD_GB_MAP_VIEW, this);
		dlg->ShowWindow(SW_SHOW);
	}
}

void MainWnd::OnUpdateToolsMapview(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X);
}

void MainWnd::OnToolsMemoryviewer()
{
	if (theApp.cartridgeType == 0)
	{
		MemoryViewerDlg *dlg = new MemoryViewerDlg;
		dlg->Create(IDD_MEM_VIEWER, this);
		dlg->ShowWindow(SW_SHOW);
	}
	else
	{
		GBMemoryViewerDlg *dlg = new GBMemoryViewerDlg;
		dlg->Create(IDD_MEM_VIEWER, this);
		dlg->ShowWindow(SW_SHOW);
	}
}

void MainWnd::OnUpdateToolsMemoryviewer(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X);
}

void MainWnd::OnToolsOamviewer()
{
	if (theApp.cartridgeType == 0)
	{
		OamView *dlg = new OamView;
		dlg->Create(IDD_OAM_VIEW, this);
		dlg->ShowWindow(SW_SHOW);
	}
	else
	{
		GBOamView *dlg = new GBOamView;
		dlg->Create(IDD_GB_OAM_VIEW, this);
		dlg->ShowWindow(SW_SHOW);
	}
}

void MainWnd::OnUpdateToolsOamviewer(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X);
}

void MainWnd::OnToolsPaletteview()
{
	if (theApp.cartridgeType == 0)
	{
		PaletteView *dlg = new PaletteView;
		dlg->Create(IDD_PALETTE_VIEW, this);
		dlg->ShowWindow(SW_SHOW);
	}
	else
	{
		GBPaletteView *dlg = new GBPaletteView;
		dlg->Create(IDD_GB_PALETTE_VIEW, this);
		dlg->ShowWindow(SW_SHOW);
	}
}

void MainWnd::OnUpdateToolsPaletteview(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X);
}

void MainWnd::OnToolsTileviewer()
{
	if (theApp.cartridgeType == 0)
	{
		TileView *dlg = new TileView;
		dlg->Create(IDD_TILE_VIEWER, this);
		dlg->ShowWindow(SW_SHOW);
	}
	else
	{
		GBTileView *dlg = new GBTileView;
		dlg->Create(IDD_GB_TILE_VIEWER, this);
		dlg->ShowWindow(SW_SHOW);
	}
}

void MainWnd::OnUpdateToolsTileviewer(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X);
}

void MainWnd::OnDebugNextframe()
{
	if (theApp.paused)
		theApp.paused = false;
	theApp.winPauseNextFrame = true;
}

void MainWnd::OnUpdateDebugNextframe(CCmdUI*pCmdUI)
{}

void MainWnd::OnDebugNextframeAccountForLag()
{
	theApp.nextframeAccountForLag = !theApp.nextframeAccountForLag;
}

void MainWnd::OnUpdateDebugNextframeAccountForLag(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.nextframeAccountForLag);
}

#if (defined(WIN32) && !defined(SDL))
extern u32 currentButtons [4];     // from System.cpp
#else
u32 currentButtons [4];     /// SDL FIXME movie support NYI (the input format is quite different)
#endif

void MainWnd::OnDebugFramesearch()
{
	extern SMovie Movie;
	if (!theApp.frameSearching)
	{
		// starting a new search
		theApp.frameSearching       = true;
		theApp.frameSearchStart     = (Movie.state == MOVIE_STATE_NONE) ? GBASystemCounters.frameCount : Movie.currentFrame;
		theApp.frameSearchLength    = 0;
		theApp.frameSearchLoadValid = false;
		theApp.emulator.emuWriteMemState(&theApp.frameSearchMemory[REWIND_SIZE*0], REWIND_SIZE); // 0 is start state, 1 is
                                                                                                 // intermediate state (for
                                                                                                 // speedup when going forward),
                                                                                                 // 2 is end state
		theApp.emulator.emuWriteMemState(&theApp.frameSearchMemory[REWIND_SIZE*1], REWIND_SIZE);

		// store old buttons from frame before the search
		for (int i = 0; i < 4; i++)
			theApp.frameSearchOldInput[i] = currentButtons[i];
	}
	else
	{
		// advance forward 1 step in the search
		theApp.frameSearchLength++;

		// try it
		theApp.emulator.emuReadMemState(&theApp.frameSearchMemory[REWIND_SIZE*1], REWIND_SIZE);
	}

	char str [32];
	sprintf(str, "%d frame search", theApp.frameSearchLength);
	systemScreenMessage(str, 0);

	theApp.frameSearchSkipping = true;

	// make sure the display updates at least 1 frame to show the new message
	theApp.frameSearchFirstStep = true;

	if (theApp.paused)
		theApp.paused = false;
}

void MainWnd::OnUpdateDebugFramesearch(CCmdUI*pCmdUI)
{
	extern SMovie Movie;
	pCmdUI->Enable(emulating && Movie.state != MOVIE_STATE_PLAY);
	pCmdUI->SetCheck(theApp.frameSearching);
}

void MainWnd::OnDebugFramesearchPrev()
{
	if (theApp.frameSearching)
	{
		if (theApp.frameSearchLength > 0)
		{
			// rewind 1 step in the search
			theApp.frameSearchLength--;
		}

		// try it
		theApp.emulator.emuReadMemState(&theApp.frameSearchMemory[REWIND_SIZE*0], REWIND_SIZE);

		char str[32];
		sprintf(str, "%d frame search", theApp.frameSearchLength);
		systemScreenMessage(str, 0);

		theApp.frameSearchSkipping = true;

		// make sure the display updates at least 1 frame to show the new message
		theApp.frameSearchFirstStep = true;

		if (theApp.paused)
			theApp.paused = false;
	}
}

void MainWnd::OnUpdateDebugFramesearchPrev(CCmdUI*pCmdUI)
{
	extern SMovie Movie;
	pCmdUI->Enable(emulating && theApp.frameSearching && Movie.state != MOVIE_STATE_PLAY);
}

void MainWnd::OnDebugFramesearchLoad()
{
	if (theApp.frameSearchLoadValid)
	{
		theApp.emulator.emuReadMemState(&theApp.frameSearchMemory[REWIND_SIZE*2], REWIND_SIZE);
		theApp.paused = true;

		if (theApp.frameSearching)
			systemScreenMessage("end frame search", 0);
		else
			systemScreenMessage("restore search end", 0);
	}
	theApp.frameSearching      = false;
	theApp.frameSearchSkipping = false;
}

void MainWnd::OnUpdateDebugFramesearchLoad(CCmdUI*pCmdUI)
{
	extern SMovie Movie;
	pCmdUI->Enable(emulating && Movie.state != MOVIE_STATE_PLAY);
}

void MainWnd::OnToolsFrameCounter()
{
	theApp.frameCounter = !theApp.frameCounter;
	extern void VBAUpdateFrameCountDisplay(); VBAUpdateFrameCountDisplay();
}

void MainWnd::OnUpdateToolsFrameCounter(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.frameCounter);
}

void MainWnd::OnToolsLagCounter()
{
	theApp.lagCounter = !theApp.lagCounter;
	extern void VBAUpdateFrameCountDisplay(); VBAUpdateFrameCountDisplay();
}

void MainWnd::OnUpdateToolsLagCounter(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.lagCounter);
}

void MainWnd::OnToolsLagCounterReset()
{
	GBASystemCounters.lagCount = 0;
}

void MainWnd::OnToolsInputDisplay()
{
	theApp.inputDisplay = !theApp.inputDisplay;
	systemScreenMessage(theApp.inputDisplay ? "Input Display On" : "Input Display Off");
	extern void VBAUpdateButtonPressDisplay(); VBAUpdateButtonPressDisplay();
}

void MainWnd::OnUpdateToolsInputDisplay(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.inputDisplay);
}

void MainWnd::OnToolsDebugGdb()
{
	theApp.winCheckFullscreen();
	GDBPortDlg dlg;

	if (dlg.DoModal())
	{
		GDBWaitingDlg wait(dlg.getSocket(), dlg.getPort());
		if (wait.DoModal())
		{
			remoteSetSockets(wait.getListenSocket(), wait.getSocket());
			debugger  = true;
			emulating = 1;
			theApp.cartridgeType = 0;
			theApp.filename      = "\\gnu_stub";
			rom         = (u8 *)malloc(0x2000000 + 4);
			workRAM     = (u8 *)calloc(1, 0x40000 + 4);
			bios        = (u8 *)calloc(1, 0x4000 + 4);
			internalRAM = (u8 *)calloc(1, 0x8000 + 4);
			paletteRAM  = (u8 *)calloc(1, 0x400 + 4);
			vram        = (u8 *)calloc(1, 0x20000 + 4);
			oam         = (u8 *)calloc(1, 0x400 + 4);
			pix         = (u8 *)calloc(1, 4 * 240 * 160);
			ioMem       = (u8 *)calloc(1, 0x400 + 4);

			theApp.emulator = GBASystem;

			CPUInit(theApp.biosFileName, theApp.useBiosFile ? true : false);
			CPUReset();
		}
	}
}

void MainWnd::OnUpdateToolsDebugGdb(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X && remoteSocket == -1);
}

void MainWnd::OnToolsDebugLoadandwait()
{
	theApp.winCheckFullscreen();
	if (fileOpenSelect(0))
	{
		if (FileRun())
		{
			if (theApp.cartridgeType != 0)
			{
				systemMessage(IDS_ERROR_NOT_GBA_IMAGE, "Error: not a GBA image");
				OnFileClose();
				return;
			}
			GDBPortDlg dlg;

			if (dlg.DoModal())
			{
				GDBWaitingDlg wait(dlg.getSocket(), dlg.getPort());
				if (wait.DoModal())
				{
					remoteSetSockets(wait.getListenSocket(), wait.getSocket());
					debugger  = true;
					emulating = 1;
				}
			}
		}
	}
}

void MainWnd::OnUpdateToolsDebugLoadandwait(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X && remoteSocket == -1);
}

void MainWnd::OnToolsDebugBreak()
{
	if (armState)
	{
		armNextPC -= 4;
		reg[15].I -= 4;
	}
	else
	{
		armNextPC -= 2;
		reg[15].I -= 2;
	}
	debugger = true;
}

void MainWnd::OnUpdateToolsDebugBreak(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X && remoteSocket != -1);
}

void MainWnd::OnToolsDebugDisconnect()
{
	remoteCleanUp();
	debugger = false;
}

void MainWnd::OnUpdateToolsDebugDisconnect(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption <= VIDEO_4X && remoteSocket != -1);
}

void MainWnd::OnToolsSoundRecording()
{
	if (!theApp.soundRecording)
		OnToolsSoundStartrecording();
	else
		OnToolsSoundStoprecording();
}

void MainWnd::OnToolsSoundStartrecording()
{
	theApp.winCheckFullscreen();

	CString wavName = theApp.filename;

	if (VBAMovieActive())
	{
		extern SMovie Movie;
		wavName = Movie.filename;
		int index = wavName.ReverseFind('.');
		if (index != -1)
			wavName = wavName.Left(index);
	}

	LPCTSTR exts[] = { ".wav", NULL };

	CString filter = theApp.winLoadFilter(IDS_FILTER_WAV);
	CString title  = winResLoadString(IDS_SELECT_WAV_NAME);

	wavName = winGetDestFilename(wavName, IDS_WAV_DIR, exts[0]);
	CString wavDir = winGetDestDir(IDS_WAV_DIR);

	FileDlg dlg(this, wavName, filter, 1, "WAV", exts, wavDir, title, true);

	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}

	theApp.soundRecordName = dlg.GetPathName();
	theApp.soundRecording = true;
}

void MainWnd::OnToolsSoundStoprecording()
{
	if (theApp.soundRecorder)
	{
		delete theApp.soundRecorder;
		theApp.soundRecorder = NULL;
	}
	theApp.soundRecording = false;
}

void MainWnd::OnUpdateToolsSoundRecording(CCmdUI*pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
	{
		if (!theApp.soundRecording)
			pCmdUI->SetText(winResLoadString(IDS_STARTSOUNDRECORDING));
		else
			pCmdUI->SetText(winResLoadString(IDS_STOPSOUNDRECORDING));

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}

	pCmdUI->Enable(emulating);
}

void MainWnd::OnToolsAVIRecording()
{
	if (!theApp.aviRecording)
		OnToolsStartAVIRecording();
	else
		OnToolsStopAVIRecording();
}

void MainWnd::OnToolsStartAVIRecording()
{
	theApp.winCheckFullscreen();

	CString aviName = theApp.filename;

	if (VBAMovieActive())
	{
		extern SMovie Movie;
		aviName = Movie.filename;
		int index = aviName.ReverseFind('.');
		if (index != -1)
			aviName = aviName.Left(index);
	}

	LPCTSTR exts[] = { ".avi", NULL };

	CString filter = theApp.winLoadFilter(IDS_FILTER_AVI);
	CString title  = winResLoadString(IDS_SELECT_AVI_NAME);

	aviName = winGetDestFilename(aviName, IDS_AVI_DIR, exts[0]);
	CString aviDir = winGetDestDir(IDS_AVI_DIR);

	FileDlg dlg(this, aviName, filter, 1, "AVI", exts, aviDir, title, true);

	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}

	theApp.aviRecordName = theApp.soundRecordName =  dlg.GetPathName();
	theApp.aviRecording  = true;

///  extern long linearFrameCount; linearFrameCount = 0;
///  extern long linearSoundByteCount; linearSoundByteCount = 0;

	if (theApp.aviRecorder == NULL)
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

		if (theApp.aviRecorder)
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
	}
}

void MainWnd::OnToolsPauseAVIRecording()
{
	theApp.aviRecorder->Pause(!theApp.aviRecorder->IsPaused());
}

void MainWnd::OnToolsStopAVIRecording()
{
	if (theApp.aviRecorder != NULL)
	{
		delete theApp.aviRecorder;
		theApp.aviRecorder = NULL;
	}
	theApp.aviRecording = false;
}

void MainWnd::OnUpdateToolsAVIRecording(CCmdUI*pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
	{
		if (!theApp.aviRecording)
			pCmdUI->SetText(winResLoadString(IDS_STARTAVIRECORDING));
		else
			pCmdUI->SetText(winResLoadString(IDS_STOPAVIRECORDING));

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}

	pCmdUI->Enable(emulating);
}

void MainWnd::OnUpdateToolsPauseAVIRecording(CCmdUI*pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
	{
		if (!theApp.aviRecording)
		{
			pCmdUI->SetText(winResLoadString(IDS_PAUSEAVIRECORDING));
			theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
			pCmdUI->Enable(false);
		}
		else
		{
			if (!theApp.aviRecorder->IsPaused())
				pCmdUI->SetText(winResLoadString(IDS_PAUSEAVIRECORDING));
			else
				pCmdUI->SetText(winResLoadString(IDS_RESUMEAVIRECORDING));

			theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
			pCmdUI->Enable(emulating);
		}
	}
}

void MainWnd::OnToolsRecordMovie()
{
	MovieCreate dlg;
	dlg.DoModal();
}

void MainWnd::OnUpdateToolsRecordMovie(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnToolsStopMovie()
{
	VBAMovieStop(false);
}

void MainWnd::OnUpdateToolsStopMovie(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating && VBAMovieActive());
}

void MainWnd::OnToolsPlayMovie()
{
	MovieOpen dlg;
	dlg.DoModal();
}

void MainWnd::OnUpdateToolsPlayMovie(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnToolsPlayReadOnly()
{
	if (!VBAMovieActive()) {
		theApp.movieReadOnly = !theApp.movieReadOnly;
		systemScreenMessage(theApp.movieReadOnly  ? "Movie now read-only" : "Movie now editable");
	}
	else
		VBAMovieToggleReadOnly();
}

void MainWnd::OnUpdateToolsPlayReadOnly(CCmdUI*pCmdUI)
{
///  pCmdUI->Enable(VBAMovieActive()); // FIXME: this is right, but disabling menu items screws up accelerators until you view
// the menu!
///  pCmdUI->SetCheck(VBAMovieReadOnly());
	pCmdUI->Enable(TRUE); // TEMP
	pCmdUI->SetCheck(VBAMovieActive() ? VBAMovieReadOnly() : theApp.movieReadOnly);
}

void MainWnd::OnToolsResumeRecord()
{
	VBAMovieSwitchToRecording();
}

void MainWnd::OnUpdateToolsResumeRecord(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(!VBAMovieReadOnly() && VBAMovieGetState() == MOVIE_STATE_PLAY);
}

void MainWnd::OnToolsPlayRestart()
{
	VBAMovieRestart();
}

void MainWnd::OnUpdateToolsPlayRestart(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(VBAMovieActive());
}

void MainWnd::OnToolsOnMovieEndPause()
{
	theApp.movieOnEndPause = !theApp.movieOnEndPause;
}

void MainWnd::OnUpdateToolsOnMovieEndPause(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.movieOnEndPause);
}

void MainWnd::OnToolsOnMovieEndStop()
{
	theApp.movieOnEndBehavior = 0;
}

void MainWnd::OnUpdateToolsOnMovieEndStop(CCmdUI*pCmdUI)
{
	pCmdUI->SetRadio(theApp.movieOnEndBehavior == 0);
}

void MainWnd::OnToolsOnMovieEndRestart()
{
	theApp.movieOnEndBehavior = 1;
}

void MainWnd::OnUpdateToolsOnMovieEndRestart(CCmdUI*pCmdUI)
{
	pCmdUI->SetRadio(theApp.movieOnEndBehavior == 1);
}

void MainWnd::OnToolsOnMovieEndRerecord()
{
	theApp.movieOnEndBehavior = 2;
}

void MainWnd::OnUpdateToolsOnMovieEndRerecord(CCmdUI*pCmdUI)
{
	pCmdUI->SetRadio(theApp.movieOnEndBehavior == 2);
}

#include "assert.h"
void MainWnd::OnToolsRewind()
{
	assert(theApp.rewindTimer > 0 && theApp.rewindSlots > 0);
	if (emulating && theApp.emulator.emuReadMemState && theApp.rewindMemory && theApp.rewindCount)
	{
		assert(theApp.rewindPos >= 0 && theApp.rewindPos < theApp.rewindSlots);
		theApp.rewindPos = (--theApp.rewindPos + theApp.rewindSlots) % theApp.rewindSlots;
		assert(theApp.rewindPos >= 0 && theApp.rewindPos < theApp.rewindSlots);
		theApp.emulator.emuReadMemState(&theApp.rewindMemory[REWIND_SIZE*theApp.rewindPos], REWIND_SIZE);
		theApp.rewindCount--;
		if (theApp.rewindCount > 0)
			theApp.rewindCounter = 0;
		else
		{
			theApp.rewindCounter    = theApp.rewindTimer;
			theApp.rewindSaveNeeded = true;

			// immediately save state to avoid eroding away the earliest you can rewind to
			theApp.saveRewindStateIfNecessary();

			theApp.rewindSaveNeeded = false;
		}
	}
}

void MainWnd::OnUpdateToolsRewind(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.rewindMemory != NULL && emulating && theApp.rewindCount);
}

void MainWnd::OnToolsCustomize()
{
	AccelEditor dlg;

	if (dlg.DoModal())
	{
		theApp.winAccelMgr = dlg.mgr;
		theApp.winAccelMgr.UpdateWndTable();
		theApp.winAccelMgr.Write();
	}

	theApp.winAccelMgr.UpdateMenu(theApp.menu);	// we should always do this since the menu has been reloaded
}

void MainWnd::OnUpdateToolsCustomize(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption != VIDEO_320x240);
}
