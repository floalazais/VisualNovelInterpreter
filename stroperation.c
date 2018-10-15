#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "stretchy_buffer.h"
#include "stroperation.h"

char *strcopy(char *destination, char *source)
{
	for (unsigned int i = 0; i < strlen(source); i++)
	{
		buf_add(destination, source[i]);
	}
	buf_add(destination, '\0');
	return destination;
}

char *strappend(char *destination, char *suffix)
{
	if (buf_len(destination) >= 1)
	{
		if (destination[buf_len(destination) - 1] == '\0')
		{
			destination[buf_len(destination) - 1] = suffix[0];
			for (unsigned int i = 1; i < strlen(suffix); i++)
			{
				buf_add(destination, suffix[i]);
			}
		}
	}
	 else {
		for (unsigned int i = 0; i < strlen(suffix); i++)
		{
			buf_add(destination, suffix[i]);
		}
	}
	buf_add(destination, '\0');
	return destination;
}

bool strmatch(char *a, char *b)
{
	return !strcmp(a, b);
}
