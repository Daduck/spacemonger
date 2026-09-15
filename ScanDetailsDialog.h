#ifndef SCANDETAILSDIALOG_H
#define SCANDETAILSDIALOG_H

#ifdef bool
#pragma push_macro("bool")
#undef bool
#endif
#include <string>
#include <vector>
#ifdef bool
#pragma pop_macro("bool")
#endif
#include "e.h"

class CScanDetailsDialog : public CDialog {
public:
	CScanDetailsDialog(ui64 totalSkipped, const std::vector<std::wstring>& paths,
		CWnd *parent = NULL);

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnCopyPaths();
	DECLARE_MESSAGE_MAP()

	ui64 m_totalSkipped;
	std::vector<std::wstring> m_paths;
};

#endif
