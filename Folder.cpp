#include "Folder.h"
#include "FolderTree.h"
#include "FolderEntryArrays.h"
#include "FolderSort.h"

#include <vector>
#include <algorithm>
#include <string.h>

wchar_t *SM_ArenaWcsDup(CStringArena& arena, const wchar_t *string, int stringlen)
{
	wchar_t *newstr = arena.Allocate(stringlen + 1);
	if (newstr == NULL) return NULL;

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
		src = newstr + 1;
		while ((c = *src) != L'\0') {
			if (c >= L'A' && c <= L'Z') *src += 32;
			src++;
		}
	}

	return newstr;
}

static void AssignFolderEntryArrays(CFolder *folder, const CFolderEntryArrays& arrays)
{
	folder->names = arrays.names;
	folder->sizes = arrays.sizes;
	folder->actualsizes = arrays.actualsizes;
	folder->children = arrays.children;
	folder->times = arrays.times;
}

CFolder::CFolder()
{
	// Entry arrays are allocated lazily on the first Add*: scans create one
	// CFolder per directory, and most directories hold only a few entries.
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
}

CFolder::~CFolder()
{
	for (unsigned int i = 0; i < cur; i++) {
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

	unsigned int newmax = (max == 0) ? 8 : max * 2;
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

BOOL CFolder::AddFileWithArena(CStringArena &arena, const wchar_t *name, ui32 namelen, ui64 size, ui64 actual_size, ui64 time)
{
	if (cur >= max && !MoreEntries()) return 0;

	names[cur] = SM_ArenaWcsDup(arena, name, namelen);
	if (names[cur] == NULL) return 0;
	actualsizes[cur] = actual_size;
	times[cur] = time;
	size_self += (sizes[cur] = size);
	children[cur] = NULL;
	cur++;
	return 1;
}

BOOL CFolder::AddFolderWithArena(CStringArena &arena, const wchar_t *name, ui32 namelen, CFolder *folder, ui64 time)
{
	if (cur >= max && !MoreEntries()) return 0;

	names[cur] = SM_ArenaWcsDup(arena, name, namelen);
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

void CFolder::EightBitCountingSort(ui64 *dsize, ui64 *ssize, ui32 count, ui32 bitpos,
		wchar_t **dnames, wchar_t **snames, CFolder **dkids, CFolder **skids,
		ui64 *dasize, ui64 *sasize, ui64 *dtimes, ui64 *stimes)
{
	ui32 countarray[257];
	ui32 i, dest;

#define VALUE(size) (0xFF - (ui32)((size) >> bitpos) & 0xFF)

	SM_InitRadixCountArray(countarray);

	for (i = 0; i < count; i++)
		countarray[VALUE(ssize[i]) + 1]++;

	for (i = 1; i < 256; i++)
		countarray[i] += countarray[i-1];

	for (i = 0; i < count; i++) {
		dest = countarray[VALUE(ssize[i])]++;
		dsize[dest] = ssize[i];
		dnames[dest] = snames[i];
		dkids[dest] = skids[i];
		dasize[dest] = sasize[i];
		dtimes[dest] = stimes[i];
	}
#undef VALUE
}

void CFolder::Finalize(void)
{
	size_children = 0;
	for (ui32 i = 0; i < cur; i++) {
		if (children[i] != NULL) {
			children[i]->parent = this;
			children[i]->Finalize();
			sizes[i] = children[i]->SizeTotal();
			actualsizes[i] = children[i]->SizeTotal();
			size_children += sizes[i];
		}
	}

	if (cur <= 1) {
		for (ui32 i = 0; i < cur; i++) {
			if (children[i] != NULL) {
				children[i]->parent = this;
				children[i]->parentindex = i;
			}
		}
		return;
	}

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
		if (children[i] != NULL) {
			children[i]->parent = this;
			children[i]->parentindex = i;
		}
	}
}

void CFolder::DelFile(unsigned int index)
{
}

void CFolder::RenameFile(unsigned int index, const wchar_t *name)
{
}

unsigned int CFolder::FindFile(const wchar_t *name)
{
	return((unsigned int)-1);
}
