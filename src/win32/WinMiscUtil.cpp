#include "stdafx.h"
#include "WinMiscUtil.h"
#include "WinResUtil.h"
#include "resource.h"
#include "../NLS.h"
#include "VBA.h"
#include "Reg.h"
#include "../common/movie.h"

extern int emulating;
// these could be made VBA members, but  the VBA class is already oversized too much

bool winFileExists(const char *filename)
{
	FILE*f = fopen(filename, "rb");
	if (f)
	{
		fclose(f);
		return true;
	}
	return false;
}

bool winIsDriveRoot(const CString &file)
{
	if (file.GetLength() == 3)
	{
		if (file[1] == ':' && file[2] == '\\')
			return true;
	}
	return false;
}

CString winGetOriginalFilename(const CString &file)
{
	int index = file.Find('|');

	if (index != -1)
		return file.Left(index);
	else
		return file;
}

CString winGetDirFromFilename(const CString &file)
{
	CString temp  = winGetOriginalFilename(file);
	int		index = max(temp.ReverseFind('/'), temp.ReverseFind('\\'));
	if (index != -1)
	{
		temp = temp.Left(index);
		if (temp.GetLength() == 2 && temp[1] == ':')
			temp += "\\";
	}

	return temp;
}

CString winGetDestDir(const CString &TargetDirReg)
{
	CString targetDir = regQueryStringValue(TargetDirReg, NULL);

	if (targetDir.IsEmpty())
		targetDir = winGetDirFromFilename(theApp.filename);

	return targetDir;
}

CString winGetDestFilename(const CString &LogicalRomName, const CString &TargetDirReg, const CString &ext)
{
	if (LogicalRomName.GetLength() == 0)
		return CString();

	CString targetDir = winGetDestDir(TargetDirReg);
	if (!winIsDriveRoot(targetDir))
		targetDir += '\\';

	CString buffer = LogicalRomName;

	int index = max(buffer.ReverseFind('/'), max(buffer.ReverseFind('\\'), buffer.ReverseFind('|')));
	if (index != -1)
		buffer = buffer.Right(buffer.GetLength()-index-1);

	index = buffer.ReverseFind('.');
	if (index != -1)
		buffer = buffer.Left(index);

	CString filename;
	filename.Format("%s%s%s", targetDir, buffer, ext);
	bool fileExists = winFileExists(filename);

	// check for old style of naming, for better backward compatibility
	if (!fileExists || theApp.filenamePreference == 0)
	{
		index = LogicalRomName.Find('|');
		if (index != -1)
		{
			buffer = LogicalRomName.Left(index);
			index  = max(buffer.ReverseFind('/'), buffer.ReverseFind('\\'));

			int dotIndex = buffer.ReverseFind('.');
			if (dotIndex > index)
				buffer = buffer.Left(dotIndex);

			if (index != -1)
				buffer = buffer.Right(buffer.GetLength()-index-1);

			CString filename2;
			filename2.Format("%s%s%s", targetDir, buffer, ext);
			bool file2Exists = winFileExists(filename2);

			if ((file2Exists && !fileExists) || (theApp.filenamePreference == 0 && (file2Exists || !fileExists)))
				return filename2;
		}
	}

	return filename;
}

CString winGetSavestateFilename(const CString &LogicalRomName, int nID)
{
	CString ext;
	std::string fs;
	size_t startindex;
	size_t endindex;
	if (VBAMovieActive() & theApp.AsscWithSaveState)  {
		fs =VBAMovieGetFilename();	
		startindex = fs.find_last_of("/\\");
		if(startindex < fs.length())
			startindex++;
		else
			startindex = 0;
		endindex = fs.find_last_of(".");
		if(endindex < fs.length() && endindex > startindex)
			endindex;
		else
			endindex = fs.length();
		fs = fs.substr(startindex, endindex - startindex);
		ext.Format("-%s-%d.sgm", fs.c_str(), nID);
	}
	else {
		ext.Format("-%d.sgm", nID);
	}
	return winGetDestFilename(LogicalRomName, IDS_SAVE_DIR, ext);
}

CString winGetSavestateMenuString(const CString &LogicalRomName, int nID)
{
	CString str;
	if (theApp.showSlotTime)
	{
		CFileStatus status;
		if (emulating && CFile::GetStatus(winGetSavestateFilename(theApp.filename, nID), status))
		{
			str.Format("#&%d %s", nID, status.m_mtime.Format("%Y/%m/%d %H:%M:%S"));
		}
		else
		{
			str.Format("#&%d ----/--/-- --:--:--", nID);
		}
	}
	else
	{
		str.Format("Slot #&%d", nID);
	}

	return str;
}

