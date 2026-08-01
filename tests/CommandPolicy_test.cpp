#include "../CommandPolicy.h"

#include <stdio.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

static int delete_is_allowed_only_when_enabled_and_selected()
{
	CHECK(SM_CanDeleteSelection(0, 1));
	CHECK(!SM_CanDeleteSelection(0, 0));
	CHECK(!SM_CanDeleteSelection(1, 1));
	CHECK(!SM_CanDeleteSelection(1, 0));
	return 1;
}

int main()
{
	if (!delete_is_allowed_only_when_enabled_and_selected()) return 1;
	return 0;
}
