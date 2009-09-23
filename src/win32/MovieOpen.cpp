// MovieOpen.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MovieOpen.h"
#include "MainWnd.h"
#include "FileDlg.h"
#include "WinResUtil.h"
#include "VBA.h"

#include "../gba/GBA.h"
#include "../gb/gbGlobals.h"
#include "../common/Util.h"
#include "7zip/OpenArchive.h"

// MovieOpen dialog

IMPLEMENT_DYNAMIC(MovieOpen, CDialog)
MovieOpen::MovieOpen(CWnd*pParent /*=NULL*/)
	: CDialog(MovieOpen::IDD, pParent)
{
}

MovieOpen::~MovieOpen()
{
	SetArchiveParentHWND(NULL);
}

BOOL MovieOpen::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetArchiveParentHWND(GetSafeHwnd());

	GetDlgItem(IDC_CHECK_HIDEBORDER)->ShowWindow(FALSE);
	GetDlgItem(IDC_LABEL_WARNING1)->SetWindowText("");
	GetDlgItem(IDC_LABEL_WARNING2)->SetWindowText("");
	GetDlgItem(IDC_EDIT_PAUSEFRAME)->SetWindowText("");
	GetDlgItem(IDC_EDIT_PAUSEFRAME)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_PAUSEFRAME)->EnableWindow(FALSE);

	CheckDlgButton(IDC_READONLY, theApp.movieReadOnly);
	m_editDescription.SetReadOnly(theApp.movieReadOnly);

	m_editFilename.LimitText(_MAX_PATH);
	m_editAuthor.LimitText(MOVIE_METADATA_AUTHOR_SIZE);
	m_editDescription.LimitText(MOVIE_METADATA_SIZE - MOVIE_METADATA_AUTHOR_SIZE);
	m_editPauseFrame.LimitText(8);

	// convert the ROM filename into a default movie name
	CString movieName = ((MainWnd *)theApp.m_pMainWnd)->getRelatedFilename(theApp.filename, IDS_MOVIE_DIR, ".vbm");

	GetDlgItem(IDC_MOVIE_FILENAME)->SetWindowText(movieName);

	// scroll to show the rightmost side of the movie filename
	((CEdit*)GetDlgItem(IDC_MOVIE_FILENAME))->SetSel((DWORD)(movieName.GetLength()-1), FALSE);

	OnBnClickedMovieRefresh();

	return TRUE; // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void MovieOpen::DoDataExchange(CDataExchange*pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MovieCreate)
	DDX_Control(pDX, IDC_EDIT_AUTHOR, m_editAuthor);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_editDescription);
	DDX_Control(pDX, IDC_MOVIE_FILENAME, m_editFilename);
	DDX_Control(pDX, IDC_EDIT_PAUSEFRAME, m_editPauseFrame);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(MovieOpen, CDialog)
ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
ON_BN_CLICKED(IDOK, OnBnClickedOk)
ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
ON_BN_CLICKED(IDC_MOVIE_REFRESH, OnBnClickedMovieRefresh)
ON_BN_CLICKED(IDC_READONLY, OnBnClickedReadonly)
ON_BN_CLICKED(IDC_CHECK_PAUSEFRAME, OnBnClickedCheckPauseframe)
ON_BN_CLICKED(IDC_CHECK_HIDEBORDER, OnBnClickedHideborder)
ON_EN_CHANGE(IDC_MOVIE_FILENAME, OnEnChangeMovieFilename)
END_MESSAGE_MAP()

// MovieOpen message handlers

// FIXME: file-scope-global
static bool shouldReopenBrowse = false;

