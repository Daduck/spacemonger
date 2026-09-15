
#include "StdAfx.h"
#include "SpaceMonger.h"
#include "FolderView.h"
#include "AboutDialog.h"
#include "ScanDetailsDialog.h"
#include "Lang.h"

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SYSCOMMAND()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATEAPP()
	ON_WM_CLOSE()
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(0x02E0, OnDpiChanged)
	ON_MESSAGE(WM_SCAN_NOTICE_CLICKED, OnScanNoticeClickedMessage)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(IDC_SCAN_NOTICE, OnScanNoticeUpdate)
	ON_UPDATE_COMMAND_UI_RANGE(100, 41000, OnIgnoreUpdate)
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(CScanNoticeButton, CButton)
	ON_WM_LBUTTONUP()
	ON_WM_KEYUP()
END_MESSAGE_MAP()

void CScanNoticeButton::OnLButtonUp(UINT nFlags, CPoint point)
{
	CButton::OnLButtonUp(nFlags, point);
	CRect rect;
	GetClientRect(&rect);
	if (rect.PtInRect(point) && GetParent() != NULL)
		GetParent()->SendMessage(WM_SCAN_NOTICE_CLICKED, 0, 0);
}

void CScanNoticeButton::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CButton::OnKeyUp(nChar, nRepCnt, nFlags);
	if ((nChar == VK_SPACE || nChar == VK_RETURN) && GetParent() != NULL)
		GetParent()->SendMessage(WM_SCAN_NOTICE_CLICKED, 0, 0);
}

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	CFrameWnd::PreCreateWindow(cs);

	cs.style &= ~FWS_ADDTOTITLE;
	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;

	return(1);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1) return(-1);

	SetWindowText("SpaceMonger");

	HINSTANCE hinst = AfxFindResourceHandle(MAKEINTRESOURCE(ID_SPACEMONGER), RT_ICON);
	HICON hicon = ::LoadIcon(hinst, MAKEINTRESOURCE(ID_SPACEMONGER));
	SetIcon(hicon, 1);
	//SetIcon(hicon, 0);

	if (!m_toolbar.Create(this)) return(-1);
	if (!m_scanNoticeButton.Create("", WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP,
		CRect(0, 0, 1, 1), this, IDC_SCAN_NOTICE)) return(-1);
	m_scanNoticeButton.ShowWindow(SW_HIDE);

	CMenu *menu = GetSystemMenu(FALSE);
	menu->AppendMenu(MF_SEPARATOR);
	menu->AppendMenu(MF_STRING, ID_APP_ABOUT, CString("&")
		+ CurLang->about_spacemonger + "...");

	WINDOWPLACEMENT w;
	w.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement(m_hWnd, &w);
	theApp.m_settings.rect.left = w.rcNormalPosition.left;
	theApp.m_settings.rect.top = w.rcNormalPosition.top;
	theApp.m_settings.rect.right = w.rcNormalPosition.right;
	theApp.m_settings.rect.bottom = w.rcNormalPosition.bottom;
	theApp.m_settings.showcmd = w.showCmd;

	RecalcLayout(0);

	return(0);
}

void CMainFrame::OnDestroy(void)
{
	WINDOWPLACEMENT w;
	w.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement(m_hWnd, &w);
	theApp.m_settings.rect.left = w.rcNormalPosition.left;
	theApp.m_settings.rect.top = w.rcNormalPosition.top;
	theApp.m_settings.rect.right = w.rcNormalPosition.right;
	theApp.m_settings.rect.bottom = w.rcNormalPosition.bottom;
	theApp.m_settings.showcmd = w.showCmd;
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	if (bActive) {
		CSpaceMonger *monger = (CSpaceMonger *)AfxGetApp();
		if (monger != NULL && monger->m_view != NULL) {
			CFolderView *view = (CFolderView *)monger->m_view;
			view->OnUpdate(monger->m_document);
		}
	}
	else {
		CSpaceMonger *monger = (CSpaceMonger *)AfxGetApp();
		if (monger != NULL && monger->m_view != NULL) {
			CFolderView *view = (CFolderView *)monger->m_view;
			view->SelectFolder(NULL);
		}
	}
}

