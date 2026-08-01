#ifndef TIPWNDTEXT_H
#define TIPWNDTEXT_H

#include <stddef.h>
#include <string.h>

inline size_t SM_CopyTipText(char *destination, size_t destinationSize, const char *source)
{
	if (destination == NULL || destinationSize == 0)
		return 0;

	if (source == NULL)
		source = "";

	size_t sourceLength = strlen(source);
	size_t copiedLength = sourceLength < destinationSize - 1
		? sourceLength : destinationSize - 1;
	if (copiedLength != 0)
		memcpy(destination, source, copiedLength);
	destination[copiedLength] = '\0';
	return copiedLength;
}

#endif
