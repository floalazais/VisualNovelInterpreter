#ifndef XALLOC_H
#define XALLOC_H

void *_xmalloc(size_t size, const char *file, int line, bool stretchy);
#define xmalloc(a) _xmalloc(a, __FILE__, __LINE__, false)
void *xrealloc(void *p, size_t size, const char *file, int line);
void xfree(void *ptr);
void print_leaks();

#endif /* end of include guard: XALLOC_H */
