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

// RewindInterval.cpp : implementation file
//

#include "stdafx.h"
#include <cmath>
#include "resource.h"
#include "RewindInterval.h"
#include "VBA.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// RewindInterval dialog

RewindInterval::RewindInterval(float interval, int slots, CWnd*pParent /*=NULL*/)
	: CDialog(RewindInterval::IDD, pParent)
{
	//{{AFX_DATA_INIT(RewindInterval)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	this->interval = interval;
	this->slots    = slots;
}

void RewindInterval::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RewindInterval)
	DDX_Control(pDX, IDC_INTERVAL, m_interval);
	DDX_Control(pDX, IDC_REWINDSLOTS, m_slots);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(RewindInterval, CDialog)
//{{AFX_MSG_MAP(RewindInterval)
ON_BN_CLICKED(ID_CANCEL, OnCancel)
ON_BN_CLICKED(ID_OK, OnOk)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// RewindInterval message handlers

void RewindInterval::OnCancel()
{
	EndDialog(-1);
}

void RewindInterval::OnOk()
{
	CString buffer, buffer2;

	m_interval.GetWindowText(buffer);
	m_slots.GetWindowText(buffer2);

	float interval = (float)atof(buffer);
	int   slots    = atoi(buffer2);

	if (interval >= 0 && (int)interval <= 600)
	{
		if (slots >= 0 && slots <= MAX_REWIND_SLOTS)
		{
			int iInterval = (int)(interval*6.0f + 0.5f);
			if (interval > 0 && iInterval == 0)
				iInterval = 1;
			EndDialog(iInterval | (slots << 16));
			theApp.winAccelMgr.UpdateMenu(theApp.menu);
		}
		else
			systemMessage(IDS_INVALID_INTERVAL_VALUE,
			              "Invalid rewind slot amount. Please enter a number "
			              "between 0 and 128 slots");
	}
	else
		systemMessage(IDS_INVALID_INTERVAL_VALUE,
		              "Invalid rewind interval value. Please enter a number "
		              "between 0 and 600 seconds");
}

BOOL RewindInterval::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_interval.LimitText(5);
	m_slots.LimitText(3);

	CString buffer, buffer2;
	buffer.Format("%.1f", interval);
	m_interval.SetWindowText(buffer);
	buffer2.Format("%d", slots);
	m_slots.SetWindowText(buffer2);

	CenterWindow();

	return TRUE; // return TRUE unless you set the focus to a control
	             // EXCEPTION: OCX Property Pages should return FALSE
}

