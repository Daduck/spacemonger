#ifndef SCANNOTICE_H
#define SCANNOTICE_H

#include <windows.h>
#include "e.h"

struct ScanNoticeState {
	ui64 skippedDirectories;
	DWORD firstError;

	ScanNoticeState()
		: skippedDirectories(0), firstError(ERROR_SUCCESS)
	{
	}

	void Set(ui64 skipped, DWORD error)
	{
		skippedDirectories = skipped;
		firstError = error;
	}

	void Clear()
	{
		skippedDirectories = 0;
		firstError = ERROR_SUCCESS;
	}

	bool IsVisible() const
	{
		return skippedDirectories != 0;
	}
};

#endif
