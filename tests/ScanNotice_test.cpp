#include "../ScanNotice.h"

#include <stdio.h>

static int TestNoticeLifecycle()
{
	ScanNoticeState notice;
	if (notice.IsVisible()) return 0;

	notice.Set(451, ERROR_ACCESS_DENIED);
	if (!notice.IsVisible()) return 0;
	if (notice.skippedDirectories != 451) return 0;
	if (notice.firstError != ERROR_ACCESS_DENIED) return 0;

	notice.Clear();
	return !notice.IsVisible() && notice.skippedDirectories == 0
		&& notice.firstError == ERROR_SUCCESS;
}

int main()
{
	if (!TestNoticeLifecycle()) {
		fprintf(stderr, "ScanNotice lifecycle test failed\n");
		return 1;
	}
	return 0;
}
