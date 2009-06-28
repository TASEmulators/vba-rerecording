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

#include "../vbalua.h"

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

	GetDlgItem(IDC_LUA_FILENAME)->SetWindowText("");

	return TRUE; // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void LuaOpenDialog::OnBnClickedBrowse()
{
	extern char *regQueryStringValue(const char *key, char *def);  // from Reg.cpp
	CString capdir = regQueryStringValue(IDS_LUA_DIR, "");
	CString luaName;
	CString captureBuffer;

	if (capdir.IsEmpty())
		capdir = ((MainWnd *)theApp.m_pMainWnd)->getDirFromFile(theApp.filename);

	CString filename = "";
	if (emulating)
	{
		filename = theApp.szFile;
		int slash = max(filename.ReverseFind('/'), max(filename.ReverseFind('\\'), filename.ReverseFind('|')));
		if (slash != -1)
			filename = filename.Right(filename.GetLength()-slash-1);
		int dot = filename.Find('.');
		if (dot != -1)
			filename = filename.Left(dot);
		filename += ".lua";
	}

	CString filter = theApp.winLoadFilter(IDS_FILTER_LUA);
	CString title  = winResLoadString(IDS_SELECT_LUA_NAME);

	LPCTSTR exts[] = { ".lua", NULL };

	FileDlg dlg(this, filename, filter, 1, "lua", exts, capdir, title, false, true);

	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}

	luaName       = dlg.GetPathName();
	captureBuffer = luaName;

	if (dlg.m_ofn.nFileOffset > 0)
	{
		captureBuffer = captureBuffer.Left(dlg.m_ofn.nFileOffset);
	}

	int len = captureBuffer.GetLength();

	if (len > 3 && captureBuffer[len-1] == '\\')
		captureBuffer = captureBuffer.Left(len-1);

	extern void regSetStringValue(const char *key, const char *value);   // from Reg.cpp
	regSetStringValue(IDS_LUA_DIR, captureBuffer);

	GetDlgItem(IDC_LUA_FILENAME)->SetWindowText(luaName);
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

