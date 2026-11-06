// memtrack_redirect.h — подключается к tickets.c автоматически (см. CMakeLists.txt).
// malloc/free в заданиях — обычные, просто компилятор незаметно направляет их в счётчик памяти,
// чтобы программа могла показать утечки. Писать код можно как всегда.
#ifndef MEMTRACK_REDIRECT_H
#define MEMTRACK_REDIRECT_H

#include <stdlib.h>
#include "memtrack.h"

#define malloc(n)     mt_malloc(n)
#define calloc(n, s)  mt_calloc(n, s)
#define realloc(p, n) mt_realloc(p, n)
#define free(p)       mt_free(p)

#endif
