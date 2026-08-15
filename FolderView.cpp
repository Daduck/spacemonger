#include "stdafx.h"
#include "spacemonger.h"
#include "FolderView.h"
#include "AsyncScanEngine.h"
#include "MainFrm.h"
#include "CommandPolicy.h"
#include "TipWnd.h"
#include "PathUtil.h"
#include "Lang.h"

IMPLEMENT_DYNCREATE(CFolderView, CFreeView)

static COLORREF BoxColors[] = {
	PALETTERGB(0xFF, 0x7F, 0x7F),
	PALETTERGB(0xFF, 0xBF, 0x7F),
	PALETTERGB(0xFF, 0xFF, 0x00),
	PALETTERGB(0x7F, 0xFF, 0x7F),
	PALETTERGB(0x7F, 0xFF, 0xFF),
	PALETTERGB(0xBF, 0xBF, 0xFF),
	PALETTERGB(0xBF, 0xBF, 0xBF),
	PALETTERGB(0xFF, 0x7F, 0xFF),

	PALETTERGB(0xFF, 0xBF, 0xBF),
	PALETTERGB(0xFF, 0xDF, 0xBF),
	PALETTERGB(0xFF, 0xFF, 0xBF),
	PALETTERGB(0xBF, 0xFF, 0xBF),
	PALETTERGB(0xDF, 0xFF, 0xFF),
	PALETTERGB(0xDF, 0xDF, 0xFF),
	PALETTERGB(0xDF, 0xDF, 0xDF),
	PALETTERGB(0xFF, 0xBF, 0xFF),

	PALETTERGB(0xBF, 0x7F, 0x7F),
	PALETTERGB(0xBF, 0x9F, 0x5F),
	PALETTERGB(0xBF, 0xBF, 0x3F),
	PALETTERGB(0x7F, 0xBF, 0x7F),
	PALETTERGB(0x7F, 0xBF, 0xBF),
	PALETTERGB(0x9F, 0x9F, 0xFF),
	PALETTERGB(0x9F, 0x9F, 0x9F),
	PALETTERGB(0xBF, 0x7F, 0xBF),

	PALETTERGB(0x00, 0x00, 0x00),
	PALETTERGB(0xFF, 0xFF, 0xFF),
};

// These colors are used for specifically-chosen folder colors.
static COLORREF FixedColors[] = {
	PALETTERGB(0xFF, 0xFF, 0xFF),
	PALETTERGB(0xBF, 0xBF, 0xBF),
	PALETTERGB(0x7F, 0x7F, 0x7F),

	PALETTERGB(0xFF, 0x7F, 0x7F),
	PALETTERGB(0xFF, 0xBF, 0x7F),
	PALETTERGB(0xFF, 0xFF, 0x00),
	PALETTERGB(0x7F, 0xFF, 0x7F),
	PALETTERGB(0x7F, 0xFF, 0xFF),
	PALETTERGB(0xBF, 0xBF, 0xFF),
	PALETTERGB(0xFF, 0x7F, 0xFF),

	PALETTERGB(0xFF, 0xFF, 0xFF),
	PALETTERGB(0xFF, 0xFF, 0xFF),
	PALETTERGB(0xBF, 0xBF, 0xBF),

	PALETTERGB(0xFF, 0x9F, 0x9F),
	PALETTERGB(0xFF, 0xDF, 0xBF),
	PALETTERGB(0xFF, 0xFF, 0xBF),
	PALETTERGB(0xBF, 0xFF, 0xBF),
	PALETTERGB(0xDF, 0xFF, 0xFF),
	PALETTERGB(0xDF, 0xDF, 0xFF),
	PALETTERGB(0xFF, 0xBF, 0xFF),

	PALETTERGB(0xBF, 0xBF, 0xBF),
	PALETTERGB(0x7F, 0x7F, 0x7F),
	PALETTERGB(0x3F, 0x3F, 0x3F),

	PALETTERGB(0xBF, 0x7F, 0x7F),
	PALETTERGB(0xBF, 0x9F, 0x9F),
	PALETTERGB(0xBF, 0xBF, 0x3F),
	PALETTERGB(0x7F, 0xBF, 0x7F),
	PALETTERGB(0x7F, 0xBF, 0xBF),
	PALETTERGB(0x9F, 0x9F, 0xFF),
	PALETTERGB(0xBF, 0x7F, 0xBF),
};

CFolderView::CFolderView()
{
	rootfolder = NULL;
	selected = NULL;
	zoomlevel = 0;
	showfreespace = 1;
	lastcur = NULL;
}

CFolderView::~CFolderView()
{
	m_layoutNodes.clear();
}

BEGIN_MESSAGE_MAP(CFolderView, CFreeView)
	//{{AFX_MSG_MAP(CFolderView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI_RANGE(100, 41000, OnIgnoreUpdate)
END_MESSAGE_MAP()

