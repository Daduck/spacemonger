
#include "stdafx.h"
#include "spacemonger.h"
#include "FolderTree.h"
#include "DiskUsage.h"
#include "FolderEntryArrays.h"
#include "PathUtil.h"
#include "Lang.h"
#include <vector>
#include <algorithm>

#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <winioctl.h>

//////////////////////////////////////////////////////////////////////////////
//  Reparse points are handled via WIN32_FIND_DATA dwReserved0 in modern Windows.


//////////////////////////////////////////////////////////////////////////////

static void AssignFolderEntryArrays(CFolder *folder, const CFolderEntryArrays& arrays)
{
	folder->names = arrays.names;
	folder->sizes = arrays.sizes;
	folder->actualsizes = arrays.actualsizes;
	folder->children = arrays.children;
	folder->times = arrays.times;
}

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

static void PseudoSleep(ui32 time)
{
	ui32 starttime = GetTickCount();
	MSG msg;
	while (GetTickCount() - starttime < time) {
		while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
}

BOOL CFolderTree::LoadTree(const CString &path, BOOL includespace, CWnd *modalwin)
{
	CFolderDialog dialog;

	if (path == "") return 1;

	dialog.Create(IDD_SCAN_DIALOG, modalwin);
	dialog.Reset();

	m_path = path;
	GetSpace(path);
	if (root != NULL) delete root;
	nameArena.Reset();
	root = new CFolder;
	cur = root;
	filespace = 0;

	if (!root->LoadFolderInitial(this, path, clustersize, &dialog)) {
		root->Finalize();
		if (root != NULL) delete root;
		nameArena.Reset();
		root = cur = NULL;
		freespace = usedspace = totalspace = 0;
		m_path = "";
		return 0;
	}
	else {
		dialog.ForcedUpdate(this);
		numfiles = dialog.numfiles;
		numfolders = dialog.numfolders;
		if (includespace
			&& !root->AddFile(this, L"<<<<<<<<<<<<<<<<<<<<", 1, freespace, freespace, 0)) {
			delete root;
			nameArena.Reset();
			root = cur = NULL;
			freespace = usedspace = totalspace = 0;
			m_path = "";
			return 0;
		}
		root->Finalize();
		::PseudoSleep(1000);
		modalwin->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
		return 1;
	}
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
	// the size of the disk.  If GetDiskFreeSpaceEx doesn't exist, then
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

static wchar_t *arena_wcsdup(CStringArena& arena, const wchar_t *string, int stringlen)
{
	wchar_t *newstr = arena.Allocate(stringlen+1);
	if (newstr == NULL) return(NULL);

	wchar_t *src;
	wchar_t c;
	BOOL allupper = 1;
	src = newstr;
	while (stringlen--) {
		c = *src++ = *string++;
		if (c >= L'a' && c <= L'z') allupper = 0;
	}
	*src = L'\0';
	if (allupper && src != newstr) {
		src = newstr+1;
		while ((c = *src) != L'\0') {
			if (c >= L'A' && c <= L'Z') *src += 32;
			src++;
		}
	}

	return(newstr);
}

CFolder::CFolder()
{
	cur = 0;
	max = 0;
	names = NULL;
	sizes = NULL;
	actualsizes = NULL;
	children = NULL;
	times = NULL;
	size_self = size_children = 0;
	parent = NULL;
	parentindex = 0;

	CFolderEntryArrays arrays;
	if (SM_AllocateFolderEntryArrays(&arrays, 32)) {
		AssignFolderEntryArrays(this, arrays);
		max = 32;
	}
}

CFolder::~CFolder()
{
	unsigned int i;

	for (i = 0; i < cur; i++) {
		if (children[i] != NULL) delete children[i];
	}

	free(names);
	free(children);
	free(sizes);
	free(actualsizes);
	free(times);

	cur = max = 0;
	size_self = size_children = 0;
	parent = NULL;
	parentindex = 0;
}

BOOL CFolder::MoreEntries(void)
{
	if (max > ((unsigned int)-1) / 2) return 0;

	unsigned int newmax = (max == 0) ? 32 : max * 2;
	CFolderEntryArrays arrays;
	if (!SM_AllocateFolderEntryArrays(&arrays, newmax)) return 0;

	if (cur != 0) {
		memcpy(arrays.names, names, cur * sizeof(wchar_t *));
		memcpy(arrays.sizes, sizes, cur * sizeof(ui64));
		memcpy(arrays.times, times, cur * sizeof(ui64));
		memcpy(arrays.actualsizes, actualsizes, cur * sizeof(ui64));
		memcpy(arrays.children, children, cur * sizeof(CFolder *));
	}

	free(names);
	free(sizes);
	free(times);
	free(actualsizes);
	free(children);
	AssignFolderEntryArrays(this, arrays);

	max = newmax;
	return 1;
}

BOOL CFolder::AddFile(CFolderTree *tree, const wchar_t *name, ui32 namelen, ui64 size, ui64 actual_size, ui64 time)
{
	if (cur >= max && !MoreEntries()) return 0;

	names[cur] = arena_wcsdup(tree->nameArena, name, namelen);
	if (names[cur] == NULL) return 0;
	actualsizes[cur] = actual_size;
	times[cur] = time;
	size_self += (sizes[cur] = size);
	children[cur] = NULL;
	cur++;
	tree->filespace += size;
	return 1;
}

BOOL CFolder::AddFolder(CFolderTree *tree, const wchar_t *name, ui32 namelen, CFolder *folder, ui64 time)
{
	if (cur >= max && !MoreEntries()) return 0;

	names[cur] = arena_wcsdup(tree->nameArena, name, namelen);
	if (names[cur] == NULL) return 0;
	size_children += (sizes[cur] = folder->SizeTotal());
	actualsizes[cur] = sizes[cur];
	times[cur] = time;
	children[cur] = folder;
	folder->parent = this;
	folder->parentindex = cur;
	cur++;
	return 1;
}

// EightBitCountingSort
//   Performs an 8-bit counting sort on the given data.  O(n).

void CFolder::EightBitCountingSort(ui64 *dsize, ui64 *ssize, ui32 count, ui32 bitpos,
		wchar_t **dnames, wchar_t **snames, CFolder **dkids, CFolder **skids,
		ui64 *dasize, ui64 *sasize, ui64 *dtimes, ui64 *stimes)
{
	ui32 countarray[257];
	ui32 i, dest;

#define VALUE(size) (0xFF - (ui32)((size) >> bitpos) & 0xFF)

	// Initially, zero offsets for each
	for (i = 0; i < 256; i++)
		countarray[i] = 0;

	// Count how many of each value we have
	for (i = 0; i < count; i++)
		countarray[VALUE(ssize[i]) + 1]++;

	// Create array offsets for each
	for (i = 1; i < 256; i++)
		countarray[i] += countarray[i-1];

	// Now move them into place
	for (i = 0; i < count; i++) {
		dest = countarray[VALUE(ssize[i])]++;
		dsize[dest] = ssize[i];
		dnames[dest] = snames[i];
		dkids[dest] = skids[i];
		dasize[dest] = sasize[i];
		dtimes[dest] = stimes[i];
	}
}

// Finalize
//   Sorts the directory by file size.  O(n)!

void CFolder::Finalize(void)
{
	if (cur <= 1) return;

	if (cur < 512) {
		std::vector<ui32> indices(cur);
		for (ui32 i = 0; i < cur; i++) indices[i] = i;
		std::sort(indices.begin(), indices.end(), [&](ui32 a, ui32 b) {
			return sizes[a] > sizes[b];
		});

		for (ui32 i = 0; i < cur; i++) {
			if (indices[i] != i) {
				ui32 j = i;
				wchar_t* t_name = names[j];
				ui64 t_size = sizes[j];
				ui64 t_asize = actualsizes[j];
				CFolder* t_kid = children[j];
				ui64 t_time = times[j];

				while (indices[j] != i) {
					ui32 k = indices[j];
					names[j] = names[k];
					sizes[j] = sizes[k];
					actualsizes[j] = actualsizes[k];
					children[j] = children[k];
					times[j] = times[k];

					indices[j] = j;
					j = k;
				}
				names[j] = t_name;
				sizes[j] = t_size;
				actualsizes[j] = t_asize;
				children[j] = t_kid;
				times[j] = t_time;
				indices[j] = j;
			}
		}
	} else {
		CFolderEntryArrays arrays;
		if (!SM_AllocateFolderEntryArrays(&arrays, cur)) goto update_parent_indexes;

		// We do a radix sort with internal counting sort.  This means
		// that we pass over the source data exactly 16*cur times to sort it.
		EightBitCountingSort(arrays.sizes, sizes, cur, 0,  arrays.names, names, arrays.children, children, arrays.actualsizes, actualsizes, arrays.times, times);
		EightBitCountingSort(sizes, arrays.sizes, cur, 8,  names, arrays.names, children, arrays.children, actualsizes, arrays.actualsizes, times, arrays.times);
		EightBitCountingSort(arrays.sizes, sizes, cur, 16, arrays.names, names, arrays.children, children, arrays.actualsizes, actualsizes, arrays.times, times);
		EightBitCountingSort(sizes, arrays.sizes, cur, 24, names, arrays.names, children, arrays.children, actualsizes, arrays.actualsizes, times, arrays.times);
		EightBitCountingSort(arrays.sizes, sizes, cur, 32, arrays.names, names, arrays.children, children, arrays.actualsizes, actualsizes, arrays.times, times);
		EightBitCountingSort(sizes, arrays.sizes, cur, 40, names, arrays.names, children, arrays.children, actualsizes, arrays.actualsizes, times, arrays.times);
		EightBitCountingSort(arrays.sizes, sizes, cur, 48, arrays.names, names, arrays.children, children, arrays.actualsizes, actualsizes, arrays.times, times);
		EightBitCountingSort(sizes, arrays.sizes, cur, 56, names, arrays.names, children, arrays.children, actualsizes, arrays.actualsizes, times, arrays.times);

		SM_FreeFolderEntryArrays(&arrays);
	}

update_parent_indexes:
	for (ui32 i = 0; i < cur; i++) {
		if (children[i] != NULL) children[i]->parentindex = i;
	}
}

static ui32 strxcpy(char *dest, const char *src)
{
	ui32 len = 0;
	while ((*dest++ = *src++) != '\0') len++;
	return len;
}

BOOL CFolder::LoadFolderInitial(CFolderTree *tree, const char *name, ui64 clustersize, CFolderDialog *dialog)
{
	std::wstring widePath = PathUtil::AnsiToWide(name);
	std::wstring absPath = PathUtil::GetAbsolutePath(widePath);
	absPath = PathUtil::EnsureTrailingBackslash(absPath);
	
	if (dialog != NULL) {
		std::string ansiAbsPath = PathUtil::WideToAnsi(absPath);
		dialog->SetPath(tree, ansiAbsPath.c_str(), this);
	}

	BOOL aligned;
	aligned = (clustersize != 0 && (clustersize & (clustersize - 1)) == 0);

	std::wstring preparedPath = PathUtil::PrepareLongPath(absPath);
	preparedPath = PathUtil::EnsureTrailingBackslash(preparedPath);

	return LoadFolder(tree, preparedPath, clustersize-1, aligned, dialog);
}

BOOL CFolder::LoadFolder(CFolderTree *tree, std::wstring& path, ui64 clustersize, BOOL aligned, CFolderDialog *dialog)
{
	WIN32_FIND_DATAW finddata;
	BOOL gotfile;
	HANDLE handle;
	ui64 size;
	static DWORD last_tick = 0;
	std::wstring::size_type baseLength;

	baseLength = PathUtil::AppendComponent(path, L"*.*");

	handle = FindFirstFileW(path.c_str(), &finddata);
	path.resize(baseLength);
	gotfile = (handle != INVALID_HANDLE_VALUE);
	while (gotfile && (dialog == NULL || !dialog->cancelled)) {
		// Ignore "." and ".."
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (finddata.cFileName[0] == L'.' && (finddata.cFileName[1] == L'\0'
					|| (finddata.cFileName[1] == L'.' && finddata.cFileName[2] == L'\0'))) {
				goto nextfile;
			}
		}

		// Handle (i.e., IGNORE) any junction points, mount points, or symbolic links.
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
			// In modern Windows, WIN32_FIND_DATA's dwReserved0 contains the reparse tag
			// if FILE_ATTRIBUTE_REPARSE_POINT is set.
			DWORD reparseTag = finddata.dwReserved0;
			
			// If it's a name surrogate (e.g. symlink, mount point), we should not traverse it
			// or count it as local disk space to avoid double-counting or infinite loops.
			// Cloud placeholders (OneDrive) are NOT name surrogates and will safely fall through.
			if (IsReparseTagNameSurrogate(reparseTag)) {
				goto nextfile;
			}
		}

		// Process directories
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (dialog != NULL)
				dialog->IncFolders();
			std::wstring::size_type childLength = PathUtil::AppendComponent(path, finddata.cFileName);
			if (!path.empty() && path.back() != L'\\')
				path += L'\\';

			CFolder *newfolder = new CFolder;
			if (!newfolder->LoadFolder(tree, path, clustersize, aligned, dialog)) {
				delete newfolder;
				path.resize(childLength);
				FindClose(handle);
				return 0;
			}

			if (!AddFolder(tree, finddata.cFileName, wcslen(finddata.cFileName), newfolder,
				*(ui64 *)&finddata.ftLastWriteTime)) {
				delete newfolder;
				path.resize(childLength);
				FindClose(handle);
				return 0;
			}
			path.resize(childLength);
		}
		else {
			// Process files.
			if (dialog != NULL) dialog->IncFiles();
			
			std::wstring::size_type fileLength = PathUtil::AppendComponent(path, finddata.cFileName);

			SM_FILE_SIZE_INFO sizeinfo;
			SM_LoadFileSizeInfoW(path.c_str(), &finddata, &sizeinfo);

			ui64 actualsize = (ui64)SM_GetLogicalFileSize(&sizeinfo);
			size = (ui64)SM_ChooseDisplayedFileSize(&sizeinfo, clustersize, aligned);
			
			if (!AddFile(tree, finddata.cFileName, wcslen(finddata.cFileName), size, actualsize,
				*(ui64 *)&finddata.ftLastWriteTime)) {
				path.resize(fileLength);
				FindClose(handle);
				return 0;
			}
			path.resize(fileLength);
		}

	nextfile:
		gotfile = FindNextFileW(handle, &finddata);

		// Only poll the system event queue every 1/5 of a second or so.
		DWORD tick = ::GetTickCount();
		if (tick > last_tick + 200) {
			last_tick = tick;
			MSG msg;
			if (dialog != NULL) {
				std::string ansiPath = PathUtil::WideToAnsi(path);
				const char *displayPath = ansiPath.c_str();
				if (strncmp(displayPath, "\\\\?\\", 4) == 0) displayPath += 4;
				dialog->SetPath(tree, displayPath, this);
			}
			while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
	}
	if (handle != INVALID_HANDLE_VALUE) FindClose(handle);
	Finalize();

	return dialog == NULL || !dialog->cancelled;
}


