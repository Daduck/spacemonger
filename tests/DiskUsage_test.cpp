#include "../DiskUsage.h"

#include <assert.h>
#include <windows.h>

static void local_files_keep_cluster_rounded_size()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 4097;
	info.allocated_size = 0;
	info.attributes = FILE_ATTRIBUTE_ARCHIVE;
	info.has_allocated_size = 0;

	assert(SM_ChooseDisplayedFileSize(&info, 4095, 0) == 4096);
}

static void sparse_and_offline_files_use_allocated_size()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 119ULL * 1024ULL * 1024ULL * 1024ULL;
	info.allocated_size = 57344;
	info.attributes = FILE_ATTRIBUTE_SPARSE_FILE | FILE_ATTRIBUTE_OFFLINE;
	info.has_allocated_size = 1;

	assert(SM_ChooseDisplayedFileSize(&info, 4095, 1) == 57344);
}

static void unpinned_cloud_files_use_allocated_size()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 50ULL * 1024ULL * 1024ULL * 1024ULL;
	info.allocated_size = 4096;
	info.attributes = FILE_ATTRIBUTE_UNPINNED;
	info.has_allocated_size = 1;

	assert(SM_ChooseDisplayedFileSize(&info, 4095, 0) == 4096);
}

static void logical_size_is_preserved_for_details()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 123456789;
	info.allocated_size = 4096;
	info.attributes = FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE;
	info.has_allocated_size = 1;

	assert(SM_GetLogicalFileSize(&info) == 123456789);
}

int main()
{
	local_files_keep_cluster_rounded_size();
	sparse_and_offline_files_use_allocated_size();
	unpinned_cloud_files_use_allocated_size();
	logical_size_is_preserved_for_details();
	return 0;
}