static UINT GetWindowDpi(HWND hwnd)
{
	typedef UINT (WINAPI *GetDpiForWindowProc)(HWND);
	HMODULE hUser32 = GetModuleHandleA("user32.dll");
	if (hUser32 != NULL) {
		GetDpiForWindowProc pGetDpiForWindow = (GetDpiForWindowProc)GetProcAddress(hUser32, "GetDpiForWindow");
		if (pGetDpiForWindow != NULL && hwnd != NULL) {
			UINT dpi = pGetDpiForWindow(hwnd);
			if (dpi != 0) return dpi;
		}
	}
	HDC hdc = ::GetDC(hwnd);
	UINT dpi = 96;
	if (hdc != NULL) {
		dpi = ::GetDeviceCaps(hdc, LOGPIXELSY);
		::ReleaseDC(hwnd, hdc);
	}
	return (dpi != 0) ? dpi : 96;
}

void CFolderView::RecreateFonts(void)
{
	if (minifont.m_hObject != NULL) minifont.DeleteObject();

	minifont.CreateFont(-9, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
		OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		VARIABLE_PITCH|FF_SWISS, "Small Fonts");

	if (::IsWindow(m_nametipwnd.m_hWnd)) {
		HFONT nametipfont = ::CreateFont(-9, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
			OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			VARIABLE_PITCH|FF_SWISS, "Small Fonts");
		::SendMessage(m_nametipwnd.m_hWnd, WM_SETFONT, (WPARAM)nametipfont, TRUE);
	}
}

BOOL CFolderView::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	if (message == 0x02E0 /* WM_DPICHANGED */) {
		RecreateFonts();
		Invalidate();
		if (pResult != NULL) *pResult = 0;
		return TRUE;
	}

	if (::IsWindow(m_infotipwnd.m_hWnd)) {
		MSG msg;
		msg.message = message;
		msg.wParam = wParam;
		msg.lParam = lParam;
		msg.hwnd = m_hWnd;
		m_infotipwnd.ReflectMessage(&msg);
	}
	if (::IsWindow(m_nametipwnd.m_hWnd)) {
		MSG msg;
		msg.message = message;
		msg.wParam = wParam;
		msg.lParam = lParam;
		msg.hwnd = m_hWnd;
		m_nametipwnd.ReflectMessage(&msg);
	}
	return CFreeView::OnWndMsg(message, wParam, lParam, pResult);
}

int CFolderView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFreeView::OnCreate(lpCreateStruct) == -1)
		return -1;
	if (!m_infotipwnd.CreateEz(this, 64, 64)) return -1;
	if (!m_nametipwnd.CreateEz(this, 64, 64)) return -1;
	
	int i;
	LOGPALETTE *logpalette = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) + 26 * sizeof(PALETTEENTRY));
	logpalette->palVersion = 0x300;
	logpalette->palNumEntries = 26;
	for (i = 0; i < 26; i++) {
		logpalette->palPalEntry[i].peFlags = 0;
		logpalette->palPalEntry[i].peRed = GetRValue(BoxColors[i]);
		logpalette->palPalEntry[i].peGreen = GetGValue(BoxColors[i]);
		logpalette->palPalEntry[i].peBlue = GetBValue(BoxColors[i]);
	}
	m_palette.CreatePalette(logpalette);
	free(logpalette);

	black.CreateSolidBrush(RGB(0x00, 0x00, 0x00));
	white.CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));

	RecreateFonts();

	m_infotipwnd.SetAutoShow(1);
	m_infotipwnd.SetAutoPos(1);
	m_infotipwnd.SetShowDelay(theApp.m_settings.infotip_delay);
	m_infotipwnd.EnableWindow(0);

	m_nametipwnd.SetAutoShow(1);
	m_nametipwnd.SetShowDelay(theApp.m_settings.nametip_delay);
	m_nametipwnd.SetVPadding(0);
	m_nametipwnd.SetHPadding(1);
	m_nametipwnd.EnableWindow(0);

	return 0;
}

void CFolderView::OnDestroy() 
{
	m_palette.DeleteObject();
	black.DeleteObject();
	white.DeleteObject();
	minifont.DeleteObject();

	m_infotipwnd.DestroyWindow();
	m_nametipwnd.DestroyWindow();

	CFreeView::OnDestroy();
}

const TreemapNode *CFolderView::GetDisplayFolderFromPoint(const CPoint &point)
{
	return TreemapEngine::HitTestItem(m_layoutNodes, point.x, point.y);
}

const TreemapNode *CFolderView::GetContainerDisplayFolderFromPoint(const CPoint &point)
{
	return TreemapEngine::HitTestContainer(m_layoutNodes, point.x, point.y);
}

void CFolderView::HighlightPathAtPoint(const CPoint &point)
{
	BOOL changed = FALSE;

	for (auto &cur : m_layoutNodes) {
		if (cur.name != NULL && cur.name[0] != '<') {
			BOOL inside = point.x > cur.x && point.y > cur.y
				&& point.x < cur.x + cur.w && point.y < cur.y + cur.h;
			if (inside && !(cur.flags & TREEMAP_FLAG_HOVER)) {
				cur.flags |= TREEMAP_FLAG_HOVER;
				changed = TRUE;
			}
			else if (!inside && (cur.flags & TREEMAP_FLAG_HOVER)) {
				cur.flags &= ~TREEMAP_FLAG_HOVER;
				changed = TRUE;
			}
		}
	}

	// Nodes can't be redrawn in isolation: a folder's minimal draw clears its
	// interior, erasing every descendant already on screen. Repaint through
	// the double-buffered OnPaint instead. Hover only changes pixels when
	// rollover boxes are enabled, so skip the repaint otherwise.
	if (changed && theApp.m_settings.rollover_box) {
		InvalidateRect(NULL, FALSE);
		UpdateWindow();
	}
}

