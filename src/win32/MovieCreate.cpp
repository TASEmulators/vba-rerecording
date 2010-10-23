// MovieCreate.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MovieCreate.h"
#include "MainWnd.h"
#include "FileDlg.h"
#include "WinResUtil.h"
#include "WinMiscUtil.h"
#include "VBA.h"

#include "../NLS.h"
#include "../common/Util.h"
#include "../common/movie.h"
#include "../gba/GBAGlobals.h"
#include "../gb/gbGlobals.h"

extern u32 myROM[];

#define GBC_CAPABLE ((gbRom[0x143] & 0x80) != 0)
#define SGB_CAPABLE (gbRom[0x146] == 0x03)

// MovieCreate dialog

IMPLEMENT_DYNAMIC(MovieCreate, CDialog)
MovieCreate::MovieCreate(CWnd *pParent /*=NULL*/)
	: CDialog(MovieCreate::IDD, pParent)
{
	//{{AFX_DATA_INIT(MovieCreate)
	m_startOption  = 2; // "from start" as default
	m_systemOption = theApp.cartridgeType == 0 ? 0 : (GBC_CAPABLE ? 1 : (SGB_CAPABLE ? 2 : 3)); // GBA, GBC, SGB, or GB
	m_biosOption   = theApp.cartridgeType == 0 ? (useBios ? 2 : 1) : 0; // none for non-GBA, or introless and based on settings
	//}}AFX_DATA_INIT
}

MovieCreate::~MovieCreate()
{}

BOOL MovieCreate::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_REC_GBA)->EnableWindow(theApp.cartridgeType == 0);
	GetDlgItem(IDC_REC_GBC)->EnableWindow(theApp.cartridgeType != 0 && GBC_CAPABLE);
	GetDlgItem(IDC_REC_SGB)->EnableWindow(theApp.cartridgeType != 0 && SGB_CAPABLE);
	GetDlgItem(IDC_REC_GB)->EnableWindow(theApp.cartridgeType != 0);

	GetDlgItem(IDC_REC_NOBIOS)->EnableWindow(theApp.cartridgeType != 0);
	GetDlgItem(IDC_REC_EMUBIOS)->EnableWindow(theApp.cartridgeType == 0);
	GetDlgItem(IDC_REC_GBABIOS)->EnableWindow(theApp.cartridgeType == 0);
	GetDlgItem(IDC_REC_GBABIOSINTRO)->EnableWindow(theApp.cartridgeType == 0);

	CheckRadioButton(IDC_REC_NOBIOS, IDC_REC_GBABIOSINTRO, IDC_REC_NOBIOS + m_biosOption);

	m_editFilename.LimitText(_MAX_PATH);
	m_editAuthor.LimitText(MOVIE_METADATA_AUTHOR_SIZE);
	m_editDescription.LimitText(MOVIE_METADATA_SIZE - MOVIE_METADATA_AUTHOR_SIZE);

	// convert the ROM filename into a default movie name
	CString movieName = winGetDestFilename(theApp.filename, IDS_MOVIE_DIR, ".vbm");

	GetDlgItem(IDC_MOVIE_FILENAME)->SetWindowText(movieName);

	// scroll to show the rightmost side of the movie filename
	((CEdit *)GetDlgItem(IDC_MOVIE_FILENAME))->SetSel((DWORD)(movieName.GetLength() - 1), FALSE);

	return TRUE; // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void MovieCreate::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(MovieCreate)
	DDX_Radio(pDX, IDC_RECNOW, m_startOption);
	DDX_Radio(pDX, IDC_REC_GBA, m_systemOption);
///	done manually DDX_Radio(pDX, IDC_REC_NOBIOS, m_biosOption);
	DDX_Control(pDX, IDC_EDIT_AUTHOR, m_editAuthor);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_editDescription);
	DDX_Control(pDX, IDC_MOVIE_FILENAME, m_editFilename);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(MovieCreate, CDialog)
