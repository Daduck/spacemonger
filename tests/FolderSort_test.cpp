#include "../FolderSort.h"

#include <stdio.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

static int all_radix_count_buckets_are_initialized()
{
	ui32 buckets[257];
	for (ui32 i = 0; i < 257; i++) buckets[i] = (ui32)-1;

	SM_InitRadixCountArray(buckets);

	for (ui32 i = 0; i < 257; i++)
		CHECK(buckets[i] == 0);
	return 1;
}

int main()
{
	if (!all_radix_count_buckets_are_initialized()) return 1;
	return 0;
}