void CFolderView::OnLButtonDown(UINT flags, CPoint point)
{
	const TreemapNode *cur = GetDisplayFolderFromPoint(point);
	SelectFolder(cur);
}

void CFolderView::OnLButtonDblClk(UINT flags, CPoint point)
{
	const TreemapNode *cur = GetDisplayFolderFromPoint(point);
	if (cur != NULL) {
		if (cur->flags & TREEMAP_FLAG_FOLDER)
			ZoomIn(cur);
		else {
			CSpaceMonger *app = (CSpaceMonger *)AfxGetApp();
			app->OnFileRun();
		}
	}
}

void CFolderView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (nFlags != 0) return;

	const TreemapNode *cur = GetDisplayFolderFromPoint(point);

	if (cur != lastcur) {
		HighlightPathAtPoint(point);
		if (theApp.m_settings.show_info_tips)
			SetupInfoTip(cur);
		if (theApp.m_settings.show_name_tips)
			SetupNameTip(cur);
		lastcur = cur;
	}
}

static void PrintFileSize(CString &string, ui64 size)
{
	CString sizestring;
	ui32 displayfull, displayfractional;
	const char *displaytype;

	if (size < (ui64)(1024)) {
		displayfull = (ui32)size;
		displayfractional = 0;
		displaytype = CurLang->bytes;
	}
	else if (size < (ui64)(1024*1024)) {
		displayfull = (ui32)(size / (ui64)(1024));
		displayfractional = (ui32)(10 * (size % (ui64)(1024)) / (ui64)(1024));
		displaytype = CurLang->kb;
	}
	else if (size < (ui64)(1024*1024*1024)) {
		displayfull = (ui32)(size / (ui64)(1024*1024));
		displayfractional = (ui32)(10 * (size % (ui64)(1024*1024)) / (ui64)(1024*1024));
		displaytype = CurLang->mb;
	}
	else {
		displayfull = (ui32)(size / (ui64)(1024*1024*1024));
		displayfractional = (ui32)(10 * (size % (ui64)(1024*1024*1024)) / (ui64)(1024*1024*1024));
		displaytype = CurLang->gb;
	}
	sizestring.Format(CurLang->size_format, displayfull, displayfractional, displaytype);
	string += sizestring;
}

static void PrintDate(CString &string, ui64 time)
{
	if (time == 0) return;

	FILETIME ft;
	ft.dwLowDateTime = (DWORD)time;
	ft.dwHighDateTime = (DWORD)(time >> 32);

	FILETIME lft;
	FileTimeToLocalFileTime(&ft, &lft);

	SYSTEMTIME st;
	FileTimeToSystemTime(&lft, &st);

	char date[64], timebuf[64];
	GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, date, sizeof(date));
	GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, timebuf, sizeof(timebuf));

	string += date;
	string += " ";
	string += timebuf;
}

void CFolderView::SetupInfoTip(const TreemapNode *cur)
{
	m_infotipwnd.EnableWindow(0);

	if (cur == NULL || cur->name == NULL || cur->name[0] == '<')
		return;

	std::wstring widePath = BuildItemPathW(cur);
	std::string path = PathUtil::WideToAnsi(BuildContainerPathW(cur));
	CString string;

	WIN32_FILE_ATTRIBUTE_DATA info;
	memset(&info, 0, sizeof(info));
	GetFileAttributesExW(widePath.c_str(), GetFileExInfoStandard, &info);

	if (theApp.m_settings.infotip_flags & TIP_PATH)
		string += path.c_str();
	if (theApp.m_settings.infotip_flags & TIP_NAME)
		string += PathUtil::WideToAnsi(cur->source->names[cur->index]).c_str();
	if (theApp.m_settings.infotip_flags & (TIP_NAME|TIP_PATH))
		string += '\n';
	if (theApp.m_settings.infotip_flags & TIP_SIZE)
		PrintFileSize(string, cur->source->actualsizes[cur->index]);
	if (theApp.m_settings.infotip_flags & TIP_ATTRIB) {
		if ((theApp.m_settings.infotip_flags & TIP_SIZE)
			&& info.dwFileAttributes != 0) string += "  /  ";
		if (info.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)			string += " ", string += CurLang->attribnames[0];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)		string += " ", string += CurLang->attribnames[1];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)		string += " ", string += CurLang->attribnames[2];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)		string += " ", string += CurLang->attribnames[3];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)			string += " ", string += CurLang->attribnames[4];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE)			string += " ", string += CurLang->attribnames[5];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY)		string += " ", string += CurLang->attribnames[6];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)	string += " ", string += CurLang->attribnames[7];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)		string += " ", string += CurLang->attribnames[8];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)			string += " ", string += CurLang->attribnames[9];
		if (info.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY)		string += " ", string += CurLang->attribnames[10];
	}
	if (theApp.m_settings.infotip_flags & (TIP_ATTRIB|TIP_SIZE))
		string += '\n';
	if (theApp.m_settings.infotip_flags & TIP_DATE) {
		PrintDate(string, cur->source->times[cur->index]);
		string += '\n';
	}
	if (theApp.m_settings.infotip_flags & TIP_ICON) {
		SHFILEINFOW fileinfo;
		memset(&fileinfo, 0, sizeof(SHFILEINFOW));
		SHGetFileInfoW(widePath.c_str(), 0,
			&fileinfo, sizeof(SHFILEINFOW), SHGFI_DISPLAYNAME|SHGFI_ICON);
		m_infotipwnd.SetIcon(fileinfo.hIcon, 1);
		m_infotipwnd.SetIconPos(TW_LEFT);
	}

	m_infotipwnd.SetWindowText(string);
	m_infotipwnd.AutoSize();
	m_infotipwnd.EnableWindow(1);
	m_infotipwnd.RedrawWindow();
}