void MovieOpen::OnBnClickedBrowse()
{
	theApp.winCheckFullscreen();	// FIXME: necessary or not?

	LPCTSTR exts[] = { ".vbm", NULL };

	CString filter = theApp.winLoadFilter(IDS_FILTER_MOVIE);
	CString title  = winResLoadString(IDS_SELECT_MOVIE_NAME);

	CString movieName = ((MainWnd *)theApp.m_pMainWnd)->getRelatedFilename(theApp.filename, IDS_MOVIE_DIR, exts[0]);
	CString movieDir = ((MainWnd *)theApp.m_pMainWnd)->getRelatedDir(IDS_MOVIE_DIR);

	FileDlg dlg(this, movieName, filter, 1, "VBM", exts, movieDir, title, false, true);

	do
	{
		shouldReopenBrowse = false;

		if (dlg.DoModal() == IDCANCEL)
		{
			return;
		}

		movieName = dlg.GetPathName();

		GetDlgItem(IDC_MOVIE_FILENAME)->SetWindowText(movieName);

		// SetWindowText calls OnEnChangeMovieFilename which calls OnBnClickedMovieRefresh
		// so this extra call to OnBnClickedMovieRefresh is bad
		//OnBnClickedMovieRefresh();

	} while(shouldReopenBrowse);

	// scroll to show the rightmost side of the movie filename
	((CEdit*)GetDlgItem(IDC_MOVIE_FILENAME))->SetSel((DWORD)(movieName.GetLength()-1), FALSE);
}

// returns the checksum of the BIOS that will be loaded after the next restart
u16 checksumBIOS()
{
	bool hasBIOS = false;
	u8 * tempBIOS;
	if (theApp.useBiosFile)
	{
		tempBIOS = (u8 *)malloc(0x4000);
		int         size = 0x4000;
		if (utilLoad(theApp.biosFileName,
		             utilIsGBABios,
		             tempBIOS,
		             size))
		{
			if (size == 0x4000)
				hasBIOS = true;
		}
	}

	u16 biosCheck = 0;
	if (hasBIOS)
	{
		for (int i = 0; i < 0x4000; i += 4)
			biosCheck += *((u32 *)&tempBIOS[i]);
		free(tempBIOS);
	}

	return biosCheck;
}

void fillRomInfo(const SMovie & movieInfo, char romTitle [12], uint32 & romGameCode, uint16 & checksum, uint8 & crc)
{
	if (theApp.cartridgeType == 0) // GBA
	{
		extern u8 *bios, *rom;
		memcpy(romTitle, (const char *)&rom[0xa0], 12); // GBA TITLE
		memcpy(&romGameCode, &rom[0xac], 4); // GBA ROM GAME CODE

		bool prevUseBiosFile = theApp.useBiosFile;
		theApp.useBiosFile = (movieInfo.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) != 0;
		checksum = checksumBIOS(); // GBA BIOS CHECKSUM
		theApp.useBiosFile = prevUseBiosFile;
		crc = rom[0xbd]; // GBA ROM CRC
	}
	else // non-GBA
	{
		extern u8 *gbRom;
		memcpy(romTitle, (const char *)&gbRom[0x134], 12); // GB TITLE (note this can be 15 but is truncated to 12)
		romGameCode = (uint32)gbRom[0x146]; // GB ROM UNIT CODE

		checksum = (gbRom[0x14e]<<8)|gbRom[0x14f]; // GB ROM CHECKSUM
		crc      = gbRom[0x14d]; // GB ROM CRC
	}
}

// some extensions that might commonly be near emulation-related files that we almost certainly can't open, or at least not directly.
// also includes definitely non-movie extensions we know about, since we only use this variable in a movie opening function.
// we do this by exclusion instead of inclusion because we don't want to exclude extensions used for any archive files, even extensionless or unusually-named archives.
static const char* s_movieIgnoreExtensions [] = {"gba", "gbc", "gb", "sgb", "cgb", "bin", "agb", "bios", "mb", "elf", "sgm", "clt", "dat", "gbs", "gcf", "spc", "xpc", "pal", "act", "dmp", "avi", "ini",
	"txt", "nfo", "htm", "html", "jpg", "jpeg", "png", "bmp", "gif", "mp3", "wav", "lnk", "exe", "bat", "luasav", "sav"};

