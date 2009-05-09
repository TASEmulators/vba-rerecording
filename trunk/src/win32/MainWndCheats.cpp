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
#include "MainWnd.h"

#include "resource.h"
#include "FileDlg.h"
#include "GBACheats.h"
#include "GBCheatsDlg.h"
#include "Reg.h"
#include "WinResUtil.h"
#include "VBA.h"

#include "../GBA.h"
#include "../Globals.h"
#include "../gb/gbCheats.h"

extern int emulating;

GBACheatSearch gbaDlg;
GBCheatSearch gbDlg;

void MainWnd::OnCheatsSearchforcheats() 
{
  theApp.winCheckFullscreen();

  if(!theApp.pauseDuringCheatSearch && theApp.modelessCheatDialogIsOpen)
  {
    gbaDlg.DestroyWindow();
    gbDlg.DestroyWindow();
	theApp.modelessCheatDialogIsOpen = false;
  }

  if(theApp.cartridgeType == 0)
  {
	if(theApp.pauseDuringCheatSearch)
	{
		gbaDlg.DoModal();
	}
	else
	{
		if(!theApp.modelessCheatDialogIsOpen)
		{
			theApp.modelessCheatDialogIsOpen = true;
			gbaDlg.Create(GBACheatSearch::IDD, theApp.m_pMainWnd);
		}
	}
  }
  else
  {
	if(theApp.pauseDuringCheatSearch)
	{
	    gbDlg.DoModal();
	}
	else
	{
		if(!theApp.modelessCheatDialogIsOpen)
		{
			theApp.modelessCheatDialogIsOpen = true;
			gbDlg.Create(GBACheatSearch::IDD, theApp.m_pMainWnd);
		}
	}
  }
}

void MainWnd::OnUpdateCheatsSearchforcheats(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
}

void MainWnd::OnCheatsCheatlist() 
{
  theApp.winCheckFullscreen();
  if(theApp.cartridgeType == 0) {
    GBACheatList dlg;
    dlg.DoModal();
  } else {
    GBCheatList dlg;
    dlg.DoModal();
  }
}

void MainWnd::OnUpdateCheatsCheatlist(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
}

void MainWnd::OnCheatsAutomaticsaveloadcheats() 
{
  theApp.autoSaveLoadCheatList = !theApp.autoSaveLoadCheatList;
}

void MainWnd::OnUpdateCheatsAutomaticsaveloadcheats(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(theApp.autoSaveLoadCheatList);
}

void MainWnd::OnCheatsPauseDuringCheatSearch() 
{
  theApp.pauseDuringCheatSearch = !theApp.pauseDuringCheatSearch;
}

void MainWnd::OnUpdateCheatsPauseDuringCheatSearch(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(theApp.pauseDuringCheatSearch);
}

void MainWnd::OnCheatsLoadcheatlist() 
{
  theApp.winCheckFullscreen();
  CString buffer;
  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    buffer = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    buffer = theApp.filename;

  CString cheatDir = regQueryStringValue(IDS_CHEAT_DIR, NULL);

  if(cheatDir.IsEmpty())
    cheatDir = getDirFromFile(theApp.filename);

  if(isDriveRoot(cheatDir))
    filename.Format("%s%s.clt", cheatDir, buffer);
  else
    filename.Format("%s\\%s.clt", cheatDir, buffer);

  LPCTSTR exts[] = { ".clt" };
  CString filter = winLoadFilter(IDS_FILTER_CHEAT_LIST);
  CString title = winResLoadString(IDS_SELECT_CHEAT_LIST_NAME);

  FileDlg dlg(this, filename, filter, 0, "CLT", exts, cheatDir, title, false);

  if(dlg.DoModal() == IDOK) {
    winLoadCheatList(dlg.GetPathName());
  }
}

void MainWnd::OnUpdateCheatsLoadcheatlist(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
}

void MainWnd::OnCheatsSavecheatlist() 
{
  theApp.winCheckFullscreen();
  CString buffer;
  CString filename;

  int index = theApp.filename.ReverseFind('\\');

  if(index != -1)
    buffer = theApp.filename.Right(theApp.filename.GetLength()-index-1);
  else
    buffer = theApp.filename;

  CString cheatDir = regQueryStringValue(IDS_CHEAT_DIR, NULL);

  if(cheatDir.IsEmpty())
    cheatDir = getDirFromFile(theApp.filename);

  if(isDriveRoot(cheatDir))
    filename.Format("%s%s.clt", cheatDir, buffer);
  else
    filename.Format("%s\\%s.clt", cheatDir, buffer);

  LPCTSTR exts[] = { ".clt" };
  CString filter = winLoadFilter(IDS_FILTER_CHEAT_LIST);
  CString title = winResLoadString(IDS_SELECT_CHEAT_LIST_NAME);

  FileDlg dlg(this, filename, filter, 0, "CLT", exts, cheatDir, title, true);

  if(dlg.DoModal() == IDOK) {
    winSaveCheatList(dlg.GetPathName());
  }
}

void MainWnd::OnUpdateCheatsSavecheatlist(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(emulating);
}

void MainWnd::OnCheatsDisablecheats() 
{
  cheatsEnabled = !cheatsEnabled;
}

void MainWnd::OnUpdateCheatsDisablecheats(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(!cheatsEnabled);
}

