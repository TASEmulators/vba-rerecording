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
#include <afxres.h>
#include "resource.h"
#include "AcceleratorManager.h"

#ifndef CMapStringToWord
typedef CMap< CString, LPCSTR, WORD, WORD & > CMapStringToWord;
#endif

static CMapStringToWord winAccelStrings;
static bool initialized = false;

struct
{
	const char *command;
	WORD        id;
} winAccelCommands[] = {
	{	"Minimize",				ID_SYSTEM_MINIMIZE	},
	{	"Maximize",				ID_SYSTEM_MAXIMIZE	},
};

void winAccelAddCommandsFromTable(CAcceleratorManager &mgr)
{
	int count = sizeof(winAccelCommands)/sizeof(winAccelCommands[0]);

	for (int i = 0; i < count; i++)
	{
		if (!mgr.AddCommandAccel(winAccelCommands[i].id, winAccelCommands[i].command, false))
			mgr.CreateEntry(winAccelCommands[i].id, winAccelCommands[i].command);
	}
}

// recursive calls
void winAccelAddCommandsFromMenu(CAcceleratorManager &mgr, CMenu *pMenu, const CString &parentStr)
{
	UINT nIndexMax = pMenu->GetMenuItemCount();
	for (UINT nIndex = 0; nIndex < nIndexMax; ++nIndex)
	{
		UINT nID = pMenu->GetMenuItemID(nIndex);
		if (nID == 0)
			continue; // menu separator or invalid cmd - ignore it

		if (nID == (UINT)-1)
		{
			// possibly a submenu
			CMenu *pSubMenu = pMenu->GetSubMenu(nIndex);
			if (pSubMenu != NULL)
			{
				CString tempStr;
				pMenu->GetMenuString(nIndex, tempStr, MF_BYPOSITION);
				tempStr.Remove('&');
				winAccelAddCommandsFromMenu(mgr, pSubMenu, parentStr + '\\' + tempStr);
			}
		}
		else
		{
			// normal menu item
			// generate the strings
			CString command;
			pMenu->GetMenuString(nIndex, command, MF_BYPOSITION);
			int nPos = command.ReverseFind('\t');
			if (nPos != -1)
			{
				command.Delete(nPos, command.GetLength() - nPos);
			}
			command.Remove('&');
			command = parentStr + '\\' + command;
			if (!mgr.AddCommandAccel(nID, command, false))
			{
				mgr.CreateEntry(nID, command);
			}
		}
	}
}

