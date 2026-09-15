#include "ScanAccounting.h"

#include <stdio.h>

#define CHECK(condition) \
	do { \
	if (!(condition)) { \
		fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #condition, __FILE__, __LINE__); \
		return 0; \
	} \
} while (0)

static int test_unaccounted_space_is_clamped()
{
	CHECK(ComputeUnaccountedSpace(900, 650) == 250);
	CHECK(ComputeUnaccountedSpace(900, 900) == 0);
	CHECK(ComputeUnaccountedSpace(900, 950) == 0);
	return 1;
}

int main()
{
	return test_unaccounted_space_is_clamped() ? 0 : 1;
}
