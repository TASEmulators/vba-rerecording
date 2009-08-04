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
#include "ExportGSASnapshot.h"
#include "FileDlg.h"
#include "GSACodeSelect.h"
#include "RomInfo.h"
#include "Reg.h"
#include "WinResUtil.h"
#include "LuaOpenDialog.h"
#include "ram_search.h"
#include "ramwatch.h"
#include "VBA.h"

#include "../GBA.h"
#include "../Globals.h"
#include "../Cheats.h"
#include "../EEprom.h"
#include "../NLS.h"
#include "../Sound.h"
#include "../gb/GB.h"
#include "../gb/gbCheats.h"
#include "../gb/gbGlobals.h"
#include "../movie.h"
#include "../vbalua.h"
#include "../version.h"

extern int emulating;

extern void remoteCleanUp();

void MainWnd::OnFileOpen()
{
	theApp.winCheckFullscreen();
	if (fileOpenSelect(0))
	{
		FileRun();
	}
}

void MainWnd::OnFileOpenGBx()
{
	theApp.winCheckFullscreen();
	if (fileOpenSelect(1))
	{
		FileRun();
	}
}

void MainWnd::OnFilePause()
{
	if (!theApp.paused)
	{
/*		if (theApp.throttle > 25)
		{
			// less immediate response, better update
			// FIXME: fails to pause at the first frame immeadiately after loading a ROM
			OnDebugNextframe();
		}
		else
		{
*/			// more immediate response, worse update
			theApp.paused   = !theApp.paused;

			// otherwise would crash when not emulating
			if (emulating)
			{
				theApp.painting = true;
				systemDrawScreen();
				theApp.painting = false;
			}
/*		}
*/
		theApp.wasPaused = true;
		soundPause();
	}
	else
	{
		theApp.paused = !theApp.paused;
		theApp.wasPaused = false;
		soundResume();
	}
}

void MainWnd::OnUpdateFilePause(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.paused);
}

void MainWnd::OnFileReset()
{
	if (emulating)
	{
		if (VBAMovieGetState() == MOVIE_STATE_PLAY)
		{
			VBAMovieRestart();
			systemScreenMessage("movie play restart");
		}
		else
		{
			theApp.emulator.emuReset();
			systemScreenMessage(winResLoadString(IDS_RESET));
		}
	}
}

void MainWnd::OnUpdateFileReset(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnUpdateFileRecentFreeze(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.recentFreeze);

	if (pCmdUI->m_pMenu == NULL)
		return;

	CMenu *pMenu = pCmdUI->m_pMenu;

	int i;
	for (i = 0; i < 10; i++)
	{
		if (!pMenu->RemoveMenu(ID_FILE_MRU_FILE1+i, MF_BYCOMMAND))
			break;
	}

	for (i = 0; i < 10; i++)
	{
		CString p = theApp.recentFiles[i];
		if (p.GetLength() == 0)
			break;
		int index = max(p.ReverseFind('/'), max(p.ReverseFind('\\'), p.ReverseFind('|')));

		if (index != -1)
			p = p.Right(p.GetLength()-index-1);

		pMenu->AppendMenu(MF_STRING, ID_FILE_MRU_FILE1+i, p);
	}
	theApp.winAccelMgr.UpdateMenu((HMENU)*pMenu);
}

BOOL MainWnd::OnFileRecentFile(UINT nID)
{
	if (theApp.recentFiles[(nID&0xFFFF)-ID_FILE_MRU_FILE1].GetLength())
	{
		theApp.szFile = theApp.recentFiles[(nID&0xFFFF)-ID_FILE_MRU_FILE1];
		if (FileRun())
			emulating = true;
		else
		{
			emulating = false;
			soundPause();
		}
	}
	return TRUE;
}

void MainWnd::OnFileRecentReset()
{
	CString str1 = "Really clear your recent ROMs list?"; //winResLoadString(IDS_REALLY_CLEAR);
	CString str2 = winResLoadString(IDS_CONFIRM_ACTION);

	if (MessageBox(str1,
	               str2,
	               MB_YESNO|MB_DEFBUTTON2) == IDNO)
		return;

	int i = 0;
	for (i = 0; i < 10; i++)
		theApp.recentFiles[i] = "";
}