void MovieOpen::OnBnClickedMovieRefresh()
{
	static int recursionDepth = 0;
	if(recursionDepth > 0)
		return;
	struct Scope {Scope(){++recursionDepth;} ~Scope(){--recursionDepth;}} scope;

	CString tempName;
	GetDlgItem(IDC_MOVIE_FILENAME)->GetWindowText(tempName);

#if 1
	// use ObtainFile to support opening files within archives (.7z, .rar, .zip, .zip.rar.7z, etc.)

	if(movieLogicalName.GetLength() > 2048) movieLogicalName.Truncate(2048);

	char LogicalName[2048], PhysicalName[2048];
	if(ObtainFile(tempName, LogicalName, PhysicalName, "mov", s_movieIgnoreExtensions, sizeof(s_movieIgnoreExtensions)/sizeof(*s_movieIgnoreExtensions)))
	{
		if(tempName != LogicalName)
		{
			int selStart=0, selEnd=0;
			((CEdit*)GetDlgItem(IDC_MOVIE_FILENAME))->GetSel(selStart, selEnd);

			GetDlgItem(IDC_MOVIE_FILENAME)->SetWindowText(LogicalName);

			((CEdit*)GetDlgItem(IDC_MOVIE_FILENAME))->SetSel(selStart, selEnd, FALSE);
		}
		moviePhysicalName = PhysicalName;
		movieLogicalName = LogicalName;
		ReleaseTempFileCategory("mov", PhysicalName);
	}
	else
	{
		shouldReopenBrowse = true;
		return;
	}
#else
	// old version that only supports uncompressed movies
	moviePhysicalName = tempName;
	movieLogicalName = tempName;
#endif


	if (VBAMovieGetInfo(moviePhysicalName, &movieInfo) == SUCCESS)
	{
		if (movieInfo.readOnly)
		{
			CheckDlgButton(IDC_READONLY, TRUE);
			m_editDescription.SetReadOnly(TRUE);
		}

		char buffer [MOVIE_METADATA_SIZE];

		strncpy(buffer, movieInfo.authorInfo, MOVIE_METADATA_AUTHOR_SIZE);
		buffer[MOVIE_METADATA_AUTHOR_SIZE - 1] = '\0';
		GetDlgItem(IDC_EDIT_AUTHOR)->SetWindowText(buffer);

		strncpy(buffer, movieInfo.authorInfo + MOVIE_METADATA_AUTHOR_SIZE, MOVIE_METADATA_SIZE - MOVIE_METADATA_AUTHOR_SIZE);
		buffer[MOVIE_METADATA_SIZE - MOVIE_METADATA_AUTHOR_SIZE - 1] = '\0';
		GetDlgItem(IDC_EDIT_DESCRIPTION)->SetWindowText(buffer);

		int option = 2;
		if (movieInfo.header.startFlags & MOVIE_START_FROM_SRAM)
			option = 1;
		if (movieInfo.header.startFlags & MOVIE_START_FROM_SNAPSHOT)
			option = 0;
		CheckRadioButton(IDC_RECNOW, IDC_RECSTART, IDC_RECNOW + option);

		option = 3;
		if (movieInfo.header.typeFlags & MOVIE_TYPE_SGB)
			option = 2;
		if (movieInfo.header.typeFlags & MOVIE_TYPE_GBC)
			option = 1;
		if (movieInfo.header.typeFlags & MOVIE_TYPE_GBA)
			option = 0;
		CheckRadioButton(IDC_REC_GBA, IDC_REC_GB, IDC_REC_GBA + option);

		GetDlgItem(IDC_CHECK_HIDEBORDER)->ShowWindow(option == 2 ? TRUE : FALSE);

		if (movieInfo.header.typeFlags & MOVIE_TYPE_GBA)
		{
			if (movieInfo.header.optionFlags & MOVIE_SETTING_USEBIOSFILE)
			{
				if (movieInfo.header.optionFlags & MOVIE_SETTING_SKIPBIOSFILE)
					option = 2;
				else
					option = 3;
			}
			else
				option = 1;
		}
		else
			option = 0;

		CheckRadioButton(IDC_REC_NOBIOS, IDC_REC_GBABIOSINTRO, IDC_REC_NOBIOS + option);

		{
			char*  p;
			time_t ttime = (time_t)movieInfo.header.uid;
			strncpy(buffer, ctime(&ttime), 127);
			buffer[127] = '\0';
			if ((p = strrchr(buffer, '\n')))
				*p = '\0';
			GetDlgItem(IDC_LABEL_DATE)->SetWindowText(buffer);

			uint32 div     = 60;
			uint32 l       = (movieInfo.header.length_frames+(div>>1))/div;
			uint32 seconds = l%60;
			l /= 60;
			uint32 minutes = l%60;
			l /= 60;
			uint32 hours = l%60;
			sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
			GetDlgItem(IDC_LABEL_LENGTH)->SetWindowText(buffer);
			sprintf(buffer, "%ld", movieInfo.header.length_frames);
			GetDlgItem(IDC_LABEL_FRAMES)->SetWindowText(buffer);
			sprintf(buffer, "%ld", movieInfo.header.rerecord_count);
			GetDlgItem(IDC_LABEL_RERECORD)->SetWindowText(buffer);
		}

		{
			char warning1 [1024], warning2 [1024], buffer [1024];

			strcpy(warning1, "");
			strcpy(warning2, "");

			char   romTitle [12];
			uint32 romGameCode;
			uint16 checksum;
			uint8  crc;

			fillRomInfo(movieInfo, romTitle, romGameCode, checksum, crc);

			// rather than treat these as warnings, might as well always show the info in the dialog (it's probably more
			// informative and reassuring)
///			if(strncmp(movieInfo.header.romTitle,romTitle,12) != 0)
			{
				char str [13];
				strncpy(str, movieInfo.header.romTitle, 12);
				str[12] = '\0';
				sprintf(buffer, "title=%s  ", str);
				strcat(warning1, buffer);

				strncpy(str, romTitle, 12);
				str[12] = '\0';
				sprintf(buffer, "title=%s  ", str);
				strcat(warning2, buffer);
			}
///			if(((movieInfo.header.typeFlags & MOVIE_TYPE_GBA)!=0) != (theApp.cartridgeType == 0))
			{
				sprintf(buffer, "type=%s  ",
				        (movieInfo.header.typeFlags&MOVIE_TYPE_GBA) ? "GBA" : (movieInfo.header.typeFlags&
				                                                               MOVIE_TYPE_GBC) ? "GBC" : (movieInfo.header.
				                                                                                          typeFlags&
				                                                                                          MOVIE_TYPE_SGB) ? "SGB" : "GB");
				strcat(warning1, buffer);

				sprintf(buffer, "type=%s  ", theApp.cartridgeType ==
				        0 ? "GBA" : (gbRom[0x143] & 0x80 ? "GBC" : (gbRom[0x146] == 0x03 ? "SGB" : "GB")));
				strcat(warning2, buffer);
			}
///			if(movieInfo.header.romCRC != crc)
			{
				sprintf(buffer, "crc=%02d  ", movieInfo.header.romCRC);
				strcat(warning1, buffer);

				sprintf(buffer, "crc=%02d  ", crc);
				strcat(warning2, buffer);
			}
///			if(movieInfo.header.romGameCode != romGameCode)
			{
				char code [5];
				if (movieInfo.header.typeFlags&MOVIE_TYPE_GBA)
				{
					memcpy(code, &movieInfo.header.romGameCode, 4);
					code[4] = '\0';
					sprintf(buffer, "code=%s  ", code);
					strcat(warning1, buffer);
				}

				if (theApp.cartridgeType == 0)
				{
					memcpy(code, &romGameCode, 4);
					code[4] = '\0';
					sprintf(buffer, "code=%s  ", code);
					strcat(warning2, buffer);
				}
			}
///			if(movieInfo.header.romOrBiosChecksum != checksum && !((movieInfo.header.optionFlags & MOVIE_SETTING_USEBIOSFILE)==0
// && checksum==0))
			{
				sprintf(buffer,
				        movieInfo.header.typeFlags&
				        MOVIE_TYPE_GBA ? ((movieInfo.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) ==
				                          0 ? "(bios=none)  " :  "(bios=%d)  ") : "check=%d  ",
				        movieInfo.header.romOrBiosChecksum);
				strcat(warning1, buffer);

				sprintf(buffer,
				        checksum == 0 ? "(bios=none)  " : theApp.cartridgeType == 0 ? "(bios=%d)  " : "check=%d  ",
				        checksum);
				strcat(warning2, buffer);
			}

			if (strlen(warning1) > 0)
			{
				sprintf(buffer, "Movie ROM: %s", warning1);
				GetDlgItem(IDC_LABEL_WARNING1)->SetWindowText(buffer);

				sprintf(buffer, "Your ROM: %s", warning2);

				//if(movieInfo.header.romCRC != crc
				//|| strncmp(movieInfo.header.romTitle,romTitle,12) != 0
				//|| movieInfo.header.romOrBiosChecksum != checksum && !((movieInfo.header.optionFlags &
				// MOVIE_SETTING_USEBIOSFILE)==0 && checksum==0))
				//	strcat(buffer, "<-- MISMATCH");

				GetDlgItem(IDC_LABEL_WARNING2)->SetWindowText(buffer);
			}
			else
			{
				GetDlgItem(IDC_LABEL_WARNING1)->SetWindowText("(No problems detected)");
				GetDlgItem(IDC_LABEL_WARNING2)->SetWindowText(" ");
			}
		}
		GetDlgItem(IDC_CHECK_PAUSEFRAME)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_LABEL_DATE)->SetWindowText(" ");
		GetDlgItem(IDC_LABEL_LENGTH)->SetWindowText(" ");
		GetDlgItem(IDC_LABEL_FRAMES)->SetWindowText(" ");
		GetDlgItem(IDC_LABEL_RERECORD)->SetWindowText(" ");
		GetDlgItem(IDC_EDIT_AUTHOR)->SetWindowText(" ");
		GetDlgItem(IDC_EDIT_DESCRIPTION)->SetWindowText(" ");
		GetDlgItem(IDC_LABEL_WARNING1)->SetWindowText(" ");
		GetDlgItem(IDC_LABEL_WARNING2)->SetWindowText(" ");

		GetDlgItem(IDC_EDIT_PAUSEFRAME)->SetWindowText("");
		GetDlgItem(IDC_EDIT_PAUSEFRAME)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_PAUSEFRAME)->EnableWindow(FALSE);
		CheckDlgButton(IDC_CHECK_PAUSEFRAME, FALSE);
