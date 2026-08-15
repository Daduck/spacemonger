#include "stdafx.h"
#include "spacemonger.h"
#include "FolderTree.h"
#include "FolderView.h"
#include "AsyncScanEngine.h"
#include "PathUtil.h"
#include "Lang.h"
#include <cmath>

CFolderTree::CFolderTree()
{
	root = cur = NULL;
	freespace = usedspace = totalspace = 0;
	m_path = "";
}

CFolderTree::~CFolderTree()
{
	if (root != NULL) delete root;
}

BOOL CFolderTree::LoadTree(const CString &path, BOOL includespace, CWnd *modalwin)
{
	CFolderDialog dialog;

	if (path == "") return 1;

	dialog.Create(IDD_SCAN_DIALOG, modalwin);
	dialog.Reset();

	m_path = path;
	GetSpace(path);
	if (root != NULL) { delete root; root = NULL; }
	nameArena.Reset();
	cur = NULL;
	filespace = 0;

	std::wstring widePath = PathUtil::AnsiToWide((LPCTSTR)path);
	std::wstring absPath = PathUtil::GetAbsolutePath(widePath);
	absPath = PathUtil::EnsureTrailingBackslash(absPath);
	std::wstring preparedPath = PathUtil::PrepareLongPath(absPath);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);
	BOOL aligned = (clustersize != 0 && (clustersize & (clustersize - 1)) == 0);

	AsyncScanEngine engine;
	if (!engine.StartScan(preparedPath, clustersize - 1, aligned)) {
		dialog.DestroyWindow();
		return 0;
	}

	// Pumps pending messages; returns 0 if WM_QUIT arrived (scan is torn down).
	auto pumpMessages = [&]() -> BOOL {
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				engine.Cancel();
				engine.WaitForCompletion();
				dialog.DestroyWindow();
				::PostQuitMessage((int)msg.wParam);
				return 0;
			}
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		return 1;
	};

	DWORD last_ui_tick = 0;
	DWORD last_render_tick = 0;
	ui64 last_render_bytes = (ui64)-1;
	ui64 last_render_entries = (ui64)-1;
	while (engine.IsScanning()) {
		if (!pumpMessages()) return 0;

		if (dialog.cancelled) {
			engine.Cancel();
			break;
		}

		DWORD now = ::GetTickCount();
		if (now - last_ui_tick >= 50) {
			last_ui_tick = now;
			dialog.UpdateFromProgress(engine.GetProgress(), usedspace);
		}

		if (now - last_render_tick >= 250) {
			last_render_tick = now;
			CFolderView *fv = (CFolderView *)theApp.m_view;
			if (fv != NULL && ::IsWindow(fv->m_hWnd)) {
				// Re-walking and re-laying-out the tree stalls the workers
				// (it holds the tree lock), so skip it when nothing changed.
				ScanProgress progress = engine.GetProgress();
				ui64 entries = progress.numFiles + progress.numFolders;
				if (progress.bytesScanned != last_render_bytes || entries != last_render_entries) {
					last_render_bytes = progress.bytesScanned;
					last_render_entries = entries;
					fv->UpdateLiveScanLayout(engine, totalspace, freespace);
				}
			}
		}

		::Sleep(15);
	}

	// Keep pumping while the workers wind down so the UI stays responsive even
	// if one of them is stuck in a slow filesystem call.
	engine.RequestStop();
	while (!engine.IsFinished()) {
		if (!pumpMessages()) return 0;
		if (dialog.cancelled) engine.Cancel();
		::Sleep(10);
	}

	engine.WaitForCompletion();

	if (dialog.cancelled || engine.IsCancelled()) {
		// The view may still show the last live-scan frame; drop it so a
		// dead scan's half-built map doesn't linger on screen.
		CFolderView *fv = (CFolderView *)theApp.m_view;
		if (fv != NULL && ::IsWindow(fv->m_hWnd)) {
			fv->ClearLiveScanLayout();
		}
		if (root != NULL) { delete root; root = NULL; }
		nameArena.Reset();
		root = cur = NULL;
		freespace = usedspace = totalspace = 0;
		m_path = "";
		dialog.DestroyWindow();
		return 0;
	}

	root = engine.DetachResult(nameArena);
	if (root == NULL) {
		nameArena.Reset();
		root = cur = NULL;
		freespace = usedspace = totalspace = 0;
		m_path = "";
		dialog.DestroyWindow();
		return 0;
	}

	cur = root;
	ScanProgress finalProgress = engine.GetProgress();
	numfiles = finalProgress.numFiles;
	numfolders = finalProgress.numFolders;
	filespace = finalProgress.bytesScanned;

	dialog.UpdateFromProgress(finalProgress, usedspace);

	if (includespace
		&& !root->AddFileWithArena(nameArena, L"<<<<<<<<<<<<<<<<<<<<", 1, freespace, freespace, 0)) {
		delete root;
		nameArena.Reset();
		root = cur = NULL;
		freespace = usedspace = totalspace = 0;
		m_path = "";
		dialog.DestroyWindow();
		return 0;
	}

	root->Finalize();
	dialog.DestroyWindow();

	if (modalwin != NULL && ::IsWindow(modalwin->m_hWnd)) {
		modalwin->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	}

	return 1;
}

