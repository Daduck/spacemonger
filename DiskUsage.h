#ifndef DISKUSAGE_H
#define DISKUSAGE_H

#include <windows.h>

#if defined(__GNUC__)
typedef unsigned long long SM_UINT64;
#else
typedef unsigned __int64 SM_UINT64;
#endif

#ifndef FILE_ATTRIBUTE_RECALL_ON_OPEN
#define FILE_ATTRIBUTE_RECALL_ON_OPEN 0x00040000
#endif

#ifndef FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS
#define FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS 0x00400000
#endif

#ifndef FILE_ATTRIBUTE_UNPINNED
#define FILE_ATTRIBUTE_UNPINNED 0x00100000
#endif

typedef struct SM_FILE_SIZE_INFO {
	SM_UINT64 logical_size;
	SM_UINT64 allocated_size;
	DWORD attributes;
	int has_allocated_size;
} SM_FILE_SIZE_INFO;

SM_UINT64 SM_MakeFileSize(DWORD low, DWORD high);
SM_UINT64 SM_GetLogicalFileSize(const SM_FILE_SIZE_INFO *info);
SM_UINT64 SM_ChooseDisplayedFileSize(const SM_FILE_SIZE_INFO *info, SM_UINT64 cluster_mask, int aligned);
int SM_LoadFileSizeInfo(const char *path, const WIN32_FIND_DATA *finddata, SM_FILE_SIZE_INFO *info);

#endif
