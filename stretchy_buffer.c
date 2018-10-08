#include <stdlib.h>
#include <stddef.h>

#include "xalloc.h"
#include "stretchy_buffer.h"

void* sbuffer_create_or_grow_if_needed(void *buffer, size_t elementSize)
{
	if (buf_len(buffer) == _buf_capacity(buffer))
	{
		StretchyBufferHeader *newHeader;
		if (!buffer)
		{
			size_t newCapacity = 1;
			newHeader = malloc(offsetof(StretchyBufferHeader, data) + elementSize * newCapacity);
			newHeader->capacity = newCapacity;
			newHeader->count = 0;
			return newHeader->data;
		} else {
			newHeader = _buf_header(buffer);
			newHeader->capacity *= 2;
			newHeader = realloc(newHeader, offsetof(StretchyBufferHeader, data) + _buf_capacity(buffer) * elementSize);
			return newHeader->data;
		}
	} else {
		return buffer;
	}
}
