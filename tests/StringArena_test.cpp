#include "../StringArena.h"

#include <stdio.h>

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

int main()
{
	if (!reset_allows_fresh_allocations()) return 1;
	if (!large_allocations_do_not_break_followup_allocations()) return 1;
	return 0;
}
