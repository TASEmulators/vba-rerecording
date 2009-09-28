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

#ifndef VBA_WIN32_AVIWRITE_H
#define VBA_WIN32_AVIWRITE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vfw.h>

class AVIWrite {
 public:
  AVIWrite();
  virtual ~AVIWrite();

  bool Open(const char *filename);
  virtual bool AddFrame(const u8 *bmp);
  void SetFPS(int fps);
  void SetVideoFormat(BITMAPINFOHEADER *);
  bool IsSoundAdded();
  void SetSoundFormat(WAVEFORMATEX *);
  bool AddSound(const u8 *sound, int len);
  int videoFrames();

 private:
  int m_fps;
  WAVEFORMATEX m_soundFormat;
  BITMAPINFOHEADER m_bitmap;
  AVISTREAMINFO m_header;
  AVISTREAMINFO m_soundHeader;
  PAVIFILE m_file;
  PAVISTREAM m_stream;
  PAVISTREAM m_streamCompressed;
  PAVISTREAM m_streamSound;
  AVICOMPRESSOPTIONS m_options;
  AVICOMPRESSOPTIONS *m_arrayOptions[1];
  int m_videoFrames;
  int m_samplesSound;
  LONG m_totalBytes;
  bool m_failed;
  int m_segmentNumber;
  bool m_usePrevOptions;
  char m_aviFileName[MAX_PATH];
  char m_aviBaseName[MAX_PATH];
  char m_aviExtension[MAX_PATH];
  void CleanUp();
  bool NextSegment();
};

#endif // VBA_WIN32_AVIWRITE_H
