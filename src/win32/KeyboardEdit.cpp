////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1998 by Thierry Maurel
// All rights reserved
//
// Distribute freely, except: don't remove my name from the source or
// documentation (don't take credit for my work), mark your changes (don't
// get me blamed for your possible bugs), don't alter or remove this
// notice.
// No warrantee of any kind, express or implied, is included with this
// software; use at your own risk, responsibility for damages (if any) to
// anyone resulting from the use of this software rests entirely with the
// user.
//
// Send bug reports, bug fixes, enhancements, requests, flames, etc., and
// I'll try to keep a version up to date.  I can be reached as follows:
//    tmaurel@caramail.com   (or tmaurel@hol.fr)
//
////////////////////////////////////////////////////////////////////////////////
// File    : KeyboardEdit.cpp
// Project : AccelsEditor
////////////////////////////////////////////////////////////////////////////////
// Version : 1.0                       * Authors : A.Lebatard + T.Maurel
// Date    : 17.08.98
//
// Remarks : implementation file
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "KeyboardEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern TCHAR *mapVirtKeysStringFromWORD(WORD wKey);

IMPLEMENT_DYNAMIC(CKeyboardEdit, CEdit)

/////////////////////////////////////////////////////////////////////////////
// CKeyboardEdit

CKeyboardEdit::CKeyboardEdit()
{
	m_bKeyDefined = false;
	ResetKey();
}

CKeyboardEdit::~CKeyboardEdit()
{
}

BEGIN_MESSAGE_MAP(CKeyboardEdit, CEdit)
//{{AFX_MSG_MAP(CKeyboardEdit)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#pragma warning( disable : 4706 )
/////////////////////////////////////////////////////////////////////////////
// CKeyboardEdit message handlers
BOOL CKeyboardEdit::PreTranslateMessage(MSG *pMsg)
{
	bool bPressed = (pMsg->message == WM_KEYDOWN) || (pMsg->message == WM_SYSKEYDOWN);
	if (bPressed || pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP)
	{
		bool bReset = false;
		WORD oldVirtKey = m_wVirtKey;
		if (bPressed && m_bKeyDefined && !((1 << 30) & pMsg->lParam))
		{
			ResetKey();
			bReset = true;
		}

		bool syncShift = true, syncCtrl = true, syncAlt = true;
		if (pMsg->wParam == VK_SHIFT && !m_bKeyDefined)
		{
			if (bPressed)
				m_bShiftPressed = bPressed;
			syncShift = false;
		}
		else if (pMsg->wParam == VK_CONTROL && !m_bKeyDefined)
		{
			if (bPressed)
				m_bCtrlPressed = bPressed;
			syncCtrl = false;
		}
		else if (pMsg->wParam == VK_MENU && !m_bKeyDefined)
		{
			if (bPressed)
				m_bAltPressed = bPressed;
			syncAlt = false;
		}
		else if (!m_bKeyDefined)
		{
			m_wVirtKey = (WORD)pMsg->wParam;
			if (bPressed)
				m_bKeyDefined = true;

			if (syncShift)
				m_bShiftPressed = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
			if (syncCtrl)
				m_bCtrlPressed = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
			if (syncAlt)
				m_bAltPressed = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);
		}
		
		if (bReset && m_wVirtKey != oldVirtKey)
		{
			if ((GetAsyncKeyState(oldVirtKey) & 0x8000))
			{
				if (m_wVirtKey == 0)
				{
					m_wVirtKey = oldVirtKey;
					m_bKeyDefined = true;
				}
				else
				{
					m_wJamKey = oldVirtKey;
				}
			}
		}

		DisplayKeyboardString();
		return TRUE;
	}

	return CEdit::PreTranslateMessage(pMsg);
}

#pragma warning( default : 4706 )

////////////////////////////////////////////////////////////////////////
//
void CKeyboardEdit::DisplayKeyboardString()
{
	CString strKbd;

	// modifiers
	if (m_bCtrlPressed)
		strKbd = "Ctrl";

	if (m_bAltPressed)
	{
		if (strKbd.GetLength() > 0)
			strKbd += '+';
		strKbd += "Alt";
	}
	if (m_bShiftPressed)
	{
		if (strKbd.GetLength() > 0)
			strKbd += '+';
		strKbd += "Shift";
	}
	// virtual key
	LPCTSTR szVirtKey = mapVirtKeysStringFromWORD(m_wVirtKey);
	if (szVirtKey != NULL)
	{
		if (strKbd.GetLength() > 0)
			strKbd += '+';
		strKbd += szVirtKey;
	}
	// jammed key
	LPCTSTR szJamKey = mapVirtKeysStringFromWORD(m_wJamKey);
	if (szJamKey != NULL)
	{
		strKbd += '(';
		strKbd += szJamKey;
		strKbd += ')';
	}

	CString oldString;
	GetWindowText(oldString);
	if (oldString.Compare(strKbd))
		SetWindowText(strKbd);
}

////////////////////////////////////////////////////////////////////////
//
void CKeyboardEdit::ResetKey()
{
	m_bCtrlPressed	= false;
	m_bAltPressed	= false;
	m_bShiftPressed = false;
	m_bKeyDefined	= false;
	m_wVirtKey		= 0;
	m_wJamKey		= 0;

	if (m_hWnd != NULL)
		SetWindowText(_T(""));
}

////////////////////////////////////////////////////////////////////////
//
bool CKeyboardEdit::GetAccelKey(WORD &wVirtKey, bool &bCtrl, bool &bAlt, bool &bShift) const
{
	if (!m_bKeyDefined)
	{
		if (!m_bAltPressed && !m_bCtrlPressed && !m_bShiftPressed)
			return false;
	}

	wVirtKey = m_wVirtKey;
	bAlt	 = m_bAltPressed;
	bCtrl	 = m_bCtrlPressed;
	bShift	 = m_bShiftPressed;
	return true;
}

////////////////////////////////////////////////////////////////////////
//
bool CKeyboardEdit::GetJamKey(WORD &wJamKey) const
{
	if (m_wJamKey != 0)
		wJamKey = m_wJamKey;
	return m_wJamKey != 0;
}
