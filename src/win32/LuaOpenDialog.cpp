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

// LuaOpenDialog.cpp : implementation file
//

//#include "stdafx.h"
//#include "resource.h"
#include "LuaOpenDialog.h"
#include "FileDlg.h"
#include "MainWnd.h"
#include "WinResUtil.h"
#include "VBA.h"

#include "../common/vbalua.h"

extern int emulating; // from VBA.cpp

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// LuaOpenDialog dialog

LuaOpenDialog::LuaOpenDialog(CWnd*pParent /*=NULL*/)
	: CDialog(LuaOpenDialog::IDD, pParent)
{}

LuaOpenDialog::~LuaOpenDialog()
{}

void LuaOpenDialog::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(LuaOpenDialog)
	DDX_Control(pDX, IDC_LUA_FILENAME, m_filename);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(LuaOpenDialog, CDialog)
//{{AFX_MSG_MAP(LuaOpenDialog)
ON_WM_DROPFILES()
ON_BN_CLICKED(IDC_LUA_BROWSE, OnBnClickedBrowse)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// LuaOpenDialog message handlers

BOOL LuaOpenDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString luaName = ((MainWnd *)theApp.m_pMainWnd)->getRelatedFilename(theApp.filename, IDS_LUA_DIR, ".lua");

	GetDlgItem(IDC_LUA_FILENAME)->SetWindowText(luaName);

	// scroll to show the rightmost side of the lua filename
	((CEdit*)GetDlgItem(IDC_LUA_FILENAME))->SetSel((DWORD)(luaName.GetLength()-1), FALSE);

	return TRUE; // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void LuaOpenDialog::OnBnClickedBrowse()
{
	theApp.winCheckFullscreen();	// FIXME: necessary or not?

	CString luaName = ((MainWnd *)theApp.m_pMainWnd)->getRelatedFilename(theApp.filename, IDS_LUA_DIR, ".lua");
	CString luaDir = ((MainWnd *)theApp.m_pMainWnd)->getRelatedDir(IDS_LUA_DIR);

	CString filter = theApp.winLoadFilter(IDS_FILTER_LUA);
	CString title  = winResLoadString(IDS_SELECT_LUA_NAME);

	LPCTSTR exts[] = { ".lua", NULL };

	FileDlg dlg(this, luaName, filter, 1, "lua", exts, luaDir, title, false, true);

	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}

	luaName = dlg.GetPathName();

	GetDlgItem(IDC_LUA_FILENAME)->SetWindowText(luaName);

	// scroll to show the rightmost side of the lua filename
	((CEdit*)GetDlgItem(IDC_LUA_FILENAME))->SetSel((DWORD)(luaName.GetLength()-1), FALSE);
}

void LuaOpenDialog::OnOK()
{
	CString filename;
	m_filename.GetWindowText(filename);
	if (VBALoadLuaCode(filename))
	{
		CDialog::OnOK();
		// For user's convenience, don't close dialog unless we're done.
		// Users who make syntax errors and fix/reload will thank us.
	}
	else
	{
		// Errors are displayed by the Lua code.
	}
}

void LuaOpenDialog::OnDropFiles(HDROP hDropInfo)
{
	char szFile[1024];
	if (DragQueryFile(hDropInfo, 0, szFile, 1024))
	{
		DragFinish(hDropInfo);
		CString filename = szFile;
		m_filename.SetWindowText(filename);
	}
}

