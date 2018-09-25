#include "xalloc.h"
#include "globals.h"
#include "stretchy_buffer.h"
#include <stdio.h>

typedef struct Lol {
    void* ptr;
    char* file;
    int line;
} Lol;

Lol* lols = NULL;

void *xmalloc_int(size_t size, char* file, int line)
{
    void *result = malloc(size);
    if (!result)
    {
        error("could not allocate memory.");
    } else {
        Lol l = {result, file, line};
        buf_add(lols, l);
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

void xfree(void* ptr) {
    if (ptr != NULL) {
        for (size_t i = 0; i < buf_len(lols); i++) {
            if (lols[i].ptr == ptr) {
                lols[i].ptr = NULL;
                break;
            }
        }
    }
    free(ptr);
}

void print_lol() {
    printf("MEMORY LEAK TEST\n");
    for (size_t i = 0; i < buf_len(lols); i++) {
        Lol l = lols[i];
        if (l.ptr != NULL) {
            printf("MEMORY LEAK %s %d %d\n", l.file, l.line, (int)l.ptr);
            fflush(stdout);
        }
    }
}
