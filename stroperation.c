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

int utf8Offset;
#define MAXUNICODE 0x10FFFF

int utf8_decode(const char *o)
{
	static const unsigned int limits[] = {0xFF, 0x7F, 0x7FF, 0xFFFF};
	const unsigned char *s = (const unsigned char *)o;
	unsigned int c = s[0];
	unsigned int res = 0;  /* final result */
	if (c < 0x80)  /* ascii? */
	{
		res = c;
		utf8Offset = 1;
	} else {
		int count = 0;  /* to count number of continuation bytes */
		while (c & 0x40)
		{  /* still have continuation bytes? */
			int cc = s[++count];  /* read next byte */
			if ((cc & 0xC0) != 0x80)  /* not a continuation byte? */
				return -1;  /* invalid byte sequence */
			res = (res << 6) | (cc & 0x3F);  /* add lower 6 bits from cont. byte */
			c <<= 1;  /* to test next bit */
		}
		res |= ((c & 0x7F) << (count * 5));  /* add first byte */
		if (count > 3 || res > MAXUNICODE || res <= limits[count])
			return -1;  /* invalid byte sequence */
		utf8Offset = count + 1;  /* skip continuation bytes read and first byte */
	}
	return res;
}