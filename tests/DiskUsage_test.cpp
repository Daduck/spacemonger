#include "../DiskUsage.h"

#include <stdio.h>
#include <windows.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

static int local_files_round_up_to_cluster_size()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 4097;
	info.allocated_size = 0;
	info.attributes = FILE_ATTRIBUTE_ARCHIVE;
	info.has_allocated_size = 0;

	CHECK(SM_ChooseDisplayedFileSize(&info, 4095, 1) == 8192);
	CHECK(SM_ChooseDisplayedFileSize(&info, 4095, 0) == 8192);
	return 1;
}

static int local_files_keep_zero_and_exact_cluster_sizes()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 0;
	info.allocated_size = 0;
	info.attributes = FILE_ATTRIBUTE_ARCHIVE;
	info.has_allocated_size = 0;

	CHECK(SM_ChooseDisplayedFileSize(&info, 4095, 1) == 0);
	CHECK(SM_ChooseDisplayedFileSize(&info, 4095, 0) == 0);

	info.logical_size = 4096;
	CHECK(SM_ChooseDisplayedFileSize(&info, 4095, 1) == 4096);
	CHECK(SM_ChooseDisplayedFileSize(&info, 4095, 0) == 4096);
	return 1;
}

static int ordinary_files_do_not_need_allocated_size_lookup()
{
	CHECK(!SM_ShouldLoadAllocatedSize(FILE_ATTRIBUTE_ARCHIVE));
	return 1;
}

static int sparse_files_need_allocated_size_lookup()
{
	CHECK(SM_ShouldLoadAllocatedSize(FILE_ATTRIBUTE_SPARSE_FILE));
	return 1;
}

static int cloud_placeholders_do_not_need_allocated_size_lookup()
{
	CHECK(!SM_ShouldLoadAllocatedSize(FILE_ATTRIBUTE_OFFLINE));
	CHECK(!SM_ShouldLoadAllocatedSize(FILE_ATTRIBUTE_UNPINNED));
	CHECK(!SM_ShouldLoadAllocatedSize(FILE_ATTRIBUTE_RECALL_ON_OPEN));
	CHECK(!SM_ShouldLoadAllocatedSize(FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS));
	return 1;
}

static int sparse_files_use_allocated_size()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 119ULL * 1024ULL * 1024ULL * 1024ULL;
	info.allocated_size = 57344;
	info.attributes = FILE_ATTRIBUTE_SPARSE_FILE;
	info.has_allocated_size = 1;

	CHECK(SM_ChooseDisplayedFileSize(&info, 4095, 1) == 57344);
	return 1;
}

static int unpinned_cloud_files_use_zero_local_size()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 50ULL * 1024ULL * 1024ULL * 1024ULL;
	info.allocated_size = 4096;
	info.attributes = FILE_ATTRIBUTE_UNPINNED;
	info.has_allocated_size = 1;

	CHECK(SM_ChooseDisplayedFileSize(&info, 4095, 0) == 0);
	return 1;
}

static int logical_size_is_preserved_for_details()
{
	SM_FILE_SIZE_INFO info;
	info.logical_size = 123456789;
	info.allocated_size = 4096;
	info.attributes = FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE;
	info.has_allocated_size = 1;

	CHECK(SM_GetLogicalFileSize(&info) == 123456789);
	return 1;
}

int main()
{
	if (!local_files_round_up_to_cluster_size()) return 1;
	if (!local_files_keep_zero_and_exact_cluster_sizes()) return 1;
	if (!ordinary_files_do_not_need_allocated_size_lookup()) return 1;
	if (!sparse_files_need_allocated_size_lookup()) return 1;
	if (!cloud_placeholders_do_not_need_allocated_size_lookup()) return 1;
	if (!sparse_files_use_allocated_size()) return 1;
	if (!unpinned_cloud_files_use_zero_local_size()) return 1;
	if (!logical_size_is_preserved_for_details()) return 1;
	return 0;
}