void MainWnd::OnFileRecentFreeze()
{
	theApp.recentFreeze = !theApp.recentFreeze;
}

void MainWnd::OnFileExit()
{
	if (AskSave())
		SendMessage(WM_CLOSE);
}

void MainWnd::OnFileClose()
{
	// save battery file before we change the filename...
	if (rom != NULL || gbRom != NULL)
	{
		if (theApp.autoSaveLoadCheatList)
			winSaveCheatListDefault();
		writeBatteryFile();
		soundPause();
		theApp.emulator.emuCleanUp();
		CloseRamWindows();
		remoteCleanUp();
		if (VBAMovieActive())
			VBAMovieStop(false); // will only get here on user selecting to stop playing the ROM, canceling movie
	}
	emulating = 0;
	RedrawWindow(NULL, NULL, RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
	systemSetTitle(VBA_NAME_AND_VERSION);
}

void MainWnd::OnUpdateFileClose(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnFileLoad()
{
	theApp.winCheckFullscreen();

	LPCTSTR exts[] = { ".sgm", NULL };
	CString filter = winLoadFilter(IDS_FILTER_SGM);
	CString title  = winResLoadString(IDS_SELECT_SAVE_GAME_NAME);

	CString saveName = getRelatedFilename(theApp.filename, IDS_SAVE_DIR, exts[0]);
	CString saveDir = getRelatedDir(IDS_SAVE_DIR);

	FileDlg dlg(this, saveName, filter, 0, "SGM", exts, saveDir, title, false, true);

	if (dlg.DoModal() == IDOK)
	{
		bool res = loadSaveGame(dlg.GetPathName());

		theApp.rewindCount      = 0;
		theApp.rewindCounter    = 0;
		theApp.rewindSaveNeeded = false;

		if (res)
			systemScreenMessage(winResLoadString(IDS_LOADED_STATE));
	}
}

void MainWnd::OnUpdateFileLoad(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

// maybe these should go to util.cpp
static bool FileExists(const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if (f)
	{
		fclose(f);
		return true;
	}
	return false;
}

bool MainWnd::isDriveRoot(const CString& file)
{
	if (file.GetLength() == 3)
	{
		if (file[1] == ':' && file[2] == '\\')
			return true;
	}
	return false;
}

CString MainWnd::getDiskFilename(const CString& file)
{
	int index = file.Find('|');

	if (index != -1)
		return file.Left(index);
	else
		return file;
}

CString MainWnd::getDirFromFile(const CString& file)
{
	CString temp  = getDiskFilename(file);
	int index = max(temp.ReverseFind('/'), temp.ReverseFind('\\'));
	if (index != -1)
	{
		temp = temp.Left(index);
		if (temp.GetLength() == 2 && temp[1] == ':')
			temp += "\\";
	}

	return temp;
}

CString MainWnd::getRelatedDir(const CString& TargetDirReg)
{
	CString targetDir = regQueryStringValue(TargetDirReg, NULL);

	if (targetDir.IsEmpty())
		targetDir = getDirFromFile(theApp.filename);

	return targetDir;
}

CString MainWnd::getRelatedFilename(const CString& LogicalRomName, const CString& TargetDirReg, const CString& ext)
{
	if (LogicalRomName.GetLength() == 0)
		 return CString();

	CString targetDir = getRelatedDir(TargetDirReg);
	if (!isDriveRoot(targetDir))
		targetDir += '\\';

	CString buffer = LogicalRomName;

	int index = max(buffer.ReverseFind('/'), max(buffer.ReverseFind('\\'), buffer.ReverseFind('|')));
	if (index != -1)
		buffer = buffer.Right(buffer.GetLength()-index-1);

	index = buffer.ReverseFind('.');
	if (index != -1)
		buffer = buffer.Left(index);

	CString filename;
	filename.Format("%s%s%s", targetDir, buffer, ext);
	bool fileExists = FileExists(filename);

	// check for old style of naming, for better backward compatibility
	if (!fileExists || theApp.filenamePreference == 0)
	{
		index = LogicalRomName.Find('|');
		if (index != -1)
		{
			buffer = LogicalRomName.Left(index);
			index = max(buffer.ReverseFind('/'), buffer.ReverseFind('\\'));
			
			int dotIndex = buffer.ReverseFind('.');
			if (dotIndex > index)
				buffer = buffer.Left(dotIndex);

			if (index != -1)
				buffer = buffer.Right(buffer.GetLength()-index-1);

			CString filename2;
			filename2.Format("%s%s%s", targetDir, buffer, ext);
			bool file2Exists = FileExists(filename2);

			if ((file2Exists && !fileExists) || (theApp.filenamePreference == 0 && (file2Exists || !fileExists)))
				return filename2;
		}
	}

	return filename;
}

CString MainWnd::getSavestateFilename(const CString& LogicalRomName, int nID)
{
	CString ext;
	ext.Format("%d.sgm", nID);

	return getRelatedFilename(LogicalRomName, IDS_SAVE_DIR, ext);
}

CString MainWnd::getSavestateMenuString(const CString& LogicalRomName, int nID)
{
	CString str;
	if (theApp.showSlotTime)
	{
		CFileStatus status;
		if (emulating && CFile::GetStatus(getSavestateFilename(theApp.filename, nID), status))
		{
			str.Format("#&%d %s", nID, status.m_mtime.Format("%Y/%m/%d %H:%M:%S"));
		}
		else
		{
			str.Format("#&%d ----/--/-- --:--:--", nID);
		}
	}
	else
	{
		str.Format("Slot #&%d", nID);
	}

	return str;
}

BOOL MainWnd::OnFileLoadSlot(UINT nID)
{
	nID = nID + 1 - ID_FILE_LOADGAME_SLOT1;

	CString filename = getSavestateFilename(theApp.filename, nID);

	bool res = loadSaveGame(filename);

	// deleting rewinds because you loaded a save state is stupid
///  theApp.rewindCount = 0;
///  theApp.rewindCounter = 0;
///  theApp.rewindSaveNeeded = false;

	if (res)
	{
		CString format;
		if (VBAMovieActive())
			if (VBAMovieReadOnly())
				format = winResLoadString(IDS_REPLAYED_STATE_N);
			else
				format = winResLoadString(IDS_RERECORDED_STATE_N);
		else
			format = winResLoadString(IDS_LOADED_STATE_N);
		CString buffer;
		buffer.Format(format, nID);
		systemScreenMessage(buffer);

		int lastSlot = theApp.currentSlot;

		if (theApp.loadMakesRecent && VBAMovieActive() == loadedMovieSnapshot)
		{
			OnFileSaveSlot(nID - 1 + ID_FILE_SAVEGAME_SLOT1); // to update the file's modification date
		}

		if (theApp.loadMakesCurrent)
			theApp.currentSlot = nID - 1;
		else
			theApp.currentSlot = lastSlot; // restore value in case the call to OnFileSaveSlot changed it

		theApp.frameSearching      = false;
		theApp.frameSearchSkipping = false;
	}

	return res;
}

void MainWnd::OnFileSave()
{
	theApp.winCheckFullscreen();

	LPCTSTR exts[] = { ".sgm", NULL };
	CString filter = winLoadFilter(IDS_FILTER_SGM);
	CString title  = winResLoadString(IDS_SELECT_SAVE_GAME_NAME);

	CString saveName = getRelatedFilename(theApp.filename, IDS_SAVE_DIR, exts[0]);
	CString saveDir = getRelatedDir(IDS_SAVE_DIR);

	FileDlg dlg(this, saveName, filter, 0, "SGM", exts, saveDir, title, true);

	if (dlg.DoModal() == IDOK)
	{
		bool res = writeSaveGame(dlg.GetPathName());
		if (res)
			systemScreenMessage(winResLoadString(IDS_WROTE_STATE));
	}
}

void MainWnd::OnUpdateFileSave(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

BOOL MainWnd::OnFileSaveSlot(UINT nID)
{
	nID = nID + 1 - ID_FILE_SAVEGAME_SLOT1;

	if (theApp.saveMakesCurrent)
		theApp.currentSlot = nID - 1;

	CString filename = getSavestateFilename(theApp.filename, nID);

	bool res = writeSaveGame(filename);

	CString format = winResLoadString(IDS_WROTE_STATE_N);
	CString buffer;
	buffer.Format(format, nID);

	systemScreenMessage(buffer);

	return res;
}

BOOL MainWnd::OnSelectSlot(UINT nID)
{
	nID -= ID_SELECT_SLOT1;
	theApp.currentSlot = nID;

	CString buffer;
	buffer.Format("Slot %d selected", nID + 1);
	systemScreenMessage(buffer, 0, 600);

	return true;
}

void MainWnd::OnFileImportBatteryfile()
{
	theApp.winCheckFullscreen();

	LPCTSTR exts[] = { ".sav", NULL };
	CString filter = winLoadFilter(IDS_FILTER_SAV);
	CString title  = winResLoadString(IDS_SELECT_BATTERY_FILE);

	CString batteryName = getRelatedFilename(theApp.filename, IDS_BATTERY_DIR, exts[0]);
	CString batteryDir = getRelatedDir(IDS_BATTERY_DIR);

	FileDlg dlg(this, batteryName, filter, 0, "SAV", exts, batteryDir, title, false, true);

	if (dlg.DoModal() == IDCANCEL)
		return;

	CString str1 = winResLoadString(IDS_SAVE_WILL_BE_LOST);
	CString str2 = winResLoadString(IDS_CONFIRM_ACTION);

	if (MessageBox(str1,
	               str2,
	               MB_OKCANCEL) == IDCANCEL)
		return;

	bool res = false;

	res = theApp.emulator.emuReadBattery(dlg.GetPathName());

	if (!res)
		systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", dlg.GetPathName());
	else
	{
		theApp.emulator.emuReset();
	}
}

void MainWnd::OnUpdateFileImportBatteryfile(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnFileImportGamesharkcodefile()
{
	theApp.winCheckFullscreen();

	LPCTSTR exts[] = { NULL };
	CString filter = theApp.cartridgeType == 0 ? winLoadFilter(IDS_FILTER_SPC) : winLoadFilter(IDS_FILTER_GCF);
	CString title  = winResLoadString(IDS_SELECT_CODE_FILE);

	FileDlg dlg(this, "", filter, 0, "", exts, "", title, false, true);

	if (dlg.DoModal() == IDCANCEL)
		return;

	CString str1 = winResLoadString(IDS_CODES_WILL_BE_LOST);
	CString str2 = winResLoadString(IDS_CONFIRM_ACTION);

	if (MessageBox(str1,
	               str2,
	               MB_OKCANCEL) == IDCANCEL)
		return;

	bool    res  = false;
	CString file = dlg.GetPathName();
	if (theApp.cartridgeType == 1)
		res = gbCheatReadGSCodeFile(file);
	else
	{
		res = fileImportGSACodeFile(file);
	}
}

void MainWnd::OnUpdateFileImportGamesharkcodefile(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnFileImportGamesharksnapshot()
{
	theApp.winCheckFullscreen();

	LPCTSTR exts[] = { NULL };
	CString filter = theApp.cartridgeType == 1 ? winLoadFilter(IDS_FILTER_GBS) : winLoadFilter(IDS_FILTER_SPS);
	CString title  = winResLoadString(IDS_SELECT_SNAPSHOT_FILE);

	FileDlg dlg(this, "", filter, 0, "", exts, "", title, false, true);

	if (dlg.DoModal() == IDCANCEL)
		return;

	CString str1 = winResLoadString(IDS_SAVE_WILL_BE_LOST);
	CString str2 = winResLoadString(IDS_CONFIRM_ACTION);

	if (MessageBox(str1,
	               str2,
	               MB_OKCANCEL) == IDCANCEL)
		return;

	if (theApp.cartridgeType == 1)
		gbReadGSASnapshot(dlg.GetPathName());
	else
		CPUReadGSASnapshot(dlg.GetPathName());
}

void MainWnd::OnUpdateFileImportGamesharksnapshot(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnFileExportBatteryfile()
{
	theApp.winCheckFullscreen();

	LPCTSTR exts[] = {".sav", ".dat", NULL };
	CString filter = winLoadFilter(IDS_FILTER_SAV);
	CString title  = winResLoadString(IDS_SELECT_BATTERY_FILE);

	CString batteryName = getRelatedFilename(theApp.filename, IDS_BATTERY_DIR, exts[0]);
	CString batteryDir = getRelatedDir(IDS_BATTERY_DIR);

	FileDlg dlg(this, batteryName, filter, 1, "SAV", exts, batteryDir, title, true);

	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}

	bool result = false;

	if (theApp.cartridgeType == 1)
	{
		result = gbWriteBatteryFile(dlg.GetPathName(), false);
	}
	else
		result = theApp.emulator.emuWriteBattery(dlg.GetPathName());

	if (!result)
		systemMessage(MSG_ERROR_CREATING_FILE, "Error creating file %s",
		              dlg.GetPathName());
}

void MainWnd::OnUpdateFileExportBatteryfile(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnFileExportGamesharksnapshot()
{
	theApp.winCheckFullscreen();
	if (eepromInUse)
	{
		systemMessage(IDS_EEPROM_NOT_SUPPORTED, "EEPROM saves cannot be exported");
		return;
	}

	LPCTSTR exts[] = {".sps", NULL };

	CString filter = winLoadFilter(IDS_FILTER_SPS);
	CString title  = winResLoadString(IDS_SELECT_SNAPSHOT_FILE);

	CString name = getRelatedFilename(theApp.filename, CString(), exts[0]);

	FileDlg dlg(this, name, filter, 1, "SPS", exts, "", title, true);

	if (dlg.DoModal() == IDCANCEL)
		return;

	char buffer[16];
	strncpy(buffer, (const char *)&rom[0xa0], 12);
	buffer[12] = 0;

	ExportGSASnapshot dlg2(dlg.GetPathName(), buffer, this);
	dlg2.DoModal();
}

void MainWnd::OnUpdateFileExportGamesharksnapshot(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating && theApp.cartridgeType == 0);
}

void MainWnd::OnFileScreencapture()
{
	theApp.winCheckFullscreen();

	LPCTSTR exts[] = {".png", ".bmp", NULL };

	CString filter = winLoadFilter(IDS_FILTER_PNG);
	CString title  = winResLoadString(IDS_SELECT_CAPTURE_NAME);

	CString ext;

	if (theApp.captureFormat != 0)
		ext.Format(".bmp");
	else
		ext.Format(".png");

	CString captureName = getRelatedFilename(theApp.filename, IDS_CAPTURE_DIR, ext);
	CString captureDir = getRelatedDir(IDS_CAPTURE_DIR);

	FileDlg dlg(this,
	            captureName,
	            filter,
	            theApp.captureFormat ? 2 : 1,
	            theApp.captureFormat ? "BMP" : "PNG",
	            exts,
	            captureDir,
	            title,
	            true);

	if (dlg.DoModal() == IDCANCEL)
		return;

	if (dlg.getFilterIndex() == 2)
		theApp.emulator.emuWriteBMP(dlg.GetPathName());
	else
		theApp.emulator.emuWritePNG(dlg.GetPathName());

	systemScreenMessage(winResLoadString(IDS_SCREEN_CAPTURE));
}

void MainWnd::OnUpdateFileScreencapture(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnFileRominformation()
{
	theApp.winCheckFullscreen();
	if (theApp.cartridgeType == 0)
	{
		RomInfoGBA dlg(rom);
		dlg.DoModal();
	}
	else
	{
		RomInfoGB dlg(gbRom);
		dlg.DoModal();
	}
}

void MainWnd::OnUpdateFileRominformation(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(emulating);
}

void MainWnd::OnFileTogglemenu()
{
///  if(theApp.videoOption <= VIDEO_4X)
///    return;

	theApp.menuToggle = !theApp.menuToggle;

	if (theApp.menuToggle)
	{
		theApp.updateMenuBar();
		if (theApp.tripleBuffering)
		{
			if (theApp.display)
				theApp.display->checkFullScreen();
			DrawMenuBar();
		}
	}
	else
	{
		SetMenu(NULL);
		DestroyMenu(theApp.menu);
	}

	theApp.adjustDestRect();
}

void MainWnd::OnUpdateFileTogglemenu(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(theApp.videoOption > VIDEO_4X);
}

bool MainWnd::fileImportGSACodeFile(CString& fileName)
{
	FILE *f = fopen(fileName, "rb");

	if (f == NULL)
	{
		systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", fileName);
		return false;
	}

	u32 len;
	fread(&len, 1, 4, f);
	if (len != 14)
	{
		fclose(f);
		systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
		              fileName);
		return false;
	}
	char buffer[16];
	fread(buffer, 1, 14, f);
	buffer[14] = 0;
	if (memcmp(buffer, "SharkPortCODES", 14))
	{
		fclose(f);
		systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
		              fileName);
		return false;
	}
	fseek(f, 0x1e, SEEK_SET);
	fread(&len, 1, 4, f);
	int game = 0;
	if (len > 1)
	{
		GSACodeSelect dlg(f);
		game = dlg.DoModal();
	}
	fclose(f);

	bool v3 = false;

	int index = fileName.ReverseFind('.');

	if (index != -1)
	{
		if (fileName.Right(3).CompareNoCase("XPC") == 0)
			v3 = true;
	}

	if (game != -1)
	{
		return cheatsImportGSACodeFile(fileName, game, v3);
	}

	return true;
}

void MainWnd::OnFileSavegameOldestslot()
{
	if (!emulating)
		return;

	CFileStatus status;
	CString     str;
	time_t      time  = -1;
	int         found = -1;

	for (int i = 0; i < 10; i++)
	{
		if (CFile::GetStatus(getSavestateFilename(theApp.filename, i+1), status))
		{
			if (time - status.m_mtime.GetTime() > 0 || time == -1)
			{
				time  = (time_t)status.m_mtime.GetTime();
				found = i;
			}
		}
		else
		{
			found = i;
			break;
		}
	}

	OnFileSaveSlot(ID_FILE_SAVEGAME_SLOT1+found);
}

void MainWnd::OnUpdateFileSavegameOldestslot(CCmdUI*pCmdUI)
{
	bool enabled = emulating;
	if (pCmdUI->m_pMenu != NULL)
	{
		CFileStatus status;
		time_t		time		= -1;
		int			found		= -1;

		if (emulating)
		{
			for (int i = 0; i < 10; i++)
			{
				if (CFile::GetStatus(getSavestateFilename(theApp.filename, i+1), status))
				{
					if (time - status.m_mtime.GetTime() > 0 || time == -1)
					{
						time  = (time_t)status.m_mtime.GetTime();
						found = i;
					}
				}
				else
				{
					found = i;
					break;
				}
			}
		}

		CString str;
		enabled = (found != -1);
		if (enabled)
			str.Format("&Oldest Slot (#%d)", found+1);
		else
			str.Format("&Oldest Slot", found+1);
		
		pCmdUI->SetText(str);

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}

	pCmdUI->Enable(enabled);
}

void MainWnd::OnFileLoadgameMostrecent()
{
	if (!emulating)
		return;

	CFileStatus status;
	CString     str;
	time_t      time  = 0;
	int         found = -1;

	for (int i = 0; i < 10; i++)
	{
		if (CFile::GetStatus(getSavestateFilename(theApp.filename, i+1), status))
		{
			if (status.m_mtime.GetTime() > time)
			{
				time  = (time_t)status.m_mtime.GetTime();
				found = i;
			}
		}
	}

	if (found != -1)
	{
		OnFileLoadSlot(ID_FILE_LOADGAME_SLOT1+found);
	}
}

void MainWnd::OnUpdateFileLoadgameMostrecent(CCmdUI*pCmdUI)
{
	bool enabled = emulating;
	if (pCmdUI->m_pMenu != NULL)
	{
		CFileStatus status;
		int         found = -1;

		time_t time  = 0;
		if (emulating)
		{
			for (int i = 0; i < 10; i++)
			{
				if (CFile::GetStatus(getSavestateFilename(theApp.filename, i+1), status))
				{
					if (status.m_mtime.GetTime() > time)
					{
						time  = (time_t)status.m_mtime.GetTime();
						found = i;
					}
				}
			}
		}

		CString str;
		enabled = (found != -1);
		if (enabled)
			str.Format("Most &Recent Slot (#%d)", found+1);
		else
			str.Format("Most &Recent Slot", found+1);

		pCmdUI->SetText(str);

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}

	pCmdUI->Enable(enabled);
}

void MainWnd::OnUpdateFileLoadGameSlot(CCmdUI *pCmdUI)
{
	int slotID = pCmdUI->m_nID - ID_FILE_LOADGAME_SLOT1 + 1;

	if (pCmdUI->m_pMenu != NULL)
	{
		pCmdUI->SetText(getSavestateMenuString(theApp.filename, slotID));

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}

	CFileStatus status;
	pCmdUI->Enable(emulating && CFile::GetStatus(getSavestateFilename(theApp.filename, slotID), status));
}

void MainWnd::OnUpdateFileSaveGameSlot(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
	{
		int slotID = pCmdUI->m_nID - ID_FILE_SAVEGAME_SLOT1 + 1;

		pCmdUI->SetText(getSavestateMenuString(theApp.filename, slotID));

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}

	pCmdUI->Enable(emulating);
}

void MainWnd::OnUpdateSelectSlot(CCmdUI *pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
	{
		int slot = pCmdUI->m_nID - ID_SELECT_SLOT1;

		pCmdUI->SetText(getSavestateMenuString(theApp.filename, slot + 1));

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());

		pCmdUI->SetCheck(slot == theApp.currentSlot);
	}
}

void MainWnd::OnFileLoadgameAutoloadmostrecent()
{
	theApp.autoLoadMostRecent = !theApp.autoLoadMostRecent;
}

void MainWnd::OnUpdateFileLoadgameAutoloadmostrecent(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.autoLoadMostRecent);
}

void MainWnd::OnFileLoadgameMakeRecent()
{
	theApp.loadMakesRecent = !theApp.loadMakesRecent;
}

void MainWnd::OnUpdateFileLoadgameMakeRecent(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.loadMakesRecent);
}

void MainWnd::OnFileSavegameCurrent()
{
	OnFileSaveSlot(ID_FILE_SAVEGAME_SLOT1 + theApp.currentSlot);
}

void MainWnd::OnUpdateFileSavegameCurrent(CCmdUI*pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
	{
		int slotID = theApp.currentSlot + 1;

		CString str;
		str.Format("&Current Slot (#%d)", slotID);

		pCmdUI->SetText(str);

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}

	pCmdUI->Enable(emulating);
}

void MainWnd::OnFileLoadgameCurrent()
{
	OnFileLoadSlot(ID_FILE_LOADGAME_SLOT1 + theApp.currentSlot);
}

void MainWnd::OnUpdateFileLoadgameCurrent(CCmdUI*pCmdUI)
{
	int slotID = theApp.currentSlot + 1;

	if (pCmdUI->m_pMenu != NULL)
	{
		CString str;
		str.Format("&Current Slot (#%d)", slotID);

		pCmdUI->SetText(str);

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}

	CFileStatus status;
	pCmdUI->Enable(emulating && CFile::GetStatus(getSavestateFilename(theApp.filename, slotID), status));
}

void MainWnd::OnFileLoadgameMakeCurrent()
{
	theApp.loadMakesCurrent = !theApp.loadMakesCurrent;
}

void MainWnd::OnUpdateFileLoadgameMakeCurrent(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.loadMakesCurrent);
}

void MainWnd::OnFileSavegameMakeCurrent()
{
	theApp.saveMakesCurrent = !theApp.saveMakesCurrent;
}

void MainWnd::OnUpdateFileSavegameMakeCurrent(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.saveMakesCurrent);
}

