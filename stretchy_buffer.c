#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "xalloc.h"
#include "stretchy_buffer.h"

void *sbuffer_create_or_grow_if_needed(void *buffer, size_t elementSize, const char *file, int line)
{
	if (buf_len(buffer) == _buf_capacity(buffer))
	{
		StretchyBufferHeader *newHeader;
		if (!buffer)
		{
			size_t newCapacity = 10;
			newHeader = _xmalloc(offsetof (StretchyBufferHeader, data) + elementSize * newCapacity, file, line, true);
			newHeader->capacity = newCapacity;
			newHeader->count = 0;
			return newHeader->data;
		} else {
			newHeader = _buf_header(buffer);
			newHeader->capacity *= 2;
			newHeader = xrealloc(newHeader, offsetof (StretchyBufferHeader, data) + _buf_capacity(buffer) * elementSize, file, line);
			return newHeader->data;
		}
	} else {
		return buffer;
	}
}