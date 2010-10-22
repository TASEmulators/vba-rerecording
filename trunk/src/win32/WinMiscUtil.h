#ifndef VBA_WIN32_WINMISCUTIL_H
#define VBA_WIN32_WINMISCUTIL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

extern bool winFileExists(const char *filename);
extern bool winIsDriveRoot(const CString &file);
extern CString winGetOriginalFilename(const CString &file);
extern CString winGetDirFromFilename(const CString &file);
extern CString winGetSavestateFilename(const CString &LogicalRomName, int nID);
extern CString winGetSavestateMenuString(const CString &LogicalRomName, int nID);
extern CString winGetDestDir(const CString &TargetDirReg);
extern CString winGetDestFilename(const CString &LogicalRomName, const CString &TargetDirReg, const CString &ext);
extern void winCorrectPath(CString &path);
extern void winCorrectPath(char *path);

void winScreenCapture(int captureNumber);
bool winImportGSACodeFile(CString& fileName);
void winLoadCheatList(const char *name);
void winSaveCheatList(const char *name);
void winLoadCheatListDefault();
void winSaveCheatListDefault();
void winReadBatteryFile();
void winWriteBatteryFile();
bool winReadSaveGame(const char *name);
bool winWriteSaveGame(const char *name);

#endif // VBA_WIN32_WINMISCUTIL_H