void CFolderView::SetupNameTip(const TreemapNode *cur)
{
	m_nametipwnd.EnableWindow(0);

	if (cur == NULL) return;

	CDC *pDC = GetDC();
	pDC->SelectObject(&minifont);
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
	pDC->SelectPalette(&m_palette, 0);

	int tx, ty, failed = 0;
	int len = (int)wcslen(cur->name);
	CSize size;
	::GetTextExtentPoint32W(pDC->GetSafeHdc(), cur->name, len, &size);
	int x = cur->x, y = cur->y, w = cur->w + 1, h = cur->h + 1;
	if (size.cx > w - 2) tx = x + 2;
	else if (cur->flags & 1) tx = x + 2, failed++;
	else tx = x + (w - size.cx) / 2, failed++;
	if (size.cy > h - 2) ty = y + 1;
	else if (cur->flags & 1) ty = y + 1, failed++;
	else ty = y + (h - size.cy) / 2, failed++;
	if (failed == 2) return;

	if (!(cur->flags & 1) && h >= 36 && w >= 48)
		ty -= 12;

	if (selected == cur) {
		m_nametipwnd.SetBgColor(RGB(0,0,0));
		m_nametipwnd.SetTextColor(RGB(255,255,255));
	}
	else if (cur->flags & 4) {
		m_nametipwnd.SetBgColor(BoxColors[(cur->depth & 7) + 8]);
		m_nametipwnd.SetTextColor(RGB(0,0,0));
	}
	else {
		m_nametipwnd.SetBgColor(BoxColors[cur->depth & 7]);
		m_nametipwnd.SetTextColor(RGB(0,0,0));
	}
	::SetWindowTextW(m_nametipwnd.m_hWnd, cur->name);
	m_nametipwnd.AutoSize();
	m_nametipwnd.MoveWindow(this, tx-2, ty-1);
	m_nametipwnd.PushOnScreen();

	ReleaseDC(pDC);
	m_nametipwnd.EnableWindow(1);
	m_nametipwnd.RedrawWindow();
}

static void AddMenuEntry(HMENU menu, UINT cmd = 0, const char *string = NULL,
	BOOL checked = 0, BOOL disabled = 0, BOOL bold = 0)
{
	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);
	mii.fMask = MIIM_CHECKMARKS|MIIM_ID|MIIM_STATE|MIIM_TYPE;
	mii.fType = ((string != NULL) ? MFT_STRING : MFT_SEPARATOR);
	mii.fState = 0;
	if (bold) mii.fState |= MFS_DEFAULT;
	if (checked) mii.fState |= MFS_CHECKED;
	if (disabled) mii.fState |= MFS_GRAYED|MFS_DISABLED;
	mii.wID = cmd;
	mii.hSubMenu = NULL;
	mii.hbmpChecked = NULL;
	mii.hbmpUnchecked = NULL;
	mii.dwTypeData = (char *)string;
	if (string != NULL) mii.cch = (UINT)strlen(string);
	else mii.cch = 0;

	::InsertMenuItem(menu, GetMenuItemCount(menu), TRUE, &mii);
}