void winCorrectPath(CString &path)
{
	CString tempStr;
	FILE *	tempFile;

	if (tempFile = fopen(path, "rb"))
	{
		fclose(tempFile);
	}
	else
	{
		for (int i = 0; i < 11; i++)
		{
			switch (i)
			{
			case 0:
			{
				char curDir[_MAX_PATH];
				GetCurrentDirectory(_MAX_PATH, curDir);
				curDir[_MAX_PATH-1] = '\0';
				tempStr = curDir;
			}   break;
			case 1:
				tempStr = regQueryStringValue(IDS_ROM_DIR, "."); break;
			case 2:
				tempStr = regQueryStringValue(IDS_GBXROM_DIR, "."); break;
			case 3:
				tempStr = regQueryStringValue(IDS_BATTERY_DIR, "."); break;
			case 4:
				tempStr = regQueryStringValue(IDS_SAVE_DIR, "."); break;
			case 5:
				tempStr = regQueryStringValue(IDS_MOVIE_DIR, "."); break;
			case 6:
				tempStr = regQueryStringValue(IDS_CHEAT_DIR, "."); break;
			case 7:
				tempStr = regQueryStringValue(IDS_IPS_DIR, "."); break;
			case 8:
				tempStr = regQueryStringValue(IDS_AVI_DIR, "."); break;
			case 9:
				tempStr = regQueryStringValue(IDS_WAV_DIR, "."); break;
			case 10:
				tempStr = regQueryStringValue(IDS_CAPTURE_DIR, "."); break;
			/*
			                // what do these do?
			                case 11: tempStr = "C:";
			                case 12: {
			                    tempStr = regQueryStringValue(IDS_ROM_DIR,".");
			                    char * slash = (char*)strrchr(tempStr, '\\'); // should use member func intstead
			                    if(slash)
			                        slash[0] = '\0';
			                }	break;
			 */
			default:
				break;
			}
			tempStr += "\\";
			tempStr += path;

			if (tempFile = fopen(tempStr, "rb"))
			{
				fclose(tempFile);
				path = tempStr;
				break;
			}
		}
	}
}

void winCorrectPath(char *path)
{
	CString pathCStr(path);
	winCorrectPath(pathCStr);
	strcpy(path, pathCStr);
}

// some file I/O

void winScreenCapture(int captureNumber)
{
	CString ext;
	if (theApp.captureFormat != 0)
		ext.Format("_%02d.bmp", captureNumber);
	else
		ext.Format("_%02d.png", captureNumber);

	CString captureName = winGetDestFilename(theApp.filename, IDS_CAPTURE_DIR, ext);

	if (theApp.captureFormat == 0)
		theApp.emulator.emuWritePNG(captureName);
	else
		theApp.emulator.emuWriteBMP(captureName);

	CString msg = winResLoadString(IDS_SCREEN_CAPTURE);
	systemScreenMessage(msg);
}

#include "GSACodeSelect.h"
#include "../gba/GBACheats.h"
#include "../gb/gbCheats.h"

bool winImportGSACodeFile(CString& fileName)
{
	FILE *f = fopen(fileName, "rb");

	if (f == NULL)
	{
		systemMessage(MSG_CANNOT_OPEN_FILE, "Cannot open file %s", fileName);
		return false;
	}

	u32 len;
	fread(&len, 1, 4, f);
	if (len != 14)
	{
		fclose(f);
		systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
		              fileName);
		return false;
	}
	char buffer[16];
	fread(buffer, 1, 14, f);
	buffer[14] = 0;
	if (memcmp(buffer, "SharkPortCODES", 14))
	{
		fclose(f);
		systemMessage(MSG_UNSUPPORTED_CODE_FILE, "Unsupported code file %s",
		              fileName);
		return false;
	}
	fseek(f, 0x1e, SEEK_SET);
	fread(&len, 1, 4, f);
	int game = 0;
	if (len > 1)
	{
		GSACodeSelect dlg(f);
		game = dlg.DoModal();
	}
	fclose(f);

	bool v3 = false;

	int index = fileName.ReverseFind('.');

	if (index != -1)
	{
		if (fileName.Right(3).CompareNoCase("XPC") == 0)
			v3 = true;
	}

	if (game != -1)
	{
		return cheatsImportGSACodeFile(fileName, game, v3);
	}

	return true;
}

void winLoadCheatList(const char *name)
{
	bool res = false;

	if (theApp.cartridgeType == 0)
		res = cheatsLoadCheatList(name);
	else
		res = gbCheatsLoadCheatList(name);

	if (res)
		systemScreenMessage(winResLoadString(IDS_LOADED_CHEATS));
}

void winSaveCheatList(const char *name)
{
	if (theApp.cartridgeType == 0)
		cheatsSaveCheatList(name);
	else
		gbCheatsSaveCheatList(name);
}

void winLoadCheatListDefault()
{
	CString cheatName = winGetDestFilename(theApp.filename, IDS_CHEAT_DIR, ".clt");

	winLoadCheatList(cheatName);
}

void winSaveCheatListDefault()
{
	CString cheatName = winGetDestFilename(theApp.filename, IDS_CHEAT_DIR, ".clt");

	winSaveCheatList(cheatName);
}

void winReadBatteryFile()
{
	CString batteryName = winGetDestFilename(theApp.filename, IDS_BATTERY_DIR, ".sav");

	bool res = false;

	if (theApp.emulator.emuReadBattery)
		res = theApp.emulator.emuReadBattery(batteryName);

	if (res)
		systemScreenMessage(winResLoadString(IDS_LOADED_BATTERY));
}

void winWriteBatteryFile()
{
	CString batteryName = winGetDestFilename(theApp.filename, IDS_BATTERY_DIR, ".sav");

	if (theApp.emulator.emuWriteBattery)
		theApp.emulator.emuWriteBattery(batteryName);
}

bool winReadSaveGame(const char *name)
{
	if (theApp.emulator.emuReadState)
		return theApp.emulator.emuReadState(name);
	return false;
}

bool winWriteSaveGame(const char *name)
{
	if (theApp.emulator.emuWriteState)
		return theApp.emulator.emuWriteState(name);
	return false;
}