/*
        /// FIXME: how to un-check all the radio buttons?
        CheckRadioButton(IDC_RECNOW, IDC_RECSTART, -1);
        CheckRadioButton(IDC_REC_GBA, IDC_REC_GB, -1);
        CheckRadioButton(IDC_REC_NOBIOS, IDC_REC_GBABIOSINTRO, -1);*/
	}
}

BOOL MovieOpen::OnWndMsg(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *res)
{
	switch (msg)
	{
	case WM_CTLCOLORSTATIC:
	{
		//HWND hwndDlg = GetSafeHwnd();
		HWND warnDlg = NULL;
		GetDlgItem(IDC_LABEL_WARNING2, &warnDlg);

		if ((HWND)lParam == warnDlg)
		{
			char   romTitle [12];
			uint32 romGameCode;
			uint16 checksum;
			uint8  crc;

			fillRomInfo(movieInfo, romTitle, romGameCode, checksum, crc);

			if (movieInfo.header.romCRC != crc
			    || strncmp(movieInfo.header.romTitle, romTitle, 12) != 0
			    || movieInfo.header.romOrBiosChecksum != checksum
			    && !((movieInfo.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) == 0 && checksum == 0))
			{
				// draw the md5 sum in red if it's different from the md5 of the rom used in the replay
				HDC hdcStatic = (HDC)wParam;
				SetTextColor(hdcStatic, RGB(255, 0, 0));      // use red for a mismatch

				// I'm not sure why this doesn't work to make the background transparent... it turns white anyway
				SetBkMode(hdcStatic, TRANSPARENT);
				return (LONG)GetStockObject(NULL_BRUSH);
			}
		}
		return FALSE;
	}
	}
	return CDialog::OnWndMsg(msg, wParam, lParam, res);
}

