#ifndef FOLDER_H
#define FOLDER_H

#ifdef bool
#pragma push_macro("bool")
#undef bool
#include <string>
#pragma pop_macro("bool")
#else
#include <string>
#endif

#include "e.h"
#include "StringArena.h"

#ifdef _AFX
#include <afxwin.h>
#else
#include <windows.h>
#endif

class CFolderTree;
class CFolderDialog;
struct CFolder;

wchar_t *SM_ArenaWcsDup(CStringArena& arena, const wchar_t *string, int stringlen);

struct CFolder {
public:
	CFolder();
	~CFolder();

	BOOL AddFileWithArena(CStringArena &arena, const wchar_t *name, ui32 namelen, ui64 size, ui64 actual_size, ui64 time);
	BOOL AddFolderWithArena(CStringArena &arena, const wchar_t *name, ui32 namelen, CFolder *folder, ui64 time);
	void Finalize(void);
	void DelFile(unsigned int index);
	void RenameFile(unsigned int index, const wchar_t *name);
	unsigned int FindFile(const wchar_t *name);
	inline ui64 SizeFiles() const { return size_self; }
	inline ui64 SizeSub() const { return size_children; }
	inline ui64 SizeTotal() const { return size_self + size_children; }

private:
	BOOL MoreEntries(void);
	void EightBitCountingSort(ui64 *dsize, ui64 *ssize, ui32 count, ui32 bitpos,
		wchar_t **dnames, wchar_t **snames, CFolder **dkids, CFolder **skids,
		ui64 *dasize, ui64 *sasize, ui64 *dtimes, ui64 *stimes);

public:
	CFolder *parent;
	unsigned int parentindex;

	wchar_t **names;
	CFolder **children;
	ui64 *sizes;
	ui64 *actualsizes;
	ui64 *times;
	ui64 size_self, size_children;

	unsigned int cur, max;
};

#endif // FOLDER_H
