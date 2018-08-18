#ifndef STRETCHY_BUFFER_H
#define STRETCHY_BUFFER_H

#include <stdlib.h>
#include <stddef.h>

typedef struct StretchyBufferHeader
{
	size_t count;
	size_t capacity;
	char data[];
} StretchyBufferHeader;

#define _buf_header(b) ((StretchyBufferHeader *)((char *)(b) - offsetof(StretchyBufferHeader, data)))
#define _buf_capacity(b) (!(b) ? 0 : _buf_header(b)->capacity)
#define buf_len(b) (!(b) ? 0 : _buf_header(b)->count)
#define buf_free(b) (!(b) ? free(NULL) : free(_buf_header(b)))

void* sbuffer_create_or_grow_if_needed(void *buffer, size_t elementSize);

#define buf_add(b, item) ((b) = sbuffer_create_or_grow_if_needed(b, sizeof(*b)), (b)[_buf_header(b)->count++] = item)

#endif /* end of include guard: STRETCHY_BUFFER_H */
