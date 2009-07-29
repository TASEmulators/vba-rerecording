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

// AccelEditor.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "AccelEditor.h"
#include "CmdAccelOb.h"
#include "VBA.h"

//#include "../System.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AccelEditor dialog

AccelEditor::AccelEditor(CWnd*pParent /*=NULL*/)
	: ResizeDlg(AccelEditor::IDD, pParent)
{
	//{{AFX_DATA_INIT(AccelEditor)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	mgr = theApp.winAccelMgr;
	m_commonOnly = false;
}

void AccelEditor::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AccelEditor)
	DDX_Control(pDX, IDC_CURRENTS, m_currents);
	DDX_Control(pDX, IDC_ALREADY_AFFECTED, m_alreadyAffected);
	DDX_Control(pDX, IDC_COMMANDS, m_commands);
	DDX_Control(pDX, IDC_COMMANDS_HIDDEN, m_commands_hidden);
	DDX_Control(pDX, IDC_EDIT_KEY, m_key);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AccelEditor, CDialog)
//{{AFX_MSG_MAP(AccelEditor)
ON_BN_CLICKED(ID_OK, OnOk)
ON_LBN_SELCHANGE(IDC_COMMANDS, OnSelchangeCommands)
ON_BN_CLICKED(IDC_RESET, OnReset)
ON_BN_CLICKED(IDC_ASSIGN, OnAssign)
ON_BN_CLICKED(ID_CANCEL, OnCancel)
ON_BN_CLICKED(IDC_REMOVE, OnRemove)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AccelEditor message handlers

