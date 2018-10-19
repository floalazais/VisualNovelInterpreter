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

static Leak leaks[1000000];
static unsigned int nbLeaks = 0;

void *xmalloc_int(size_t size, char* file, int line, bool stretchy)
{
    void *result = malloc(size);
    if (!result)
    {
        error("could not allocate memory.");
    } else {
        Leak leak = {result, file, line, stretchy};
        leaks[nbLeaks++] = leak;
		if (nbLeaks == 1000000)
		{
			error("too much allocations.");
		}
        return result;
    }
}

void *xrealloc(void *ptr, size_t size, char* file, int line)
{
    void *result = realloc(ptr, size);
    if (!result)
    {
        error("could not allocate memory.");
    } else {
		if (result != ptr)
		{
			for (size_t i = 0; i < nbLeaks; i++)
			{
				if (leaks[i].ptr == ptr)
				{
					leaks[i].ptr = result;
					break;
				}
			}
		}
        return result;
    }
}

void xfree(void* ptr)
{
    if (ptr)
    {
		for (size_t i = 0; i < nbLeaks; i++)
		{
			if (leaks[i].ptr == ptr)
			{
				leaks[i] = leaks[nbLeaks - 1];
				nbLeaks--;
				break;
			}
		}
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
