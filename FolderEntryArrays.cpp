#include "FolderEntryArrays.h"

static int SM_CheckedArrayBytes(size_t count, size_t elementSize, size_t *bytes)
{
	if (elementSize != 0 && count > ((size_t)-1) / elementSize) {
		*bytes = 0;
		return 0;
	}
	*bytes = count * elementSize;
	return 1;
}

static int SM_AllocateArray(void **target, size_t count, size_t elementSize)
{
	size_t bytes;
	*target = NULL;
	if (!SM_CheckedArrayBytes(count, elementSize, &bytes)) return 0;
	if (bytes == 0) return 1;

	*target = malloc(bytes);
	return *target != NULL;
}

int SM_AllocateFolderEntryArrays(CFolderEntryArrays *arrays, size_t count)
{
	CFolderEntryArrays allocated;
	SM_InitFolderEntryArrays(&allocated);

	if (!SM_AllocateArray((void **)&allocated.names, count, sizeof(wchar_t *))
		|| !SM_AllocateArray((void **)&allocated.sizes, count, sizeof(ui64))
		|| !SM_AllocateArray((void **)&allocated.actualsizes, count, sizeof(ui64))
		|| !SM_AllocateArray((void **)&allocated.children, count, sizeof(CFolder *))
		|| !SM_AllocateArray((void **)&allocated.times, count, sizeof(ui64))) {
		SM_FreeFolderEntryArrays(&allocated);
		SM_InitFolderEntryArrays(arrays);
		return 0;
	}

	*arrays = allocated;
	return 1;
}