ON_BN_CLICKED(IDOK, OnBnClickedOk)
ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
ON_BN_CLICKED(IDC_RECSTART, OnBnClickedRecstart)
ON_BN_CLICKED(IDC_RECRESET, OnBnClickedRecreset)
ON_BN_CLICKED(IDC_RECNOW, OnBnClickedRecnow)
ON_BN_CLICKED(IDC_REC_GBA, OnBnClickedRecGba)
ON_BN_CLICKED(IDC_REC_GBC, OnBnClickedRecGbc)
ON_BN_CLICKED(IDC_REC_SGB, OnBnClickedRecSgb)
ON_BN_CLICKED(IDC_REC_GB, OnBnClickedRecGb)
ON_BN_CLICKED(IDC_REC_NOBIOS, OnBnClickedRecNobios)
ON_BN_CLICKED(IDC_REC_EMUBIOS, OnBnClickedRecEmubios)
ON_BN_CLICKED(IDC_REC_GBABIOS, OnBnClickedRecGbabios)
ON_BN_CLICKED(IDC_REC_GBABIOSINTRO, OnBnClickedRecGbabiosintro)
END_MESSAGE_MAP()

// MovieCreate message handlers

void MovieCreate::OnBnClickedBrowse()
{
	theApp.winCheckFullscreen();

	LPCTSTR exts[] = { ".vbm", NULL };

	CString filter = theApp.winResLoadFilter(IDS_FILTER_MOVIE);
	CString title  = winResLoadString(IDS_SELECT_MOVIE_NAME);

	CString movieName = winGetDestFilename(theApp.filename, IDS_MOVIE_DIR, exts[0]);
	CString movieDir  = winGetDestDir(IDS_MOVIE_DIR);

	FileDlg dlg(this, movieName, filter, 1, "VBM", exts, movieDir, title, true);

	if (dlg.DoModal() == IDCANCEL)
	{
		return;
	}

	movieName = dlg.GetPathName();

	GetDlgItem(IDC_MOVIE_FILENAME)->SetWindowText(movieName);

	// scroll to show the rightmost side of the movie filename
	((CEdit *)GetDlgItem(IDC_MOVIE_FILENAME))->SetSel((DWORD)(movieName.GetLength() - 1), FALSE);
}

void loadBIOS()
{
	if (bios == NULL)
		return;

	useBios = false;
	if (theApp.useBiosFile)
	{
		int size = 0x4000;
		if (utilLoad(theApp.biosFileName,
		             utilIsGBABios,
		             bios,
		             size))
		{
			if (size == 0x4000)
				useBios = true;
		}
	}
	if (!useBios)
	{
		memcpy(bios, myROM, /*sizeof(myROM)*/ 0x4000); // copy internal emulator BIOS
		theApp.useBiosFile = false;
	}
}

bool checkBIOS(CString &biosFileName)
{
	if (biosFileName.IsEmpty())
		return false;

	bool ok		  = false;
	u8 * tempBIOS = (u8 *)malloc(0x4000);
	int	 size	  = 0x4000;
	if (utilLoad(biosFileName,
	             utilIsGBABios,
	             tempBIOS,
	             size))
	{
		if (size == 0x4000)
			ok = true;
		else
			systemMessage(MSG_INVALID_BIOS_FILE_SIZE, N_("Invalid BIOS file size"));
	}
	free(tempBIOS);
	return ok;
}