void CMainFrame::RecalcLayout(BOOL bNotify)
{
	CRect rectBorder;
	int numwnds, wndnum, width;
	CWnd *wnd, *first, *last;

	if (m_bInRecalcLayout || !IsWindow(m_hWnd)
		|| (wnd = GetWindow(GW_CHILD)) == NULL) return;

	m_bInRecalcLayout = TRUE;

	first = wnd->GetWindow(GW_HWNDFIRST);
	last = wnd->GetWindow(GW_HWNDLAST);

	GetClientRect(&rectBorder);
	rectBorder.OffsetRect(-rectBorder.left, -rectBorder.top);
	width = rectBorder.right - rectBorder.left;

	// Position our two toolbars
	m_toolbar.SetWindowPos(NULL, 0, -2, width, 30,
		SWP_NOZORDER|SWP_NOACTIVATE);
	if (::IsWindow(m_scanNoticeButton.m_hWnd) && m_scanNotice.IsVisible()) {
		CRect toolbarRect;
		m_toolbar.GetWindowRect(&toolbarRect);
		ScreenToClient(&toolbarRect);
		int noticeWidth = min(120, max(112, width / 5));
		m_scanNoticeButton.SetWindowPos(&CWnd::wndTop,
			toolbarRect.right - noticeWidth - 4, toolbarRect.top + 5,
			noticeWidth, 20,
			SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
	rectBorder.top += 28;

	// First count how many windows need to be rearranged
	numwnds = 0;
	if (first != NULL) {
		wnd = first;
		while (1) {
			if (wnd->m_hWnd != m_toolbar.m_hWnd && wnd->m_hWnd != m_scanNoticeButton.m_hWnd)
				numwnds++;
			if (wnd == last) break;
			wnd = wnd->GetWindow(GW_HWNDNEXT);
		}
	}

	// Now lay them all out in a horizontal band (cheap & easy)
	if (first != NULL && numwnds > 0) {
		wndnum = 0, wnd = first;
		while (1) {
			if (wnd->m_hWnd != m_toolbar.m_hWnd && wnd->m_hWnd != m_scanNoticeButton.m_hWnd) {
				wnd->SetWindowPos(NULL, rectBorder.left + (width * wndnum) / numwnds, rectBorder.top,
					(width * (wndnum+1)) / numwnds - (width * wndnum) / numwnds,
					rectBorder.bottom - rectBorder.top,
					SWP_NOZORDER);
				if (++wndnum == numwnds) break;
			}
			wnd = wnd->GetWindow(GW_HWNDNEXT);
		}
	}

	m_bInRecalcLayout = FALSE;
}

void CMainFrame::SetScanNotice(ui64 skippedDirectories, DWORD firstError,
	const std::vector<std::wstring>& paths)
{
	m_scanNotice.Set(skippedDirectories, firstError);
	m_skippedPaths = paths;

	CString text;
	text.Format(CurLang->scan_partial_button_format, skippedDirectories);
	m_scanNoticeButton.SetWindowText(text);
	m_scanNoticeButton.EnableWindow(TRUE);
	m_scanNoticeButton.ShowWindow(SW_SHOWNORMAL);
	RecalcLayout(0);
}

void CMainFrame::ClearScanNotice(void)
{
	m_scanNotice.Clear();
	m_skippedPaths.clear();
	if (::IsWindow(m_scanNoticeButton.m_hWnd)) {
		m_scanNoticeButton.EnableWindow(FALSE);
		m_scanNoticeButton.ShowWindow(SW_HIDE);
		RecalcLayout(0);
	}
}

void CMainFrame::OnScanNoticeClicked()
{
	if (!m_scanNotice.IsVisible()) return;
	CScanDetailsDialog dialog(m_scanNotice.skippedDirectories, m_skippedPaths, this);
	dialog.DoModal();
}

LRESULT CMainFrame::OnScanNoticeClickedMessage(WPARAM wParam, LPARAM lParam)
{
	OnScanNoticeClicked();
	return 0;
}

void CMainFrame::OnScanNoticeUpdate(CCmdUI *ui)
{
	ui->Enable(m_scanNotice.IsVisible());
}

void CMainFrame::OnSysCommand(UINT nid, LPARAM lparam)
{
	if (nid == ID_APP_ABOUT) {
		CAboutDialog about;
		about.DoModal();
	}
	else CFrameWnd::OnSysCommand(nid, lparam);
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
#if 0
	CRect rect;
	GetClientRect(&rect);
	FillBox(pDC->m_hDC, ColorFlat, 0, 0,
		rect.right - rect.left, rect.bottom - rect.top);
#endif
	return(-1);
}

void CMainFrame::OnClose()
{
	CFrameWnd::OnClose();
}

void CMainFrame::OnIgnoreUpdate(CCmdUI *ui)
{
	GeneralIgnoreUpdate(ui);
}

void CMainFrame::OnWindowPosChanged(WINDOWPOS *wp)
{
	CFrameWnd::OnWindowPosChanged(wp);

	WINDOWPLACEMENT w;
	w.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement(m_hWnd, &w);
	theApp.m_settings.rect.left = w.rcNormalPosition.left;
	theApp.m_settings.rect.top = w.rcNormalPosition.top;
	theApp.m_settings.rect.right = w.rcNormalPosition.right;
	theApp.m_settings.rect.bottom = w.rcNormalPosition.bottom;
	theApp.m_settings.showcmd = w.showCmd;
}

void CMainFrame::OnShowWindow(BOOL bShow, UINT status)
{
	CFrameWnd::OnShowWindow(bShow, status);

	WINDOWPLACEMENT w;
	w.length = sizeof(WINDOWPLACEMENT);
	::GetWindowPlacement(m_hWnd, &w);
	theApp.m_settings.rect.left = w.rcNormalPosition.left;
	theApp.m_settings.rect.top = w.rcNormalPosition.top;
	theApp.m_settings.rect.right = w.rcNormalPosition.right;
	theApp.m_settings.rect.bottom = w.rcNormalPosition.bottom;
	theApp.m_settings.showcmd = w.showCmd;
}

LRESULT CMainFrame::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	RECT* prc = (RECT*)lParam;
	if (prc != NULL) {
		SetWindowPos(NULL, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}
	RecalcLayout(0);
	if (theApp.m_view != NULL) {
		((CFolderView*)theApp.m_view)->RecreateFonts();
		theApp.m_view->Invalidate();
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CMainToolBar, CToolBar)

BEGIN_MESSAGE_MAP(CMainToolBar, CToolBar)
	//{{AFX_MSG_MAP(CMainToolBar)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI_RANGE(100, 41000, OnIgnoreUpdate)
END_MESSAGE_MAP()

CMainToolBar::CMainToolBar()
{
}

CMainToolBar::~CMainToolBar()
{
}

int CMainToolBar::Create(CWnd *parent)
{
	if (!CToolBar::Create(parent, WS_CHILD|WS_VISIBLE|CBRS_TOP, IDR_TOOLBAR_EN)) return 0;
	if (!LoadToolBar(CurLang->toolbar_bitmap_id)) return 0;

	EnableState(ViewFree, 1);

	return 1;
}

void CMainToolBar::EnableState(int state, BOOL enabled)
{
	switch (state) {
	case CMainToolBar::ViewFree:
		SetButtonStyle(CommandToIndex(ID_VIEW_FREE), enabled ? TBBS_BUTTON|TBBS_PRESSED : TBBS_BUTTON);
		break;
	default:
		break;
	}
}

static inline void SetButtonEnabledIfChanged(CToolBarCtrl &ctrl, int id, BOOL enable)
{
	if ((ctrl.IsButtonEnabled(id) != 0) != (enable != 0)) {
		ctrl.EnableButton(id, enable);
	}
}

void CMainToolBar::UpdateButtonsForView(CFolderView *view)
{
	CSpaceMonger *app = (CSpaceMonger *)AfxGetApp();
	CToolBarCtrl &ctrl = this->GetToolBarCtrl();
	SetButtonEnabledIfChanged(ctrl, ID_FILE_RUN, view->IsAnythingOpen() && view->IsAnythingSelected());
	SetButtonEnabledIfChanged(ctrl, ID_FILE_DELETE, !app->m_settings.disable_delete
		&& view->IsAnythingOpen() && view->IsAnythingSelected());
	SetButtonEnabledIfChanged(ctrl, ID_FILE_OPEN, 1);
	SetButtonEnabledIfChanged(ctrl, ID_FILE_REFRESH, view->IsAnythingOpen());
	SetButtonEnabledIfChanged(ctrl, ID_VIEW_ZOOM_FULL, view->IsAnythingOpen() && !view->IsZoomFull());
	SetButtonEnabledIfChanged(ctrl, ID_VIEW_ZOOM_OUT, view->IsAnythingOpen() && !view->IsZoomFull());
	SetButtonEnabledIfChanged(ctrl, ID_VIEW_ZOOM_IN, view->IsAnythingOpen() && view->IsAnythingSelected()
		&& view->IsSelectedAFolder());
	SetButtonEnabledIfChanged(ctrl, ID_VIEW_FREE, view->IsAnythingOpen());
	SetButtonEnabledIfChanged(ctrl, ID_SETTINGS, 1);
	SetButtonEnabledIfChanged(ctrl, ID_APP_ABOUT, 1);
}

void CMainToolBar::OnIgnoreUpdate(CCmdUI *ui)
{
	GeneralIgnoreUpdate(ui);
}

void CMainToolBar::Reload(void)
{
	LoadToolBar(CurLang->toolbar_bitmap_id);
	RedrawWindow();
}
