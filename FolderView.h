#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include "FolderTree.h"
#include "TipWnd.h"
#include "TreemapLayout.h"
#include <vector>

typedef TreemapNode CDisplayFolder;

class AsyncScanEngine;

class CFolderView : public CFreeView {
public:
	CFolderView();
	DECLARE_DYNCREATE(CFolderView)
	virtual ~CFolderView();

	void UpdateLiveScanLayout(AsyncScanEngine& engine, ui64 totalspace, ui64 freespace);
	void ClearLiveScanLayout(void);
	virtual void SetDocument(CFreeDoc *doc = NULL);
	virtual void SetPalette(void);

	void BuildTitleReverse(CFolder *folder, CString &string);
	const TreemapNode *GetDisplayFolderFromPoint(const CPoint &point);
	const TreemapNode *GetContainerDisplayFolderFromPoint(const CPoint &point);
	void HighlightPathAtPoint(const CPoint &point);

	void ZoomIn(const TreemapNode *folder);
	void ZoomOut(void);
	void ZoomFull(void);
	void ShowFreeSpace(BOOL show);

	inline bool IsAnythingOpen(void)
		{ return rootfolder != NULL; }
	inline bool IsAnythingSelected(void)
		{ return selected != NULL; }
	inline bool IsSelectedAFolder(void)
		{ return selected != NULL && (selected->flags & 1) != 0; }
	inline bool IsZoomFull(void)
		{ return zoomlevel == 0; }

	void UpdateTitleBar(void);
	void RecreateFonts(void);

	void SelectFolder(const TreemapNode *cur);
	void BuildTitleReverseW(CFolder *folder, std::wstring& string);
	std::wstring BuildItemPathW(const TreemapNode *folder);
	std::wstring BuildContainerPathW(const TreemapNode *folder);

	//{{AFX_VIRTUAL(CFolderView)
protected:
	virtual void OnDraw(CDC *pDC);
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CFolderView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT flags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT flags, CPoint point);
	afx_msg void OnRButtonUp(UINT flags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg void OnIgnoreUpdate(CCmdUI *ui);
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT *pResult);
	afx_msg void OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized);
	DECLARE_MESSAGE_MAP()

public:
	virtual void OnUpdate(CFreeDoc *doc);

protected:
	void MinimalDrawDisplayFolder(CDC *pDC, const TreemapNode *cur, BOOL selected);
	void AnimateBox(const CRect &start, const CRect &end);
	void SetupInfoTip(const TreemapNode *cur);
	void SetupNameTip(const TreemapNode *cur);

protected:
	CPalette m_palette;
	CBrush black, white;
	CFont minifont;
	CFolder *rootfolder;
	std::vector<TreemapNode> m_layoutNodes;
	// Owns the name strings referenced by live-scan layout nodes, so the
	// nodes outlive the scan engine's arenas (e.g. on a cancelled scan).
	std::vector<std::wstring> m_liveNameStorage;
	CTipWnd m_infotipwnd;
	CTipWnd m_nametipwnd;
	const TreemapNode *lastcur;

public:
	int zoomlevel;
	const TreemapNode *selected;
	BOOL showfreespace;
};

#endif
