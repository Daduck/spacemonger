#include "../FolderEntryArrays.h"

#include <stdio.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

static int impossible_count_fails_without_partial_arrays()
{
	CFolderEntryArrays arrays;
	SM_InitFolderEntryArrays(&arrays);

	CHECK(!SM_AllocateFolderEntryArrays(&arrays, (size_t)-1));
	CHECK(arrays.names == NULL);
	CHECK(arrays.sizes == NULL);
	CHECK(arrays.actualsizes == NULL);
	CHECK(arrays.children == NULL);
	CHECK(arrays.times == NULL);
	return 1;
}

static int normal_count_allocates_all_arrays()
{
	CFolderEntryArrays arrays;
	SM_InitFolderEntryArrays(&arrays);

	CHECK(SM_AllocateFolderEntryArrays(&arrays, 32));
	CHECK(arrays.names != NULL);
	CHECK(arrays.sizes != NULL);
	CHECK(arrays.actualsizes != NULL);
	CHECK(arrays.children != NULL);
	CHECK(arrays.times != NULL);

	SM_FreeFolderEntryArrays(&arrays);
	CHECK(arrays.names == NULL);
	CHECK(arrays.sizes == NULL);
	CHECK(arrays.actualsizes == NULL);
	CHECK(arrays.children == NULL);
	CHECK(arrays.times == NULL);
	return 1;
}

int main()
{
	if (!impossible_count_fails_without_partial_arrays()) return 1;
	if (!normal_count_allocates_all_arrays()) return 1;
	return 0;
}
