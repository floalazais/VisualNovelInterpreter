#include "xalloc.h"
#include "globals.h"

void *xmalloc(size_t size)
{
	void *result = malloc(size);
	if (!result)
	{
        error("could not allocate memory.");
    } else {
		return result;
	}
}

void *xrealloc(void *p, size_t size)
{
	void *result = realloc(p, size);
	if (!result)
	{
        error("could not allocate memory.");
    } else {
		return result;
	}
}