void MovieCreate::OnBnClickedOk()
{
	int startFlags = 0, controllerFlags = 0, typeFlags = 0;

	if (m_startOption == 0)
		startFlags |= MOVIE_START_FROM_SNAPSHOT;
	else if (m_startOption == 1)
		startFlags |= MOVIE_START_FROM_SRAM;
	//else
	//	startFlags = 0; // no SRAM or snapshot

	if (m_systemOption == 0)
		typeFlags |= MOVIE_TYPE_GBA, gbEmulatorType = 4;
	else if (m_systemOption == 1)
		typeFlags |= MOVIE_TYPE_GBC, gbEmulatorType = 1;
	else if (m_systemOption == 2)
		typeFlags |= MOVIE_TYPE_SGB, gbEmulatorType = 2;
	else
		gbEmulatorType = 3;  // plain old GB

	controllerFlags |= MOVIE_CONTROLLER(0);
	if (typeFlags & MOVIE_TYPE_SGB)
	{
		// XXX FIXME - the code for multiple controllers must be broken somehow
		// (it crashes strangely during FreezeToStream in SGB games)

		// SGB games are free to request controllers while running, so we have to assume it needs all 4
///		controllerFlags |= MOVIE_CONTROLLER(1) | MOVIE_CONTROLLER(2) | MOVIE_CONTROLLER(3);
	}

	if (m_biosOption == 1)
		theApp.useBiosFile = false;
	if (m_biosOption == 2)
		theApp.useBiosFile = true, theApp.skipBiosFile = true, loadBIOS();
	if (m_biosOption == 3)
		theApp.useBiosFile = true, theApp.skipBiosFile = false, loadBIOS();

	// get author and movie info from the edit fields:
	char info [MOVIE_METADATA_SIZE], buffer [MOVIE_METADATA_SIZE];

	GetDlgItem(IDC_EDIT_AUTHOR)->GetWindowText(buffer, MOVIE_METADATA_AUTHOR_SIZE);
	strncpy(info, buffer, MOVIE_METADATA_AUTHOR_SIZE);
	info[MOVIE_METADATA_AUTHOR_SIZE - 1] = '\0';

	GetDlgItem(IDC_EDIT_DESCRIPTION)->GetWindowText(buffer, MOVIE_METADATA_SIZE - MOVIE_METADATA_AUTHOR_SIZE);
	strncpy(info + MOVIE_METADATA_AUTHOR_SIZE, buffer, MOVIE_METADATA_SIZE - MOVIE_METADATA_AUTHOR_SIZE);
	info[MOVIE_METADATA_SIZE - 1] = '\0';

	if (memLagTempEnabled && theApp.cartridgeType == 0)
	{
		// lag reduction is off -- if the user didn't turn it off, silently turn it back on, otherwise ask
		int ans = !memLagEnabled ? IDYES : AfxGetApp()->m_pMainWnd->MessageBox(
		    "Lag reduction is currently OFF.\nIt is recommended that you turn this ON for recording.\nWould you like to turn it on now?",
		    "Warning",
		    MB_YESNOCANCEL | MB_ICONWARNING);
		switch (ans)
		{
		case IDYES:
			extern void SetPrefetchHack(bool);
			SetPrefetchHack(true);
			break;
		case IDNO:
			break;
		case IDCANCEL:
			return;
		}
	}

	CString movieName;

	GetDlgItem(IDC_MOVIE_FILENAME)->GetWindowText(movieName);

	// actually make the movie file:
	int code = VBAMovieCreate(movieName, info, startFlags, controllerFlags, typeFlags);

	if (code != MOVIE_SUCCESS)
	{
		systemMessage(0, "Failed to create movie %s", (const char *)movieName);
		return;
	}

	OnOK();
}

void MovieCreate::OnBnClickedCancel()
{
	OnCancel();
}

void MovieCreate::OnBnClickedRecstart()
{
	m_startOption = 2;
	if (theApp.cartridgeType == 0)
	{
		GetDlgItem(IDC_REC_EMUBIOS)->EnableWindow(TRUE);
		GetDlgItem(IDC_REC_GBABIOSINTRO)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_REC_GBC)->EnableWindow(GBC_CAPABLE);
		GetDlgItem(IDC_REC_SGB)->EnableWindow(SGB_CAPABLE);
		GetDlgItem(IDC_REC_GB)->EnableWindow(TRUE);
	}
}

