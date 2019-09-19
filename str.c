#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include "xalloc.h"
#include "stretchy_buffer.h"
#include "str.h"

buf(char) strclone(const char *source)
{
	buf(char) result = NULL;
	for (int i = 0; source[i] != '\0'; i++)
	{
		buf_add(result, source[i]);
	}
	buf_add(result, '\0');
	return result;
}

buf(char) strclonen(const char *source, size_t length)
{
	buf(char) result = NULL;
	for (unsigned int i = 0; i < length && source[i] != '\0'; i++)
	{
		buf_add(result, source[i]);
	}
	buf_add(result, '\0');
	return result;
}

buf(char) strcopy(buf(char) *destination, const char *source)
{
	buf_clear(*destination);
	for (int i = 0; source[i] != '\0'; i++)
	{
		buf_add(*destination, source[i]);
	}
	buf_add(*destination, '\0');
	return *destination;
}

buf(char) strcopyn(buf(char) *destination, const char *source, size_t length)
{
	buf_clear(*destination);
	for (unsigned int i = 0; i < length && source[i] != '\0'; i++)
	{
		buf_add(*destination, source[i]);
	}
	buf_add(*destination, '\0');
	return *destination;
}

buf(char) strappend(buf(char) *destination, const char *suffix)
{
	if (buf_len(*destination) >= 1 && (*destination)[buf_len(*destination) - 1] == '\0')
	{
		_buf_header(*destination)->count--;
	}
	for (int i = 0; suffix[i] != '\0'; i++)
	{
		buf_add(*destination, suffix[i]);
	}
	buf_add(*destination, '\0');
	return *destination;
}

buf(char) strnappend(buf(char) *destination, int appendListLength, ...)
{
	if (appendListLength > 0)
	{
		if (buf_len(*destination) >= 1 && (*destination)[buf_len(*destination) - 1] == '\0')
		{
			_buf_header(*destination)->count--;
		}

		va_list arguments;
		va_start(arguments, appendListLength);
		const char *suffix;
		for (int i = 0; i < appendListLength; i++)
		{
			suffix = va_arg(arguments, const char *);
			for (int i = 0; suffix[i] != '\0'; i++)
			{
				buf_add(*destination, suffix[i]);
			}
		}
		va_end(arguments);
		buf_add(*destination, '\0');
	}
	return *destination;
}

buf(char) strmerge(const char *prefix, const char *suffix)
{
	buf(char) result = NULL;
	for (int i = 0; prefix[i] != '\0'; i++)
	{
		buf_add(result, prefix[i]);
	}
	for (int i = 0; suffix[i] != '\0'; i++)
	{
		buf_add(result, suffix[i]);
	}
	buf_add(result, '\0');
	return result;
}

bool strmatch(const char *a, const char *b)
{
	return !strcmp(a, b);
}

#define MAXUNICODE 0x10FFFF

buf(int) utf8_decode(const char *string)
{
	static const unsigned int limits[] = {0xFF, 0x7F, 0x7FF, 0xFFFF};
	const unsigned char *unsignedString = (const unsigned char *)string;

	buf(int) result = NULL;

	unsigned int currentChar;
	int currentCode;
	int currentIndex = 0;

	while (true)
	{
		currentChar = unsignedString[currentIndex];
		currentCode = 0;
		if (currentChar == 0)
		{
			break;
		} else if (currentChar < 0x80) { // ascii ?
			currentCode = currentChar;
			currentIndex++;
			buf_add(result, currentCode);
		} else {
			int count = 0; // to count number of continuation bytes
			while (currentChar & 0x40)
			{ // still have continuation bytes?
				count++;
				unsigned int continuationCharacter = unsignedString[currentIndex + count]; // read next byte
				if ((continuationCharacter & 0xC0) != 0x80) // not a continuation byte?
				{
					// invalid byte sequence
					buf_free(result);
					return NULL;
				}
				currentCode = (currentCode << 6) | (continuationCharacter & 0x3F); // add lower 6 bits from cont. byte
				currentChar <<= 1; // to test next bit
			}
			currentCode |= ((currentChar & 0x7F) << (count * 5)); // add first byte
			if (count > 3 || currentCode > MAXUNICODE || currentCode <= limits[count])
			{
				// invalid byte sequence
				buf_free(result);
				return NULL;
			}
			currentIndex += count + 1; // skip continuation bytes read and first byte
			buf_add(result, currentCode);
		}
	}
	return result;
}

buf(unsigned short) codepoint_to_utf16(const int *codepoints)
{
	buf(unsigned short) result = NULL;

	for (int currentCodepointIndex = 0; currentCodepointIndex < buf_len(codepoints); currentCodepointIndex++)
	{
		int currentCodepoint = codepoints[currentCodepointIndex];
		if (currentCodepoint < 0x10000)
		{
			buf_add(result, (unsigned short)currentCodepoint);
		} else {
			currentCodepoint -= 0x10000;
			unsigned short firstWord = 0b1101100000000000 + (currentCodepoint >> 10);
			unsigned short secondWord = 0b1101110000000000 + (currentCodepoint & 0b1111111111);
			buf_add(result, firstWord);
			buf_add(result, secondWord);
		}
	}
	return result;
}