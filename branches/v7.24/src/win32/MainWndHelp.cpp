#include "stdafx.h"
#include "resource.h"
#include "MainWnd.h"
#include "AboutDialog.h"
#include "BugReport.h"
#include "VBA.h"

extern int emulating;

void MainWnd::OnHelpAbout()
{
	theApp.winCheckFullscreen();
	AboutDialog dlg;

	dlg.DoModal();
}

void MainWnd::OnHelpFaq()
{
	::ShellExecute(0, _T("open"), "http://vba.ngemu.com/faq.shtml",
	               0, 0, SW_SHOWNORMAL);
}

void MainWnd::OnHelpBugreport()
{
	BugReport dlg(theApp.m_pMainWnd);

	dlg.DoModal();
}

