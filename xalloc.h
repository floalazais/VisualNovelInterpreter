#ifndef XALLOC_H
#define XALLOC_H

#include <stdlib.h>

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);

#endif /* end of include guard: XALLOC_H */
