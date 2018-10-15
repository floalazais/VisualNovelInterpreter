#ifndef XALLOC_H
#define XALLOC_H

void *xmalloc_int(size_t size, char *file, int line, bool stretchy);
#define xmalloc(a) xmalloc_int(a, __FILE__, __LINE__, false)
void *xrealloc(void *p, size_t size, char* file, int line);
void xfree(void* ptr);
void print_leaks();

#endif /* end of include guard: XALLOC_H */
