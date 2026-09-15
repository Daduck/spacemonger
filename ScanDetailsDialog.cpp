#include "stdafx.h"
#include "resource.h"
#include "ScanDetailsDialog.h"
#include "PathUtil.h"
#include "Lang.h"

CScanDetailsDialog::CScanDetailsDialog(ui64 totalSkipped,
	const std::vector<std::wstring>& paths, CWnd *parent)
	: CDialog(IDD_SCAN_DETAILS, parent)
	, m_totalSkipped(totalSkipped)
	, m_paths(paths)
{
}

BEGIN_MESSAGE_MAP(CScanDetailsDialog, CDialog)
	ON_BN_CLICKED(IDC_SCAN_DETAILS_COPY, OnCopyPaths)
END_MESSAGE_MAP()

BOOL CScanDetailsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(CurLang->scan_details_title);
	CString summary;
	summary.Format(CurLang->scan_details_summary_format,
		m_totalSkipped, (ui64)m_paths.size());
	SetDlgItemText(IDC_SCAN_DETAILS_SUMMARY, summary);
	SetDlgItemText(IDC_SCAN_DETAILS_COPY, CurLang->scan_details_copy);
	SetDlgItemText(IDOK, CurLang->ok);

	CListBox *list = (CListBox *)GetDlgItem(IDC_SCAN_DETAILS_LIST);
	if (list != NULL) {
		for (const auto& path : m_paths) {
			std::string display = PathUtil::WideToAnsi(path);
			list->AddString(display.c_str());
		}
	}

	return TRUE;
}

void CScanDetailsDialog::OnCopyPaths()
{
	if (m_paths.empty()) return;

	SIZE_T chars = 1;
	for (const auto& path : m_paths) chars += path.length() + 2;
	HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, chars * sizeof(wchar_t));
	if (memory == NULL) return;

	wchar_t *buffer = (wchar_t *)GlobalLock(memory);
	if (buffer == NULL) {
		GlobalFree(memory);
		return;
	}

	wchar_t *cursor = buffer;
	for (const auto& path : m_paths) {
		memcpy(cursor, path.c_str(), path.length() * sizeof(wchar_t));
		cursor += path.length();
		*cursor++ = L'\r';
		*cursor++ = L'\n';
	}
	*cursor = L'\0';
	GlobalUnlock(memory);

	if (!::OpenClipboard(m_hWnd)) {
		GlobalFree(memory);
		return;
	}
	EmptyClipboard();
	if (SetClipboardData(CF_UNICODETEXT, memory) == NULL) GlobalFree(memory);
	CloseClipboard();
}