void CFolderTree::GetSpace(const CString &path)
{
	DWORD SecPerClus = 0, BytesPerSec = 0, ClusPerDisk = 0, FreeClus = 0;
	ui64 oddfree = 0, total = 0, totalfree = 0;
	BOOL gotclusters;

	typedef BOOL (WINAPI *GetFreeDiskSpaceExFunc)(LPCTSTR pathname,
		ui64 *oddfree, ui64 *total, ui64 *totalfree);

	// First, compute the cluster size (which will be needed later)
	gotclusters = GetDiskFreeSpace(path, &SecPerClus, &BytesPerSec, &FreeClus, &ClusPerDisk);
	clustersize = gotclusters ? (ui64)BytesPerSec * (ui64)SecPerClus : 1;

	// Next, load in Kernel32 and use GetDiskFreeSpaceEx to find out
	// the size of the disk. If GetDiskFreeSpaceEx doesn't exist, then
	// fall back on the values from GetDiskFreeSpace.
	HINSTANCE hLibrary = LoadLibrary("KERNEL32.DLL");
	GetFreeDiskSpaceExFunc getfreediskspaceex =
		hLibrary == NULL ? NULL : (GetFreeDiskSpaceExFunc)GetProcAddress(hLibrary, "GetDiskFreeSpaceExA");
	if (getfreediskspaceex != NULL && getfreediskspaceex(path, &oddfree, &total, &totalfree)) {
		freespace = totalfree;
		totalspace = total;
	}
	else if (gotclusters) {
		freespace = clustersize * (ui64)FreeClus;
		totalspace = clustersize * (ui64)ClusPerDisk;
	}
	else {
		freespace = totalspace = 0;
	}
	if (hLibrary != NULL) FreeLibrary(hLibrary);

	usedspace = totalspace - freespace;
}

CFolder *CFolderTree::GetRoot(void)
{
	return(root);
}

CFolder *CFolderTree::SetCur(const CString &path)
{
	return(cur);
}

CFolder *CFolderTree::GetCur(void)
{
	return(cur);
}

CFolder *CFolderTree::Down(unsigned int index)
{
	if (index < cur->cur) {
		CFolder *newfolder = cur->children[index];
		if (newfolder != NULL) {
			cur = newfolder;
			return(cur);
		}
	}
	return(NULL);
}

CFolder *CFolderTree::Up(void)
{
	if (cur != root) {
		cur = cur->parent;
		return(cur);
	}
	return(NULL);
}

//////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFolderDialog, CDialog)

BEGIN_MESSAGE_MAP(CFolderDialog, CDialog)
	//{{AFX_MSG_MAP(CFolderDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CFolderDialog::CFolderDialog()
{
	cur_frame = 0;
	cancelled = 0;
}

CFolderDialog::~CFolderDialog()
{
}

void CFolderDialog::OnCancel(void)
{
	cancelled = 1;
	CDialog::OnCancel();
}

void CFolderDialog::Reset(void)
{
	if (!IsWindow(m_hWnd)) return;

	cur_frame = 0;
	cancelled = 0;

	SetWindowText(CurLang->scanning);
	SetDlgItemText(IDC_STATIC_FILESFOUND, CurLang->files_found);
	SetDlgItemText(IDC_STATIC_FOLDERSFOUND, CurLang->folders_found);
	SetDlgItemText(IDCANCEL, CurLang->cancel);
	SetDlgItemText(IDC_PATH, "");
	SetDlgItemInt(IDC_FILES, 0);
	SetDlgItemInt(IDC_FOLDERS, 0);
	CProgressCtrl *progress = (CProgressCtrl *)GetDlgItem(IDC_LOAD_PROGRESS);
	if (progress != NULL) {
		progress->SetRange(0, 4096);
		progress->SetPos(0);
	}

	DrawScanAnimation(0);
}

void CFolderDialog::DrawScanAnimation(ui32 frame)
{
	CClientDC dc(this);
	CDC srcdc;
	srcdc.CreateCompatibleDC(&dc);
	::SelectObject(srcdc.m_hDC, theApp.m_scan_animation[frame]);
	CRect rect;
	GetClientRect(&rect);
	dc.BitBlt(8, rect.bottom - rect.top - (48 + 8) - 12, 128, 48, &srcdc, 0, 0, SRCCOPY);
}

void CFolderDialog::UpdateFromProgress(const ScanProgress& progress, ui64 usedspace)
{
	if (!IsWindow(m_hWnd)) return;

	std::string ansiPath = PathUtil::WideToAnsi(PathUtil::RemoveLongPathPrefix(progress.currentPath));

	SetDlgItemText(IDC_PATH, ansiPath.c_str());
	SetDlgItemInt(IDC_FILES, (UINT)progress.numFiles);
	SetDlgItemInt(IDC_FOLDERS, (UINT)progress.numFolders);

	CProgressCtrl *ctrl = (CProgressCtrl *)GetDlgItem(IDC_LOAD_PROGRESS);
	if (ctrl != NULL) {
		int pos = 0;
		if (progress.isComplete) {
			pos = 4096;
		} else if (usedspace > 0 && progress.bytesScanned > 0) {
			double ratio = (double)progress.bytesScanned / (double)usedspace;
			if (ratio > 1.0) ratio = 1.0;
			if (ratio < 0.0) ratio = 0.0;
			// Use a perceptual power curve (ratio^1.5) to prevent front-loaded large files
			// from prematurely filling the progress bar before directory walking completes
			double smoothed = pow(ratio, 1.5);
			pos = (int)(smoothed * 4000.0);
			if (pos > 4000) pos = 4000;
		}
		ctrl->SetPos(pos);
	}

	cur_frame = (cur_frame + 1) & 7;
	if (!(cur_frame & 1)) {
		DrawScanAnimation(cur_frame >> 1);
	}
}