BOOL AccelEditor::OnInitDialog()
{
	CDialog::OnInitDialog();

	DIALOG_SIZER_START(sz)
	DIALOG_SIZER_ENTRY(IDC_STATIC1, DS_MoveX)
	DIALOG_SIZER_ENTRY(IDC_STATIC2, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_STATIC3, DS_MoveX | DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_ALREADY_AFFECTED, DS_MoveY)
	DIALOG_SIZER_ENTRY(ID_OK, DS_MoveX)
	DIALOG_SIZER_ENTRY(ID_CANCEL, DS_MoveX)
	DIALOG_SIZER_ENTRY(IDC_ASSIGN, DS_MoveX)
	DIALOG_SIZER_ENTRY(IDC_REMOVE, DS_MoveX)
	DIALOG_SIZER_ENTRY(IDC_RESET, DS_MoveX)
	DIALOG_SIZER_ENTRY(IDC_CLOSE, DS_MoveY)
	DIALOG_SIZER_ENTRY(IDC_COMMANDS, DS_SizeX | DS_SizeY)
	DIALOG_SIZER_ENTRY(IDC_CURRENTS, DS_MoveX | DS_SizeY)
	DIALOG_SIZER_ENTRY(IDC_EDIT_KEY, DS_MoveX | DS_MoveY)
	DIALOG_SIZER_END()
	SetData(sz,
	        TRUE,
	        HKEY_CURRENT_USER,
	        "Software\\Emulators\\VisualBoyAdvance\\Viewer\\AccelEditor",
	        NULL);

	InitCommands();

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

void AccelEditor::InitCommands()
{
	m_commands.ResetContent();
	m_commands_hidden.ResetContent();
	m_alreadyAffected.SetWindowText("");

	POSITION pos = mgr.m_mapAccelString.GetStartPosition();

	while (pos != NULL)
	{
		CString command;
		WORD    wID;
		mgr.m_mapAccelString.GetNextAssoc(pos, command, wID);

		CString tempStr    = command;
		int     commonMark = tempStr.Find("(!)", 0);
		if (commonMark >= 0)
			tempStr.Truncate(commonMark); // cut off the (!)

		if (!m_commonOnly || commonMark >= 0)
		{
			int index = m_commands.AddString(tempStr);
			m_commands.SetItemData(index, wID);
		}
		else
		{
			int index = m_commands_hidden.AddString(tempStr);
			m_commands_hidden.SetItemData(index, wID);
		}
	}

	// Update the currents accels associated with the selected command
	if (m_commands.SetCurSel(0) != LB_ERR)
		OnSelchangeCommands();
}

void AccelEditor::OnCancel()
{
	EndDialog(FALSE);
}

void AccelEditor::OnOk()
{
	EndDialog(TRUE);
}

void AccelEditor::OnSelchangeCommands()
{
	// Check if some commands exist.
	int index = m_commands.GetCurSel();
	if (index == LB_ERR)
		return;

	WORD wIDCommand = LOWORD(m_commands.GetItemData(index));
	m_currents.ResetContent();

	CCmdAccelOb*pCmdAccel;

	if (mgr.m_mapAccelTable.Lookup(wIDCommand, pCmdAccel))
	{
		CAccelsOb*pAccel;
		CString   szBuffer;
		POSITION  pos = pCmdAccel->m_Accels.GetHeadPosition();

		// Add the keys to the 'currents keys' listbox.
		while (pos != NULL)
		{
			pAccel = pCmdAccel->m_Accels.GetNext(pos);
			pAccel->GetString(szBuffer);
			index = m_currents.AddString(szBuffer);
			// and a pointer to the accel object.
			m_currents.SetItemData(index, (DWORD)pAccel);
		}
	}
	// Init the key editor
	//  m_pKey->ResetKey();
}

void AccelEditor::OnReset()
{
	mgr.Default(); /// FIXME accelerator reset NYI
	systemMessage(
	    0,
	    "The \"Reset All Accelerators\" feature is currently unimplemented.\nYou can achieve the same result by closing VBA, opening up your \"vba.ini\" file, deleting the line that starts with \"keyboard\", then reopening VBA.");
	InitCommands(); // update the listboxes.
}

void AccelEditor::OnAssign()
{
	// Control if it's not already affected
	CCmdAccelOb*pCmdAccel;
	CAccelsOb*  pAccel;
	WORD        wIDCommand;
	POSITION    pos;

	WORD wKey;
	bool bCtrl, bAlt, bShift;

	if (!m_key.GetAccelKey(wKey, bCtrl, bAlt, bShift))
		return; // no valid key, abort

	// get the currently selected group
	int index = m_commands.GetCurSel();
	if (index == LB_ERR)
		return;

	// Get the object who manage the accels list, associated to the command.
	wIDCommand = LOWORD(m_commands.GetItemData(index));

	int count = m_commands.GetCount();
	for (int index2 = 0; index2 < count; index2++)
	{
		WORD wIDCommand2 = LOWORD(m_commands.GetItemData(index2));
		mgr.m_mapAccelTable.Lookup(wIDCommand2, pCmdAccel);

		pos = pCmdAccel->m_Accels.GetHeadPosition();
		while (pos != NULL)
		{
			pAccel = pCmdAccel->m_Accels.GetNext(pos);
			if (pAccel->IsEqual(wKey, bCtrl, bAlt, bShift))
			{
				// the key is already affected (in the same or other command)
				// (the parts that are commented out allow for a one-to-many mapping,
				//  which is only disabled because the MFC stuff that automagically activates the commands
				//  doesn't seem to be capable of activating more than one command per accelerator...)
//		if (wIDCommand != wIDCommand2) {
				// the key is already affected by a different command and also not already affected by the same commmand

				CString tempStr    = pCmdAccel->m_szCommand;
				int     commonMark = tempStr.Find("(!)", 0);
				if (commonMark >= 0)
					tempStr.Truncate(commonMark); // cut off the (!)

				m_alreadyAffected.SetWindowText(tempStr);
//		} else {
//		  // the key is already affected by the same command
				m_key.SetSel(0, -1);
				mgr.m_mapAccelTable.Lookup(wIDCommand, pCmdAccel);
//          m_alreadyAffected.SetWindowText(pCmdAccel->m_szCommand);
				return; // abort
//		}
			}
		}
	}

	// also check against any currently hidden commands
	int count2 = m_commands_hidden.GetCount();
	for (int index3 = 0; index3 < count2; index3++)
	{
		WORD wIDCommand2 = LOWORD(m_commands_hidden.GetItemData(index3));
		mgr.m_mapAccelTable.Lookup(wIDCommand2, pCmdAccel);

		pos = pCmdAccel->m_Accels.GetHeadPosition();
		while (pos != NULL)
		{
			pAccel = pCmdAccel->m_Accels.GetNext(pos);
			if (pAccel->IsEqual(wKey, bCtrl, bAlt, bShift))
			{
				// the key is already affected (in the same or other command)
				// (the parts that are commented out allow for a one-to-many mapping,
				//  which is only disabled because the MFC stuff that automagically activates the commands
				//  doesn't seem to be capable of activating more than one command per accelerator...)
//		if (wIDCommand != wIDCommand2) {
				// the key is already affected by a different command and also not already affected by the same commmand
				CString tempStr = pCmdAccel->m_szCommand; // store because pCmdAccel changes after this
				m_alreadyAffected.SetWindowText(tempStr);
//		} else {
//		  // the key is already affected by the same command
				m_key.SetSel(0, -1);
				mgr.m_mapAccelTable.Lookup(wIDCommand, pCmdAccel);
//          m_alreadyAffected.SetWindowText(pCmdAccel->m_szCommand);

				// unhide the conflicting command
				// note that it will also stay in the hidden list, but that doesn't matter
				int newIndex = m_commands.AddString(tempStr);
				m_commands.SetItemData(newIndex, wIDCommand2);

				return; // abort
//		}
			}
		}
	}

	if (mgr.m_mapAccelTable.Lookup(wIDCommand, pCmdAccel) != TRUE)
		return;

	BYTE cVirt = 0;
	if (bCtrl)
		cVirt |= FCONTROL;
	if (bAlt)
		cVirt |= FALT;
	if (bShift)
		cVirt |= FSHIFT;

	cVirt |= FVIRTKEY;

	// Create the new key...
	pAccel = new CAccelsOb(cVirt, wKey, false);
	ASSERT(pAccel != NULL);
	// ...and add in the list.
	pCmdAccel->m_Accels.AddTail(pAccel);

	// Update the listbox.
	CString szBuffer;
	pAccel->GetString(szBuffer);

	index = m_currents.AddString(szBuffer);
	m_currents.SetItemData(index, (DWORD)pAccel);

	// Reset the key editor.
	m_key.ResetKey();
}

void AccelEditor::OnRemove()
{
	// Some controls
	int indexCurrent = m_currents.GetCurSel();
	if (indexCurrent == LB_ERR)
		return;

	// 2nd part.
	int indexCmd = m_commands.GetCurSel();
	if (indexCmd == LB_ERR)
		return;

	// Ref to the ID command
	WORD wIDCommand = LOWORD(m_commands.GetItemData(indexCmd));

	// Run through the accels,and control if it can be deleted.
	CCmdAccelOb*pCmdAccel;
	if (mgr.m_mapAccelTable.Lookup(wIDCommand, pCmdAccel) == TRUE)
	{
		CAccelsOb*pAccel;
		CAccelsOb*pAccelCurrent = (CAccelsOb *)(m_currents.GetItemData(indexCurrent));
		CString   szBuffer;
		POSITION  pos = pCmdAccel->m_Accels.GetHeadPosition();
		POSITION  PrevPos;
		while (pos != NULL)
		{
			PrevPos = pos;
			pAccel  = pCmdAccel->m_Accels.GetNext(pos);
			if (pAccel == pAccelCurrent)
			{
				if (!pAccel->m_bLocked)
				{
					// not locked, so we delete the key
					pCmdAccel->m_Accels.RemoveAt(PrevPos);
					delete pAccel;
					// and update the listboxes/key editor/static text
					m_currents.DeleteString(indexCurrent);
					m_key.ResetKey();
					m_alreadyAffected.SetWindowText("");
					return;
				}
				else
				{
					systemMessage(0, "Unable to remove this\naccelerator (Locked)");
					return;
				}
			}
		}
		systemMessage(0, "internal error (CAccelDlgHelper::Remove : pAccel unavailable)");
		return;
	}
	systemMessage(0, "internal error (CAccelDlgHelper::Remove : Lookup failed)");
}

