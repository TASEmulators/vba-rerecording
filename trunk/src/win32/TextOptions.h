#pragma once

// TextOptions dialog

class TextOptions : public CDialog
{
	DECLARE_DYNAMIC(TextOptions)

public:
	TextOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~TextOptions();

	virtual BOOL OnInitDialog() ;

// Dialog Data
	enum { IDD = IDD_TEXTCONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadioPrefilter();
	afx_msg void OnBnClickedRadioPostfilter();
	afx_msg void OnBnClickedRadioPostrender();
};
