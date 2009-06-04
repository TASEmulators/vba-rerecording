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

#if !defined(AFX_DIRECTORIES_H__7ADB14C1_3C1B_4294_8D66_A4E87D6FC731__INCLUDED_)
#define AFX_DIRECTORIES_H__7ADB14C1_3C1B_4294_8D66_A4E87D6FC731__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Directories.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// Directories dialog

class Directories : public CDialog
{
  // Construction
 public:
  CString initialFolderDir;
  CString browseForDir(CString title);
  Directories(CWnd* pParent = NULL);   // standard constructor

  // Dialog Data
  //{{AFX_DATA(Directories)
  enum { IDD = IDD_DIRECTORIES };
  CEdit  m_romPath;
  CEdit  m_gbxromPath;
  CEdit  m_batteryPath;
  CEdit  m_savePath;
  CEdit  m_moviePath;
  CEdit  m_cheatPath;
  CEdit  m_ipsPath;
  CEdit  m_luaPath;
//  CEdit  m_watchPath;
//  CEdit  m_pluginPath;
  CEdit  m_aviPath;
  CEdit  m_wavPath;
  CEdit  m_capturePath;
  //}}AFX_DATA


  // Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(Directories)
 protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

  // Implementation
 protected:

  // Generated message map functions
  //{{AFX_MSG(Directories)
  virtual BOOL OnInitDialog();
  afx_msg void OnRomDir();
  afx_msg void OnRomDirReset();
  afx_msg void OnGBxRomDir();
  afx_msg void OnGBxRomDirReset();
  afx_msg void OnBatteryDir();
  afx_msg void OnBatteryDirReset();
  afx_msg void OnSaveDir();
  afx_msg void OnSaveDirReset();
  afx_msg void OnMovieDir();
  afx_msg void OnMovieDirReset();
  afx_msg void OnCheatDir();
  afx_msg void OnCheatDirReset();
  afx_msg void OnIpsDir();
  afx_msg void OnIpsDirReset();
  afx_msg void OnLuaDir();
  afx_msg void OnLuaDirReset();
  afx_msg void OnAviDir();
  afx_msg void OnAviDirReset();
  afx_msg void OnWavDir();
  afx_msg void OnWavDirReset();
  afx_msg void OnCaptureDir();
  afx_msg void OnCaptureDirReset();
  virtual void OnCancel();
  virtual void OnOK();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
    };

    //{{AFX_INSERT_LOCATION}}
    // Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIRECTORIES_H__7ADB14C1_3C1B_4294_8D66_A4E87D6FC731__INCLUDED_)
