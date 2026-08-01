#ifndef FOLDERSORT_H
#define FOLDERSORT_H

#include "e.h"

inline void SM_InitRadixCountArray(ui32 *countarray)
{
	for (ui32 i = 0; i <= 256; i++)
		countarray[i] = 0;
}

#endif
