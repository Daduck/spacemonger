
#ifndef CMAINFRAME_H
#define CMAINFRAME_H

#ifdef bool
#pragma push_macro("bool")
#undef bool
#endif
#include <string>
#include <vector>
#ifdef bool
#pragma pop_macro("bool")
#endif
#include "ScanNotice.h"

#define WM_SCAN_NOTICE_CLICKED (WM_APP + 42)

class CScanNoticeButton : public CButton {
protected:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()
};

class CMainToolBar : public CToolBar {
public:
	CMainToolBar();
	virtual ~CMainToolBar();
	DECLARE_DYNCREATE(CMainToolBar)

	virtual int Create(CWnd *parent);

	enum {
		NoState = 0,
		ViewFree,
	};

	virtual void EnableState(int state, BOOL enabled);
	void UpdateButtonsForView(class CFolderView *view);

	void Reload(void);

protected:
	//{{AFX_MSG(CMainToolBar)
	afx_msg void OnIgnoreUpdate(CCmdUI *ui);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

class CMainFrame : public CFrameWnd {
public:
	CMainFrame();
	virtual ~CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)
	BOOL PreCreateWindow(CREATESTRUCT& cs);

	virtual void RecalcLayout(BOOL bNotify);
	void SetScanNotice(ui64 skippedDirectories, DWORD firstError,
		const std::vector<std::wstring>& paths);
	void ClearScanNotice(void);

protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy(void);
	afx_msg void OnSysCommand(UINT nid, LPARAM lparam);
	afx_msg void OnClose();
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnWindowPosChanged(WINDOWPOS *wp);
	afx_msg void OnShowWindow(BOOL bShow, UINT status);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnScanNoticeClicked();
	afx_msg LRESULT OnScanNoticeClickedMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnScanNoticeUpdate(CCmdUI *ui);
	//}}AFX_MSG
	afx_msg void OnIgnoreUpdate(CCmdUI *ui);
	DECLARE_MESSAGE_MAP()

public:
	CMainToolBar m_toolbar;

protected:
	CScanNoticeButton m_scanNoticeButton;
	ScanNoticeState m_scanNotice;
	std::vector<std::wstring> m_skippedPaths;
};

#endif