void CFolder::DelFile(unsigned int index)
{
}

void CFolder::RenameFile(unsigned int index, const CString &name)
{
}

unsigned int CFolder::FindFile(const CString &name)
{
	return((unsigned int)-1);
}

//////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CFolderDialog, CDialog)

BEGIN_MESSAGE_MAP(CFolderDialog, CDialog)
	//{{AFX_MSG_MAP(CFolderDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CFolderDialog::CFolderDialog()
{
	last_tick = last_redraw = 0;
	path = "";
	numfiles = 0;
	numfolders = 0;
	chg_path = chg_numfiles = chg_numfolders = 0;
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

	last_tick = last_redraw = 0;
	path = "";
	numfiles = 0;
	numfolders = 0;
	chg_path = chg_numfiles = chg_numfolders = 0;
	cur_frame = 0;

	cancelled = 0;

	SetWindowText(CurLang->scanning);
	SetDlgItemText(IDC_STATIC_FILESFOUND, CurLang->files_found);
	SetDlgItemText(IDC_STATIC_FOLDERSFOUND, CurLang->folders_found);
	SetDlgItemText(IDCANCEL, CurLang->cancel);
	SetDlgItemText(IDC_PATH, path);
	SetDlgItemInt(IDC_FILES, numfiles);
	SetDlgItemInt(IDC_FOLDERS, numfolders);
	CProgressCtrl *progress = (CProgressCtrl *)GetDlgItem(IDC_LOAD_PROGRESS);
	if (progress != NULL) {
		progress->SetRange(0, 4096);
		progress->SetPos(0);
	}

	CClientDC dc(this);
	CDC srcdc;
	srcdc.CreateCompatibleDC(&dc);
	::SelectObject(srcdc.m_hDC, theApp.m_scan_animation[0]);
	CRect rect;
	GetClientRect(&rect);
	dc.BitBlt(8, rect.bottom - rect.top - (48 + 8) - 12, 128, 48, &srcdc, 0, 0, SRCCOPY);
}

