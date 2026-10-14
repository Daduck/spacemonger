#ifndef TIPWNDTEXT_H
#define TIPWNDTEXT_H

#include <stddef.h>
#include <wchar.h>

inline size_t SM_CopyTipText(wchar_t *destination, size_t destinationChars, const wchar_t *source)
{
	if (destination == NULL || destinationChars == 0)
		return 0;

	if (source == NULL)
		source = L"";

	size_t sourceLength = wcslen(source);
	size_t copiedLength = sourceLength < destinationChars - 1
		? sourceLength : destinationChars - 1;
	if (copiedLength != 0)
		wmemcpy(destination, source, copiedLength);
	destination[copiedLength] = L'\0';
	return copiedLength;
}

#endif