void CFolderView::OnRButtonUp(UINT flags, CPoint point)
{
	const TreemapNode *cur = GetDisplayFolderFromPoint(point);
	SelectFolder(cur);
	ClientToScreen(&point);

	bool showinfo = m_infotipwnd.IsWindowEnabled();
	bool showname = m_nametipwnd.IsWindowEnabled();
	m_infotipwnd.EnableWindow(0);
	m_nametipwnd.EnableWindow(0);

	CSpaceMonger *app = (CSpaceMonger *)AfxGetApp();
	HMENU menu = ::CreatePopupMenu();
	::AddMenuEntry(menu, ID_VIEW_ZOOM_IN, CurLang->zoomin, 0, cur == NULL, (cur != NULL) && (cur->flags & 1) != 0);
	::AddMenuEntry(menu, ID_VIEW_ZOOM_OUT, CurLang->zoomout, 0, cur == NULL, 0);
	::AddMenuEntry(menu, ID_VIEW_ZOOM_FULL, CurLang->zoomfull, 0, cur == NULL, 0);
	::AddMenuEntry(menu);
	::AddMenuEntry(menu, ID_FILE_RUN, CurLang->run, 0, cur == NULL, (cur != NULL) && (cur->flags & 1) == 0);
	::AddMenuEntry(menu, ID_FILE_DELETE, CurLang->del, 0,
		!SM_CanDeleteSelection(app->m_settings.disable_delete, cur != NULL), 0);
	::AddMenuEntry(menu);
	::AddMenuEntry(menu, ID_FILE_OPEN, CurLang->opendrive, 0, 0, 0);
	::AddMenuEntry(menu, ID_FILE_REFRESH, CurLang->rescandrive, 0, 0, 0);
	::AddMenuEntry(menu, ID_VIEW_FREE, CurLang->showfreespace, showfreespace, 0, 0);
	::AddMenuEntry(menu);
	::AddMenuEntry(menu, ID_FILE_PROPERTIES, CurLang->properties, 0, 0, 0);
	::TrackPopupMenuEx(menu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
		point.x, point.y, theApp.m_mainframe->m_hWnd, NULL);
	::DestroyMenu(menu);

	if (showname) m_nametipwnd.EnableWindow(1);
	if (showinfo) m_infotipwnd.EnableWindow(1);
}

void CFolderView::SelectFolder(const TreemapNode *cur)
{
	if (cur == selected) return;

	selected = cur;

	// See HighlightPathAtPoint: nodes can't be redrawn in isolation.
	InvalidateRect(NULL, FALSE);
	UpdateWindow();
	UpdateTitleBar();
}

static CString GetSizeString(ui64 size, ui64 totalspace, BOOL percent)
{
	CString result;

	if (totalspace == 0) totalspace = (ui64)(-1);

	if (percent) {
		size = (size * (ui64)1000) / totalspace;
		result.Format(CurLang->percent_format, (ui32)size / 10, (ui32)size % 10);
	}
	else {
		ui32 displayfull, displayfractional;
		const char *displaytype;

		if (size < (ui64)(1024)) {
			displayfull = (ui32)size;
			displayfractional = 0;
			displaytype = CurLang->bytes;
		}
		else if (size < (ui64)(1024*1024)) {
			displayfull = (ui32)(size / (ui64)(1024));
			displayfractional = (ui32)(10 * (size % (ui64)(1024)) / (ui64)(1024));
			displaytype = CurLang->kb;
		}
		else if (size < (ui64)(1024*1024*1024)) {
			displayfull = (ui32)(size / (ui64)(1024*1024));
			displayfractional = (ui32)(10 * (size % (ui64)(1024*1024)) / (ui64)(1024*1024));
			displaytype = CurLang->mb;
		}
		else {
			displayfull = (ui32)(size / (ui64)(1024*1024*1024));
			displayfractional = (ui32)(10 * (size % (ui64)(1024*1024*1024)) / (ui64)(1024*1024*1024));
			displaytype = CurLang->gb;
		}
		result.Format(CurLang->size_format, displayfull, displayfractional, displaytype);
	}

	return result;
}

void CFolderView::UpdateTitleBar(void)
{
	CMainFrame *mainfrm = (CMainFrame *)GetTopLevelFrame();
	if (mainfrm == NULL) return;

	mainfrm->m_toolbar.UpdateButtonsForView(this);

	CFolderTree *ft = (CFolderTree *)GetDocument();
	if (ft == NULL || rootfolder == NULL) {
		mainfrm->SetWindowText("SpaceMonger");
		return;
	}

	CString title = "";
	ui64 size;

	if (selected != NULL && selected->name != NULL) {
		title += ft->m_path;
		BuildTitleReverse(selected->source, title);
		title += PathUtil::WideToAnsi(selected->source->names[selected->index]).c_str();
		size = selected->source->sizes[selected->index];
		title += "  -  " + GetSizeString(size, ft->totalspace, 1)
			+ "  -  " + GetSizeString(size, ft->totalspace, 0) + "  -  ";
	}
	else if (rootfolder != NULL) {
		title += ft->m_path;
		BuildTitleReverse(rootfolder, title);
		if (rootfolder->parent == NULL) size = ft->totalspace; // Kludge
		else size = rootfolder->SizeTotal();
		title += "  -  " + GetSizeString(size, ft->totalspace, 0)
			+ " " + CurLang->total + "  -  " + GetSizeString(ft->freespace, ft->freespace, 0)
			+ " " + CurLang->free + "  -  ";
	}

	title += "SpaceMonger";

	mainfrm->SetWindowText(title);
}	

void CFolderView::BuildTitleReverse(CFolder *folder, CString &string)
{
	if (folder->parent != NULL)
		BuildTitleReverse(folder->parent, string);
	if (folder->parent == NULL) return;

	string += PathUtil::WideToAnsi(folder->parent->names[folder->parentindex]).c_str();
	string += "\\";
}

