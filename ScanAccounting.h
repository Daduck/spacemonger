#ifndef SCANACCOUNTING_H
#define SCANACCOUNTING_H

#include "e.h"

// The difference between volume-used space and the bytes represented by the
// scan is an upper-bound estimate for content the scanner could not account
// for.  It is only useful for a partial whole-volume scan; callers decide
// whether those conditions apply.
inline ui64 ComputeUnaccountedSpace(ui64 usedSpace, ui64 scannedSpace)
{
	return usedSpace > scannedSpace ? usedSpace - scannedSpace : 0;
}

#endif // SCANACCOUNTING_H
