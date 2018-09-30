#ifndef XALLOC_H
#define XALLOC_H

#include <stdlib.h>

void *xmalloc_int(size_t size, char *file, int line);
#define xmalloc(a) xmalloc_int(a, __FILE__, __LINE__)
void xfree(void* ptr);
void print_leaks();

#endif /* end of include guard: XALLOC_H */