void CFolderDialog::UpdateDisplay(CFolderTree *tree, CFolder *folder)
{
	// Guarantee that we don't update more often than 1/5 of a second
	DWORD tick = ::GetTickCount();
	if (tick > last_tick + 200) {
		last_tick = tick;

		if (!IsWindow(m_hWnd)) return;

		if (chg_path) SetDlgItemText(IDC_PATH, path);
		if (chg_numfiles) SetDlgItemInt(IDC_FILES, numfiles);
		if (chg_numfolders) SetDlgItemInt(IDC_FOLDERS, numfolders);
		CProgressCtrl *progress = (CProgressCtrl *)GetDlgItem(IDC_LOAD_PROGRESS);
		if (progress != NULL) {
			ui64 filespace = tree->filespace/16384;
			ui64 usedspace = tree->usedspace/16384;
			if (usedspace <= 0) usedspace = 1;
			progress->SetRange(0, 4096);
			progress->SetPos((int)(filespace*4096/usedspace));
		}
		chg_path = chg_numfiles = chg_numfolders = 0;

		cur_frame = (cur_frame + 1) & 7;
		if (!(cur_frame & 1)) {
			CClientDC dc(this);
			CDC srcdc;
			srcdc.CreateCompatibleDC(&dc);
			::SelectObject(srcdc.m_hDC, theApp.m_scan_animation[cur_frame >> 1]);
			CRect rect;
			GetClientRect(&rect);
			dc.BitBlt(8, rect.bottom - rect.top - (48 + 8) - 12, 128, 48, &srcdc, 0, 0, SRCCOPY);
		}
	}
#if 0
	// Redraws the display during the scan.  Very ugly-looking.
	if (tick > last_redraw + 1000) {
		last_redraw = tick;
		while (folder != NULL) {
			folder->Finalize();
			folder = folder->parent;
		}
		tree->UpdateAllViews();
	}
#endif
}

void CFolderDialog::ForcedUpdate(CFolderTree *tree)
{
	if (!IsWindow(m_hWnd)) return;

	SetDlgItemText(IDC_PATH, path);
	SetDlgItemInt(IDC_FILES, numfiles);
	SetDlgItemInt(IDC_FOLDERS, numfolders);
	CProgressCtrl *progress = (CProgressCtrl *)GetDlgItem(IDC_LOAD_PROGRESS);
	if (progress != NULL) {
		progress->SetRange(0, 4096);
		progress->SetPos(4096);
	}
}