void MovieCreate::OnBnClickedRecreset()
{
	m_startOption = 1;
	if (theApp.cartridgeType == 0)
	{
		GetDlgItem(IDC_REC_EMUBIOS)->EnableWindow(TRUE);
		GetDlgItem(IDC_REC_GBABIOSINTRO)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_REC_GBC)->EnableWindow(GBC_CAPABLE);
		GetDlgItem(IDC_REC_SGB)->EnableWindow(SGB_CAPABLE);
		GetDlgItem(IDC_REC_GB)->EnableWindow(TRUE);
	}
}

void MovieCreate::OnBnClickedRecnow()
{
	m_startOption = 0;

	// starting from emulator bios file from a snapshot made while playing with GBA bios file won't work
	if (theApp.cartridgeType == 0 && useBios)
	{
		if (m_biosOption == 1)
		{
			OnBnClickedRecGbabios();
			CheckRadioButton(IDC_REC_NOBIOS, IDC_REC_GBABIOSINTRO, IDC_REC_NOBIOS + m_biosOption);
		}
		GetDlgItem(IDC_REC_EMUBIOS)->EnableWindow(FALSE);
	}

	// "with intro" distinction makes no sense when continuing from snapshot
	if (theApp.cartridgeType == 0)
	{
		if (m_biosOption == 3)
		{
			OnBnClickedRecGbabios();
			CheckRadioButton(IDC_REC_NOBIOS, IDC_REC_GBABIOSINTRO, IDC_REC_NOBIOS + m_biosOption);
		}
		GetDlgItem(IDC_REC_GBABIOSINTRO)->EnableWindow(FALSE);
	}

	// can't switch systems while recording from snapshot!
	if (theApp.cartridgeType != 0)
	{
		int curSystemOption = (gbCgbMode == 1 ? 1 : (gbSgbMode == 1 ? 2 : 3)); // GBC, SGB, or GB
		GetDlgItem(IDC_REC_GBC)->EnableWindow(curSystemOption == 1);
		GetDlgItem(IDC_REC_SGB)->EnableWindow(curSystemOption == 2);
		GetDlgItem(IDC_REC_GB)->EnableWindow(curSystemOption == 3);
		m_systemOption = curSystemOption;
		CheckRadioButton(IDC_REC_GBA, IDC_REC_GB, IDC_REC_GBA + m_systemOption);
	}
}

void MovieCreate::OnBnClickedRecGba()
{
	m_systemOption = 0;
}

void MovieCreate::OnBnClickedRecGbc()
{
	m_systemOption = 1;
}

void MovieCreate::OnBnClickedRecSgb()
{
	m_systemOption = 2;
}

void MovieCreate::OnBnClickedRecGb()
{
	m_systemOption = 3;
}

void MovieCreate::OnBnClickedRecNobios()
{
	m_biosOption = 0;
}

void MovieCreate::OnBnClickedRecEmubios()
{
	m_biosOption = 1;
}

void MovieCreate::OnBnClickedRecGbabios()
{
	if (checkBIOS(theApp.biosFileName))
		m_biosOption = 2;
	else
	{
		((MainWnd *)theApp.m_pMainWnd)->OnOptionsEmulatorSelectbiosfile();
		if (checkBIOS(theApp.biosFileName))
			m_biosOption = 2;
		else
			CheckRadioButton(IDC_REC_NOBIOS, IDC_REC_GBABIOSINTRO, IDC_REC_EMUBIOS);
	}
}

void MovieCreate::OnBnClickedRecGbabiosintro()
{
	if (checkBIOS(theApp.biosFileName))
		m_biosOption = 3;
	else
	{
		((MainWnd *)theApp.m_pMainWnd)->OnOptionsEmulatorSelectbiosfile();
		if (checkBIOS(theApp.biosFileName))
			m_biosOption = 3;
		else
			CheckRadioButton(IDC_REC_NOBIOS, IDC_REC_GBABIOSINTRO, IDC_REC_EMUBIOS);
	}
}

