#ifndef STRINGARENA_H
#define STRINGARENA_H

#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

class CStringArena {
private:
	struct CArenaBlock {
		CArenaBlock *next;
		size_t capacity;
	};

	CArenaBlock *head;
	size_t offset;

	void FreeBlocks()
	{
		while (head) {
			CArenaBlock *next = head->next;
			free(head);
			head = next;
		}
		offset = 0;
	}

public:
	CStringArena() : head(NULL), offset(0) {}

	~CStringArena()
	{
		FreeBlocks();
	}

	void Reset()
	{
		FreeBlocks();
	}

	wchar_t *Allocate(size_t len)
	{
		const size_t defaultBlockSize = 65536;
		const size_t maxSize = (size_t)-1;
		size_t allocSize;
		size_t blockSize;

		if (len == 0 || len > maxSize / sizeof(wchar_t))
			return NULL;

		allocSize = len * sizeof(wchar_t);
		blockSize = allocSize > defaultBlockSize ? allocSize : defaultBlockSize;
		if (blockSize > maxSize - sizeof(CArenaBlock))
			return NULL;

		if (head == NULL || offset > head->capacity || allocSize > head->capacity - offset) {
			CArenaBlock *block = (CArenaBlock *)malloc(sizeof(CArenaBlock) + blockSize);
			if (block == NULL)
				return NULL;
			block->next = head;
			block->capacity = blockSize;
			head = block;
			offset = 0;
		}

		wchar_t *result = (wchar_t *)((char *)(head + 1) + offset);
		offset += allocSize;
		return result;
	}
};

#endif
