#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

class CAboutDialog : public CDialog {
public:
	CAboutDialog(CWnd* pParent = NULL);

protected:
	void OpenUrl(LPCWSTR url, LPCSTR urlText);

	//{{AFX_MSG(CAboutDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnProjectWebsite();
	afx_msg void OnReportBug();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
