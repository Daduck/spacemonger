#include "../TipWndText.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

static int copies_full_text_and_reports_length()
{
	char buffer[16];

	CHECK(SM_CopyTipText(buffer, sizeof(buffer), "hello") == 5);
	CHECK(strcmp(buffer, "hello") == 0);
	return 1;
}

static int truncates_text_to_fit_and_reports_copied_length()
{
	char buffer[5];

	CHECK(SM_CopyTipText(buffer, sizeof(buffer), "hello") == 4);
	CHECK(strcmp(buffer, "hell") == 0);
	return 1;
}

static int empty_and_zero_capacity_destinations_are_safe()
{
	char buffer[1] = {'x'};

	CHECK(SM_CopyTipText(buffer, sizeof(buffer), NULL) == 0);
	CHECK(buffer[0] == '\0');
	CHECK(SM_CopyTipText(NULL, 0, "hello") == 0);
	return 1;
}

int main()
{
	if (!copies_full_text_and_reports_length()) return 1;
	if (!truncates_text_to_fit_and_reports_copied_length()) return 1;
	if (!empty_and_zero_capacity_destinations_are_safe()) return 1;
	return 0;
}
