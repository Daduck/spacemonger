#include "DiskUsage.h"

typedef DWORD (WINAPI *SM_GetCompressedFileSizeFunc)(LPCSTR filename, LPDWORD high_size);

SM_UINT64 SM_MakeFileSize(DWORD low, DWORD high)
{
	return (SM_UINT64)low + ((SM_UINT64)high << 32);
}

SM_UINT64 SM_GetLogicalFileSize(const SM_FILE_SIZE_INFO *info)
{
	return info->logical_size;
}

static int SM_UsesAllocatedSize(DWORD attributes)
{
	DWORD flags =
		FILE_ATTRIBUTE_COMPRESSED |
		FILE_ATTRIBUTE_OFFLINE |
		FILE_ATTRIBUTE_REPARSE_POINT |
		FILE_ATTRIBUTE_SPARSE_FILE |
		FILE_ATTRIBUTE_RECALL_ON_OPEN |
		FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS |
		FILE_ATTRIBUTE_UNPINNED;

	return (attributes & flags) != 0;
}

static SM_UINT64 SM_LegacyClusterSize(SM_UINT64 logical_size, SM_UINT64 cluster_mask, int aligned)
{
	if (aligned)
		return (logical_size + cluster_mask) & cluster_mask;

	SM_UINT64 cluster_size = cluster_mask + 1;
	if (cluster_size == 0)
		return logical_size;

	return logical_size - (logical_size % cluster_size);
}

SM_UINT64 SM_ChooseDisplayedFileSize(const SM_FILE_SIZE_INFO *info, SM_UINT64 cluster_mask, int aligned)
{
	if (info->has_allocated_size && SM_UsesAllocatedSize(info->attributes))
		return info->allocated_size;

	return SM_LegacyClusterSize(info->logical_size, cluster_mask, aligned);
}

int SM_LoadFileSizeInfo(const char *path, const WIN32_FIND_DATA *finddata, SM_FILE_SIZE_INFO *info)
{
	DWORD high_size = 0;
	DWORD low_size;
	HMODULE kernel;
	SM_GetCompressedFileSizeFunc get_compressed_size;

	if (path == NULL || finddata == NULL || info == NULL)
		return 0;

	info->logical_size = SM_MakeFileSize(finddata->nFileSizeLow, finddata->nFileSizeHigh);
	info->allocated_size = 0;
	info->attributes = finddata->dwFileAttributes;
	info->has_allocated_size = 0;

	kernel = GetModuleHandle("KERNEL32.DLL");
	if (kernel == NULL)
		return 1;

	get_compressed_size = (SM_GetCompressedFileSizeFunc)GetProcAddress(kernel, "GetCompressedFileSizeA");
	if (get_compressed_size == NULL)
		return 1;

	SetLastError(ERROR_SUCCESS);
	low_size = get_compressed_size(path, &high_size);
	if (low_size != 0xFFFFFFFFUL || GetLastError() == ERROR_SUCCESS) {
		info->allocated_size = SM_MakeFileSize(low_size, high_size);
		info->has_allocated_size = 1;
	}

	return 1;
}
