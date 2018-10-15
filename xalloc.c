#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>

#include "error.h"
#include "xalloc.h"

typedef struct Leak
{
    void* ptr;
    char* file;
    int line;
	bool stretchy;
} Leak;

static Leak* leaks = NULL;
static int nbLeaks = 0;

void *xmalloc_int(size_t size, char* file, int line, bool stretchy)
{
	if (!leaks)
	{
		leaks = malloc(sizeof (*leaks) * 10000000);
	}
    void *result = malloc(size);
    if (!result)
    {
        error("could not allocate memory.");
    } else {
        Leak leak = {result, file, line, stretchy};
        leaks[nbLeaks++] = leak;
        return result;
    }
}

void remove_from_leaks(void *ptr)
{
	for (size_t i = 0; i < nbLeaks; i++)
	{
		if (leaks[i].ptr == ptr)
		{
			leaks[i].ptr = NULL;
			break;
		}
	}
}

void *xrealloc(void *p, size_t size, char* file, int line)
{
    void *result = realloc(p, size);
    if (!result)
    {
        error("could not allocate memory.");
    } else {
		if (result != p)
		{
			remove_from_leaks(p);
			Leak leak = {result, file, line, true};
			leaks[nbLeaks++] = leak;
		}
        return result;
    }
}

void xfree(void* ptr)
{
    if (ptr)
    {
		remove_from_leaks(ptr);
    }
    free(ptr);
}

void print_leaks()
{
	int leaksCount = 0;
	printf("--- Memory leaks ---\n");
    for (size_t i = 0; i < nbLeaks; i++)
    {
        if (leaks[i].ptr)
        {
            printf("    -%s:%d &%d", leaks[i].file, leaks[i].line, (int)leaks[i].ptr);
			if (leaks[i].stretchy)
			{
				printf(" BUF");
			}
			printf("\n");
            fflush(stdout);
			leaksCount++;
        }
    }
	printf("%d memory leaks.\n", leaksCount);
}