void MainWnd::OnFileSavegameIncrementSlot()
{
	theApp.currentSlot = (theApp.currentSlot + 1) % 10;

	char str [32];
	sprintf(str, "Current Slot: %d", theApp.currentSlot + 1);
	systemScreenMessage(str, 0, 600);
}

void MainWnd::OnUpdateFileSavegameIncrementSlot(CCmdUI*pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
	{
		int slotID = theApp.currentSlot + 1;

		CString str;
		str.Format("&Increase Current Slot (#%d -> #%d)", slotID, slotID % 10 + 1);

		pCmdUI->SetText(str);

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}
}

void MainWnd::OnFileSavegameDecrementSlot()
{
	theApp.currentSlot = (theApp.currentSlot + 9) % 10;

	char str [32];
	sprintf(str, "Current Slot: %d", theApp.currentSlot + 1);
	systemScreenMessage(str, 0, 600);
}

void MainWnd::OnUpdateFileSavegameDecrementSlot(CCmdUI*pCmdUI)
{
	if (pCmdUI->m_pMenu != NULL)
	{
		int slotID = theApp.currentSlot + 1;

		CString str;
		str.Format("&Decrease Current Slot (#%d -> #%d)", slotID, (slotID + 8) % 10 + 1);

		pCmdUI->SetText(str);

		theApp.winAccelMgr.UpdateMenu(pCmdUI->m_pMenu->GetSafeHmenu());
	}
}

