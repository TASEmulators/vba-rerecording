#ifndef VBA_MOVIE_OPEN_H
#define VBA_MOVIE_OPEN_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../movie.h"

// MovieOpen dialog

class MovieOpen : public CDialog
{
	DECLARE_DYNAMIC(MovieOpen)
public:
	MovieOpen(CWnd*pParent = NULL);    // standard constructor
	virtual ~MovieOpen();

	virtual BOOL OnInitDialog() ;

// Dialog Data
	//{{AFX_DATA(MovieOpen)
	enum { IDD = IDD_MOVIEOPEN };
	CEdit   m_editAuthor;
	CEdit   m_editDescription;
	CEdit   m_editFilename;
	CEdit   m_editPauseFrame;
	CString moviePhysicalName;
	CString movieLogicalName;
	SMovie  movieInfo;
	int     pauseFrame;
	//}}AFX_DATA
protected:
	virtual void DoDataExchange(CDataExchange*pDX);     // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedMovieRefresh();
	afx_msg void OnBnClickedReadonly();
	afx_msg void OnBnClickedCheckPauseframe();

	afx_msg void OnBnClickedHideborder();

	virtual BOOL OnWndMsg(UINT, WPARAM, LPARAM, LRESULT *);
	afx_msg void OnEnChangeMovieFilename();
};

#endif // VBA_MOVIE_OPEN_H