void CFolderView::BuildTitleReverseW(CFolder *folder, std::wstring& string)
{
	if (folder->parent != NULL)
		BuildTitleReverseW(folder->parent, string);
	if (folder->parent == NULL) return;

	PathUtil::AppendComponent(string, folder->parent->names[folder->parentindex]);
	if (!string.empty() && string.back() != L'\\')
		string += L'\\';
}

std::wstring CFolderView::BuildContainerPathW(const TreemapNode *folder)
{
	CFolderTree *tree = (CFolderTree *)GetDocument();
	std::wstring relativePath;

	if (folder != NULL)
		BuildTitleReverseW(folder->source, relativePath);

	return PathUtil::BuildWidePath(tree == NULL ? "" : tree->m_path, relativePath, NULL);
}

std::wstring CFolderView::BuildItemPathW(const TreemapNode *folder)
{
	CFolderTree *tree = (CFolderTree *)GetDocument();
	std::wstring relativePath;
	const wchar_t *leafName = NULL;

	if (folder != NULL) {
		BuildTitleReverseW(folder->source, relativePath);
		if (folder->source != NULL && folder->index != (ui32)-1)
			leafName = folder->source->names[folder->index];
	}

	return PathUtil::BuildWidePath(tree == NULL ? "" : tree->m_path, relativePath, leafName);
}

void CFolderView::OnDraw(CDC *pDC)
{
	pDC->SelectObject(&minifont);
	pDC->SetBkMode(TRANSPARENT);
	pDC->SetTextAlign(TA_LEFT | TA_TOP | TA_NOUPDATECP);
	pDC->SelectPalette(&m_palette, 0);
	pDC->RealizePalette();

	DrawBox(pDC->m_hDC, ColorFlat, 0, 0, m_width, m_height);
	FillBox(pDC->m_hDC, ColorFlat, 1, 1, m_width-2, m_height-2);

	for (const auto &cur : m_layoutNodes) {
		MinimalDrawDisplayFolder(pDC, &cur, selected == &cur);
	}
}

void CFolderView::MinimalDrawDisplayFolder(CDC *pDC, const TreemapNode *cur, BOOL sel)
{
	int x, y, w, h;

	x = cur->x, y = cur->y, w = cur->w + 1, h = cur->h + 1;

	if (cur->depth != -1)
		DrawBox(pDC->m_hDC, black, x, y, w, h);

	if (w > 2 && h > 2) {
		COLORREF color, bright, dark;
		CBrush brush, bbrush, dbrush;
		int colortype = (cur->flags & 1) ? theApp.m_settings.folder_color
			: theApp.m_settings.file_color;

		if (sel && !(cur->flags & 2))
			color = bright = dark = RGB(0,0,0);
		else if (cur->depth != -1) {
			if (colortype == 0) {
				color = BoxColors[cur->depth & 7];
				bright = BoxColors[(cur->depth & 7) + 8];
				dark = BoxColors[(cur->depth & 7) + 16];
			}
			else if (colortype == 1) {
				color = GetSysColor(COLOR_3DFACE);
				bright = GetSysColor(COLOR_3DHILIGHT);
				dark = GetSysColor(COLOR_3DSHADOW);
			}
			else {
				color = FixedColors[colortype-2];
				bright = FixedColors[colortype-2 + 10];
				dark = FixedColors[colortype-2 + 20];
			}
			if (theApp.m_settings.rollover_box) {
				if (cur->flags & 4) dark = color, color = bright, bright = RGB(255,255,255);
				else bright = color, color = dark;
			}
		}
		else color = bright = dark = GetSysColor(COLOR_3DFACE);

		brush.CreateSolidBrush(color);
		bbrush.CreateSolidBrush(bright);
		dbrush.CreateSolidBrush(dark);

		DrawDualBox(pDC->m_hDC, bbrush, dbrush, x + 1, y + 1, w - 2, h - 2);
		if (cur->flags & 1) {
			DrawBox(pDC->m_hDC, brush, x + 2, y + 2, w - 4, h - 4);
			FillBox(pDC->m_hDC, brush, x + 3, y + 3, w - 6, 9);
			if (h > 15 && w > 6) {
				FillBox(pDC->m_hDC, ColorFlat, x + 3, y + 12, w - 6, h - 15);
			}
		}
		else FillBox(pDC->m_hDC, brush, x + 2, y + 2, w - 4, h - 4);
	}

	if (cur->name != NULL) {
		CRgn rgn;
		rgn.CreateRectRgn(x, y, x + w, y + h);
		pDC->SelectClipRgn(&rgn);

		int len = (int)wcslen(cur->name);
		CSize size;
		int tx, ty;

		::GetTextExtentPoint32W(pDC->GetSafeHdc(), cur->name, len, &size);
		if (size.cx > w - 2 || (cur->flags & 1)) tx = x + 2;
		else tx = x + (w - size.cx) / 2;
		if (size.cy > h - 2 || (cur->flags & 1)) ty = y + 1;
		else ty = y + (h - size.cy) / 2;

		if (cur->flags & 2) {
			// There's only one free-space block, so we can afford to
			// be a little less efficient with it.
			CFolderTree *ft = (CFolderTree *)GetDocument();
			if (ft != NULL) {
				CString string;
				ui64 ts = ft->totalspace;
				if (ts <= 1) ts = 1;
				si32 freepercent = (si32)(ft->freespace * (ui64)1000 / ts);
				string.Format(CurLang->freespace_format, freepercent / 10, freepercent % 10);
				size = pDC->GetTextExtent(string);
				if (size.cx > w-2) tx = x + 2;
				else tx = x + (w - size.cx) / 2;
				pDC->TextOut(tx, ty-18, string);

				string = GetSizeString(ft->freespace, ft->totalspace, 0) + " " + CurLang->free;
				pDC->TextOut(tx, ty-6, string);
				
				string.Format("%s  %u", (const char*)CString(CurLang->files_total), ft->numfiles);
				pDC->TextOut(tx, ty+6, string);
				
				string.Format("%s  %u", (const char*)CString(CurLang->folders_total), ft->numfolders);
				pDC->TextOut(tx, ty+15, string);
			}
		}
		else {
			if (sel) pDC->SetTextColor(RGB(0xFF,0xFF,0xFF));
			if (!(cur->flags & 1) && h >= 36 && w >= 48 && cur->source != NULL && cur->index != (ui32)-1) {
				// Enough room (probably) for the date and file size
				CString string;
				CSize size;
				string.Empty();
				PrintFileSize(string, cur->source->actualsizes[cur->index]);
				size = pDC->GetTextExtent(string);
				if (size.cx > w-2)
					pDC->TextOut(x+2, ty+1, string);
				else pDC->TextOut(x+(w-size.cx)/2, ty+1, string);
				string.Empty();
				PrintDate(string, cur->source->times[cur->index]);
				size = pDC->GetTextExtent(string);
				if (size.cx > w-2)
					pDC->TextOut(x+2, ty+11, string);
				else pDC->TextOut(x+(w-size.cx)/2, ty+11, string);
				ty -= 12;
			}
			::TextOutW(pDC->GetSafeHdc(), tx, ty, cur->name, len);
			if (sel) pDC->SetTextColor(RGB(0,0,0));
		}
		pDC->SelectClipRgn(NULL);
	}
}

