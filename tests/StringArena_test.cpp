#include "../StringArena.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "CHECK failed: %s at %s:%d\n", #condition, __FILE__, __LINE__); \
			return 0; \
		} \
	} while (0)

static int reset_allows_fresh_allocations()
{
	CStringArena arena;
	wchar_t *first = arena.Allocate(8);
	CHECK(first != NULL);
	first[0] = L'a';
	first[1] = L'\0';

	arena.Reset();

	wchar_t *second = arena.Allocate(8);
	CHECK(second != NULL);
	second[0] = L'b';
	second[1] = L'\0';
	CHECK(second[0] == L'b');
	return 1;
}

static int large_allocations_do_not_break_followup_allocations()
{
	CStringArena arena;
	wchar_t *large = arena.Allocate(40000);
	CHECK(large != NULL);
	large[0] = L'x';

	wchar_t *small = arena.Allocate(16);
	CHECK(small != NULL);
	small[0] = L'y';
	CHECK(large[0] == L'x');
	CHECK(small[0] == L'y');
	return 1;
}

static int allocations_that_overflow_a_block_are_rejected()
{
	CStringArena arena;
	size_t maxLength = ((size_t)-1) / sizeof(wchar_t);

	CHECK(arena.Allocate(maxLength) == NULL);
	return 1;
}

static int merge_arenas_preserves_allocations()
{
	CStringArena arena1;
	wchar_t *str1 = arena1.Allocate(10);
	CHECK(str1 != NULL);
	wcscpy_s(str1, 10, L"First");

	CStringArena arena2;
	wchar_t *str2 = arena2.Allocate(10);
	CHECK(str2 != NULL);
	wcscpy_s(str2, 10, L"Second");

	arena1.Merge(arena2);

	CHECK(wcscmp(str1, L"First") == 0);
	CHECK(wcscmp(str2, L"Second") == 0);

	// arena2 should now be empty and ready for fresh allocations
	wchar_t *str3 = arena2.Allocate(10);
	CHECK(str3 != NULL);
	wcscpy_s(str3, 10, L"Third");
	CHECK(wcscmp(str3, L"Third") == 0);

	return 1;
}

static int allocations_after_merge_do_not_clobber_merged_strings()
{
	// Regression test: Merge adopts the donor's head block, which is already
	// (partially) full; a subsequent Allocate on the destination arena must not
	// hand out memory that overlaps the donor's live strings.
	CStringArena dest;
	CStringArena donor;
	wchar_t *donated = donor.Allocate(16);
	CHECK(donated != NULL);
	wcscpy_s(donated, 16, L"DonatedString");

	dest.Merge(donor);

	wchar_t *fresh = dest.Allocate(4);
	CHECK(fresh != NULL);
	wcscpy_s(fresh, 4, L"<");

	CHECK(wcscmp(donated, L"DonatedString") == 0);

	// Same must hold when the destination already had its own strings.
	CStringArena dest2;
	wchar_t *own = dest2.Allocate(8);
	CHECK(own != NULL);
	wcscpy_s(own, 8, L"Own");

	CStringArena donor2;
	wchar_t *donated2 = donor2.Allocate(8);
	CHECK(donated2 != NULL);
	wcscpy_s(donated2, 8, L"Two");

	dest2.Merge(donor2);
	wchar_t *fresh2 = dest2.Allocate(8);
	CHECK(fresh2 != NULL);
	wcscpy_s(fresh2, 8, L"New");

	CHECK(wcscmp(own, L"Own") == 0);
	CHECK(wcscmp(donated2, L"Two") == 0);
	CHECK(wcscmp(fresh2, L"New") == 0);

	return 1;
}

int main()
{
	if (!reset_allows_fresh_allocations()) return 1;
	if (!large_allocations_do_not_break_followup_allocations()) return 1;
	if (!allocations_that_overflow_a_block_are_rejected()) return 1;
	if (!merge_arenas_preserves_allocations()) return 1;
	if (!allocations_after_merge_do_not_clobber_merged_strings()) return 1;
	return 0;
}
