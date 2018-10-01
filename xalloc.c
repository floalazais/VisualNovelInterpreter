#include "xalloc.h"
#include "globals.h"
#include "stretchy_buffer.h"
#include <stdio.h>

typedef struct Leak
{
    void* ptr;
    char* file;
    int line;
} Leak;

Leak* leaks = NULL;

void *xmalloc_int(size_t size, char* file, int line)
{
    void *result = malloc(size);
    if (!result)
    {
        error("could not allocate memory.");
    } else {
        Leak leak = {result, file, line};
        buf_add(leaks, leak);
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

void xfree(void* ptr)
{
    if (ptr)
    {
        for (size_t i = 0; i < buf_len(leaks); i++)
        {
            if (leaks[i].ptr == ptr)
            {
                leaks[i].ptr = NULL;
                break;
            }
        }
    }
    free(ptr);
}

void print_leaks()
{
    printf("Memory leaks :\n");
    for (size_t i = 0; i < buf_len(leaks); i++)
    {
        if (leaks[i].ptr)
        {
            printf("    -%s:%d &%d\n", leaks[i].file, leaks[i].line, (int)leaks[i].ptr);
            fflush(stdout);
        }
    }
}