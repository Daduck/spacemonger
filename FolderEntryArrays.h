#ifndef FOLDERENTRYARRAYS_H
#define FOLDERENTRYARRAYS_H

#include "E.H"
#include <stddef.h>
#include <stdlib.h>

struct CFolder;

struct CFolderEntryArrays {
	wchar_t **names;
	ui64 *sizes;
	ui64 *actualsizes;
	CFolder **children;
	ui64 *times;
};

inline void SM_InitFolderEntryArrays(CFolderEntryArrays *arrays)
{
	arrays->names = NULL;
	arrays->sizes = NULL;
	arrays->actualsizes = NULL;
	arrays->children = NULL;
	arrays->times = NULL;
}

inline void SM_FreeFolderEntryArrays(CFolderEntryArrays *arrays)
{
	free(arrays->names);
	free(arrays->sizes);
	free(arrays->actualsizes);
	free(arrays->children);
	free(arrays->times);
	SM_InitFolderEntryArrays(arrays);
}

int SM_AllocateFolderEntryArrays(CFolderEntryArrays *arrays, size_t count);

#endif
