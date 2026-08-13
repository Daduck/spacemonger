#ifndef FOLDERTREE_H
#define FOLDERTREE_H

#include "Folder.h"

#ifdef _AFX

#include <afxwin.h>

class CFolderTree : public CFreeDoc {
public:
	CFolderTree();
	virtual ~CFolderTree();

	BOOL LoadTree(const CString &path, BOOL includespace, CWnd *modalwin);
	void GetSpace(const CString &path);
	CFolder *GetRoot(void);

	CFolder *SetCur(const CString &path);
	CFolder *GetCur(void);
	CFolder *Down(unsigned int index);
	CFolder *Up(void);

protected:
	CFolder *root, *cur;

public:
	CStringArena nameArena;
	CString m_path;
	ui64 freespace, usedspace, totalspace, clustersize;
	ui64 numfiles, numfolders;
	ui64 filespace;
};

struct ScanProgress;

class CFolderDialog : public CDialog {
public:
	CFolderDialog();
	DECLARE_DYNCREATE(CFolderDialog)
	virtual ~CFolderDialog();

	virtual void Reset(void);
	virtual void UpdateFromProgress(const ScanProgress& progress, ui64 usedspace);

	virtual void OnCancel(void);

protected:
	void DrawScanAnimation(ui32 frame);

	//{{AFX_MSG(CFolderDialog)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	BOOL cancelled;

protected:
	ui32 cur_frame;
};

#endif // _AFX

#endif // FOLDERTREE_H
