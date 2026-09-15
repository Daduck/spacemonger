
#include "stdafx.h"
#include "spacemonger.h"
#include "AboutDialog.h"
#include "Lang.h"
#include <shellapi.h>

namespace {
	const wchar_t kProjectWebsiteUrl[] = L"https://andedammen.dk/spacemonger";
	const char kProjectWebsiteUrlText[] = "https://andedammen.dk/spacemonger";
	const wchar_t kReportBugUrl[] = L"https://github.com/Daduck/spacemonger/issues";
	const char kReportBugUrlText[] = "https://github.com/Daduck/spacemonger/issues";
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CAboutDialog::CAboutDialog(CWnd* pParent)
	: CDialog(IDD_ABOUT, pParent)
{
}

BEGIN_MESSAGE_MAP(CAboutDialog, CDialog)
	//{{AFX_MSG_MAP(CAboutDialog)
	ON_BN_CLICKED(IDC_PROJECT_WEBSITE, OnProjectWebsite)
	ON_BN_CLICKED(IDC_REPORT_BUG, OnReportBug)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(CurLang->about_spacemonger);
	SetDlgItemText(IDC_STATIC_FREEWARE, CurLang->freeware);
	SetDlgItemText(IDC_STATIC_WARRANTY, CurLang->warranty);
	SetDlgItemText(IDC_STATIC_DESCRIPTION, CurLang->description);
	SetDlgItemText(IDC_STATIC_MAINTAINER, CurLang->maintainer);
	SetDlgItemText(IDC_PROJECT_WEBSITE, CurLang->project_website);
	SetDlgItemText(IDC_REPORT_BUG, CurLang->report_bug);
	SetDlgItemText(IDOK, CurLang->ok);

	return TRUE;
}

void CAboutDialog::OpenUrl(LPCWSTR url, LPCSTR urlText)
{
	HINSTANCE result = ShellExecuteW(m_hWnd, L"open", url, NULL, NULL, SW_SHOWNORMAL);
	if ((INT_PTR)result <= 32) {
		CString message;
		message.Format(CurLang->open_url_failed_format, urlText);
		MessageBox(message, CurLang->about_spacemonger, MB_OK | MB_ICONERROR);
	}
}

void CAboutDialog::OnProjectWebsite()
{
	OpenUrl(kProjectWebsiteUrl, kProjectWebsiteUrlText);
}

void CAboutDialog::OnReportBug()
{
	OpenUrl(kReportBugUrl, kReportBugUrlText);
}
