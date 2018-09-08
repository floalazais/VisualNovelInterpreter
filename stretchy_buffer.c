#include "xalloc.h"
#include "string.h"
#include "stretchy_buffer.h"

#include <stdio.h>

void* sbuffer_create_or_grow_if_needed(void *buffer, size_t elementSize)
{
	if (buf_len(buffer) == _buf_capacity(buffer))
	{
		StretchyBufferHeader *newHeader;
		if (!buffer)
		{
			size_t newCapacity = 1;
			newHeader = xmalloc(offsetof(StretchyBufferHeader, data) + elementSize * newCapacity);
			newHeader->capacity = newCapacity;
			newHeader->count = 0;
			return newHeader->data;
		} else {
			newHeader = _buf_header(buffer);
			newHeader->capacity *= 2;
			newHeader = xrealloc(newHeader, offsetof(StretchyBufferHeader, data) + _buf_capacity(buffer) * elementSize);
			return newHeader->data;
		}
	} else {
		return buffer;
	}
}
/*
void buf_string_add(char *destination, char *source)
{
	printf("source : %s\n", source);
	printf("destination : %s\n", destination);
	if (destination)
	{
		printf("on a déjà une destination\n");
		destination[buf_len(destination) - 1] = *source;
		for (int i = 1; i < strlen(source) + 1; i++)
		{
			printf("j'ajoute %c\n", source[i]);
			buf_add(destination, source[i]);
		}
	} else {
		printf("on génère une destination\n");
		for (int i = 0; i < strlen(source) + 1; i++)
		{
			printf("j'ajoute %c\n", source[i]);
			buf_add(destination, source[i]);
		}
	}
	printf("au final on a : %s\n", destination);
}
*/
