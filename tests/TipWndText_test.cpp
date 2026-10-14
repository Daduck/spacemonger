#include "../TipWndText.h"

#include <stdio.h>
#include <wchar.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

static int copies_full_text_and_reports_length()
{
	wchar_t buffer[16];

	CHECK(SM_CopyTipText(buffer, sizeof(buffer) / sizeof(buffer[0]), L"hello") == 5);
	CHECK(wcscmp(buffer, L"hello") == 0);
	return 1;
}

static int truncates_text_to_fit_and_reports_copied_length()
{
	wchar_t buffer[5];

	CHECK(SM_CopyTipText(buffer, sizeof(buffer) / sizeof(buffer[0]), L"hello") == 4);
	CHECK(wcscmp(buffer, L"hell") == 0);
	return 1;
}

static int empty_and_zero_capacity_destinations_are_safe()
{
	wchar_t buffer[1] = {L'x'};

	CHECK(SM_CopyTipText(buffer, sizeof(buffer), NULL) == 0);
	CHECK(buffer[0] == L'\0');
	CHECK(SM_CopyTipText(NULL, 0, L"hello") == 0);
	return 1;
}

int main()
{
	if (!copies_full_text_and_reports_length()) return 1;
	if (!truncates_text_to_fit_and_reports_copied_length()) return 1;
	if (!empty_and_zero_capacity_destinations_are_safe()) return 1;
	return 0;
}