BOOL CFolderView::OnEraseBkgnd(CDC* pDC)
{
	// Suppress background erasing to prevent flicker (we paint entire client area)
	return TRUE;
}

void CFolderView::OnPaint()
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(&rect);
	int w = rect.Width();
	int h = rect.Height();
	if (w <= 0 || h <= 0) return;

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, w, h);
	CBitmap *oldBitmap = memDC.SelectObject(&bitmap);

	OnDraw(&memDC);

	dc.BitBlt(0, 0, w, h, &memDC, 0, 0, SRCCOPY);
	memDC.SelectObject(oldBitmap);
}

void CFolderView::UpdateLiveScanLayout(AsyncScanEngine& engine, ui64 totalspace, ui64 freespace)
{
	CRect rect;
	GetClientRect(&rect);
	m_width = rect.right - rect.left;
	m_height = rect.bottom - rect.top;

	if (m_width <= 0 || m_height <= 0) return;

	TreemapConfig config;
	config.density = theApp.m_settings.density;
	config.bias = theApp.m_settings.bias;
	config.showFreeSpace = (showfreespace != 0);
	config.dpi = (int)GetWindowDpi(m_hWnd);

	selected = NULL;
	lastcur = NULL;

	engine.GenerateLiveLayout(m_width - 1, m_height - 1, totalspace, freespace, config, m_layoutNodes, m_liveNameStorage);

	// OnPaint already double-buffers, so route through the normal paint path.
	InvalidateRect(NULL, FALSE);
	UpdateWindow();
}

void CFolderView::ClearLiveScanLayout(void)
{
	m_layoutNodes.clear();
	m_liveNameStorage.clear();
	selected = NULL;
	lastcur = NULL;

	if (::IsWindow(m_hWnd)) {
		InvalidateRect(NULL, FALSE);
		UpdateWindow();
	}
}

void CFolderView::SetDocument(CFreeDoc *doc)
{
	CFreeView::SetDocument(doc);

	if (doc != NULL)
		rootfolder = ((CFolderTree *)doc)->GetRoot();
	else rootfolder = NULL;
	zoomlevel = 0;

	UpdateTitleBar();
	OnSize(0, m_width, m_height);
	CDC *dc = GetDC();
	OnDraw(dc);
	ReleaseDC(dc);
}

void CFolderView::OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized)
{
	if (nState == WA_ACTIVE || nState == WA_CLICKACTIVE)
		SetPalette();
}

void CFolderView::SetPalette(void)
{
	CDC *dc = GetDC();
	dc->SelectPalette(&m_palette, 0);
	dc->RealizePalette();
	ReleaseDC(dc);
}

void CFolderView::OnUpdate(CFreeDoc *doc)
{
	UpdateTitleBar();

	OnSize(0, m_width, m_height);

	CDC *dc = GetDC();
	OnDraw(dc);
	ReleaseDC(dc);

	m_infotipwnd.SetShowDelay(theApp.m_settings.infotip_delay);
	m_nametipwnd.SetShowDelay(theApp.m_settings.nametip_delay);
}