void MainWnd::OnFileSlotDisplayModificationTime()
{
	theApp.showSlotTime = !theApp.showSlotTime;
}

void MainWnd::OnUpdateFileSlotDisplayModificationTime(CCmdUI*pCmdUI)
{
	pCmdUI->SetCheck(theApp.showSlotTime);
}

void MainWnd::OnFileLuaLoad()
{
	theApp.winCheckFullscreen();
	LuaOpenDialog dlg;
	dlg.DoModal();
}

void MainWnd::OnUpdateFileLuaLoad(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(true);
}

void MainWnd::OnFileLuaReload()
{
	VBAReloadLuaCode();
}

void MainWnd::OnUpdateFileLuaReload(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(true);
}

void MainWnd::OnFileLuaStop()
{
	// I'm going to assume that Windows will adequately guard against this being executed
	// uselessly. Even if it wasn't, it's no big deal.
	VBALuaStop();
}

void MainWnd::OnFileRamSearch()
{
	if(!RamSearchHWnd)
	{
		reset_address_info();
		LRESULT CALLBACK RamSearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		RamSearchHWnd = ::CreateDialog(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_RAMSEARCH), AfxGetMainWnd()->GetSafeHwnd(), (DLGPROC) RamSearchProc);
	}
	else
		::SetForegroundWindow(RamSearchHWnd);
}

void MainWnd::OnUpdateFileRamSearch(CCmdUI*pCmdUI)
{
	//pCmdUI->SetCheck(RamSearchHWnd != NULL);
	pCmdUI->Enable(TRUE);
}

void MainWnd::OnFileRamWatch()
{
	if(!RamWatchHWnd)
	{
		LRESULT CALLBACK RamWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		RamWatchHWnd = ::CreateDialog(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_RAMWATCH), AfxGetMainWnd()->GetSafeHwnd(), (DLGPROC) RamWatchProc);
	}
	else
		::SetForegroundWindow(RamWatchHWnd);
}

void MainWnd::OnUpdateFileRamWatch(CCmdUI*pCmdUI)
{
	//pCmdUI->SetCheck(RamWatchHWnd != NULL);
	pCmdUI->Enable(TRUE);
}

void MainWnd::OnUpdateFileLuaStop(CCmdUI*pCmdUI)
{
	pCmdUI->Enable(VBALuaRunning());
}