void MovieOpen::OnBnClickedOk()
{
	theApp.useBiosFile = (movieInfo.header.optionFlags & MOVIE_SETTING_USEBIOSFILE) != 0;
	if (theApp.useBiosFile)
	{
		extern bool checkBIOS(CString & biosFileName); // from MovieCreate.cpp
		if (!checkBIOS(theApp.biosFileName))
		{
			systemMessage(0, "This movie requires a valid GBA BIOS file to play.\nPlease locate a BIOS file.");
			((MainWnd *)theApp.m_pMainWnd)->OnOptionsEmulatorSelectbiosfile();
			if (!checkBIOS(theApp.biosFileName))
			{
				systemMessage(0, "\"%s\" is not a valid BIOS file; cannot play movie without one.", theApp.biosFileName);
				return;
			}
		}
	}
	extern void loadBIOS();
	loadBIOS();

	int code = VBAMovieOpen(moviePhysicalName, IsDlgButtonChecked(IDC_READONLY) != FALSE);

	if (code != SUCCESS)
	{
		if (code == FILE_NOT_FOUND)
			systemMessage(0, "Could not find movie file \"%s\".", (const char *)movieLogicalName);
		else if (code == WRONG_FORMAT)
			systemMessage(0, "Movie file \"%s\" is not in proper VBM format.", (const char *)movieLogicalName);
		else if (code == WRONG_VERSION)
			systemMessage(0, "Movie file \"%s\" is not a supported version.", (const char *)movieLogicalName);
		else
			systemMessage(0, "Failed to open movie \"%s\".", (const char *)movieLogicalName);
		return;
	}
	else
	{
		// get author and movie info from the edit fields (the description might change):
		char info [MOVIE_METADATA_SIZE], buffer [MOVIE_METADATA_SIZE];

		GetDlgItem(IDC_EDIT_AUTHOR)->GetWindowText(buffer, MOVIE_METADATA_AUTHOR_SIZE);
		strncpy(info, buffer, MOVIE_METADATA_AUTHOR_SIZE);
		info[MOVIE_METADATA_AUTHOR_SIZE-1] = '\0';

		GetDlgItem(IDC_EDIT_DESCRIPTION)->GetWindowText(buffer, MOVIE_METADATA_SIZE - MOVIE_METADATA_AUTHOR_SIZE);
		strncpy(info + MOVIE_METADATA_AUTHOR_SIZE, buffer, MOVIE_METADATA_SIZE - MOVIE_METADATA_AUTHOR_SIZE);
		info[MOVIE_METADATA_SIZE-1] = '\0';

		VBAMovieSetMetadata(info);
	}

	if (IsDlgButtonChecked(IDC_CHECK_PAUSEFRAME))
	{
		char buffer [9];
		GetDlgItem(IDC_EDIT_PAUSEFRAME)->GetWindowText(buffer, 8);
		buffer[8] = '\0';

		int pauseFrame = atoi(buffer);
		if (pauseFrame >= 0)
			VBAMovieSetPauseAt(pauseFrame);
	}

	OnOK();
}

void MovieOpen::OnBnClickedCancel()
{
	OnCancel();
}

void MovieOpen::OnBnClickedReadonly()
{
	theApp.movieReadOnly = IsDlgButtonChecked(IDC_READONLY) != 0;
	m_editDescription.SetReadOnly(theApp.movieReadOnly);
	OnBnClickedMovieRefresh();
}

void MovieOpen::OnBnClickedCheckPauseframe()
{
	if (IsDlgButtonChecked(IDC_CHECK_PAUSEFRAME))
	{
		char buffer [8];
		_itoa(movieInfo.header.length_frames, buffer, 10);
		GetDlgItem(IDC_EDIT_PAUSEFRAME)->SetWindowText(buffer);
		GetDlgItem(IDC_EDIT_PAUSEFRAME)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_EDIT_PAUSEFRAME)->SetWindowText("");
		GetDlgItem(IDC_EDIT_PAUSEFRAME)->EnableWindow(FALSE);
	}
}

void MovieOpen::OnBnClickedHideborder()
{
	theApp.hideMovieBorder = IsDlgButtonChecked(IDC_CHECK_HIDEBORDER) != 0;
}

void MovieOpen::OnEnChangeMovieFilename()
{
	OnBnClickedMovieRefresh();
}