void CFolderView::OnSize(UINT nType, int cx, int cy)
{
	CRect rect;
	GetClientRect(&rect);
	m_width = cx = rect.right - rect.left;
	m_height = cy = rect.bottom - rect.top;

	m_layoutNodes.clear();
	m_liveNameStorage.clear();
	selected = NULL;
	lastcur = NULL;

	if (rootfolder == NULL) {
		if (GetDocument() == NULL) return;
		rootfolder = ((CFolderTree *)GetDocument())->GetRoot();
		zoomlevel = 0;
		if (rootfolder == NULL) return;
	}

	TreemapConfig config;
	config.density = theApp.m_settings.density;
	config.bias = theApp.m_settings.bias;
	config.showFreeSpace = (showfreespace != 0);
	config.dpi = (int)GetWindowDpi(m_hWnd);

	TreemapEngine::ComputeLayout(0, 0, cx - 1, cy - 1, rootfolder, zoomlevel, config, m_layoutNodes);
}

void CFolderView::ZoomIn(const TreemapNode *folder)
{
	CFolderTree *doc = (CFolderTree *)GetDocument();
	CFolder *oldroot = rootfolder;

	if (folder != NULL && folder->source != NULL && folder->index != (ui32)-1 && folder->source->children[folder->index] != NULL)
		rootfolder = folder->source->children[folder->index];

	if (rootfolder == oldroot) return;

	CRect start(folder->x, folder->y,
		folder->x+folder->w, folder->y+folder->h);
	CRect end;
	GetClientRect(&end);
	end.OffsetRect(-end.left, -end.top);
	if (theApp.m_settings.animated_zoom) AnimateBox(start, end);

	CFolder *parent = rootfolder->parent;
	zoomlevel = 0;
	while (parent != NULL) zoomlevel++, parent = parent->parent;
	OnUpdate(doc);
	UpdateTitleBar();
}

void CFolderView::ZoomOut(void)
{
	CFolderTree *doc = (CFolderTree *)GetDocument();
	CFolder *oldroot = rootfolder;

	if (rootfolder != NULL && rootfolder->parent != NULL)
		rootfolder = rootfolder->parent;

	if (rootfolder == oldroot) return;

	CRect start;
	GetClientRect(&start);
	start.OffsetRect(-start.left, -start.top);
	CRect end;
	end.left = end.right = start.right / 2;
	end.top = end.bottom = start.bottom / 2;
	if (theApp.m_settings.animated_zoom) AnimateBox(start, end);

	zoomlevel--;
	OnUpdate(doc);
	UpdateTitleBar();
}

void CFolderView::ZoomFull(void)
{
	CFolderTree *doc = (CFolderTree *)GetDocument();
	CFolder *oldroot = rootfolder;

	if (doc != NULL)
		rootfolder = ((CFolderTree *)doc)->GetRoot();
	else rootfolder = NULL;

	if (rootfolder == oldroot) return;

	CRect start;
	GetClientRect(&start);
	start.OffsetRect(-start.left, -start.top);
	CRect end;
	end.left = end.right = start.right / 2;
	end.top = end.bottom = start.bottom / 2;
	if (theApp.m_settings.animated_zoom) AnimateBox(start, end);

	zoomlevel = 0;
	OnUpdate(doc);
	UpdateTitleBar();
}

void CFolderView::ShowFreeSpace(BOOL shown)
{
	if (showfreespace == shown) return;
	showfreespace = shown;
	OnUpdate(GetDocument());
}

static void ComputeNewRect(CRect &result, const CRect &start, const CRect &end, ui32 max, ui32 step)
{
	if (max <= 0) max = 1;
	result.left = ((start.left * (max - step)) + (end.left * step)) / max;
	result.top = ((start.top * (max - step)) + (end.top * step)) / max;
	result.right = ((start.right * (max - step)) + (end.right * step)) / max;
	result.bottom = ((start.bottom * (max - step)) + (end.bottom * step)) / max;
}

static void MinimalRect(CDC *dc, const CRect &rect)
{
	dc->MoveTo(rect.left, rect.top);
	dc->LineTo(rect.right, rect.top);
	dc->LineTo(rect.right, rect.bottom);
	dc->LineTo(rect.left, rect.bottom);
	dc->LineTo(rect.left, rect.top);
}

void CFolderView::AnimateBox(const CRect &start, const CRect &end)
{
	// Total time of animation: 400 milliseconds + render time, or
	// about a half a second.

	ui32 step;
	CRect rect = start;
	CDC *dc = GetDC();
	dc->SetROP2(R2_NOT);
	for (step = 0; step < 8; step++) {
		ComputeNewRect(rect, start, end, 8, step);
		MinimalRect(dc, rect);
		Sleep(25);
	}
	for (step = 0; step < 8; step++) {
		ComputeNewRect(rect, start, end, 8, step);
		MinimalRect(dc, rect);
		Sleep(25);
	}
	dc->SetROP2(R2_COPYPEN);
	ReleaseDC(dc);
}

void CFolderView::OnIgnoreUpdate(CCmdUI *ui)
{
	GeneralIgnoreUpdate(ui);
}
