// memstats.h — простой учёт выделенной памяти.
// Все аллокации в проекте идут через MT_MALLOC / MT_REALLOC / MT_FREE,
// чтобы HUD мог показывать, сколько блоков сейчас живо.
#ifndef MEMSTATS_H
#define MEMSTATS_H

#include <stddef.h>

void* mt_malloc(size_t size);
void* mt_realloc(void* ptr, size_t size);
void  mt_free(void* ptr);

int    memstats_blocks(void);
size_t memstats_bytes(void);
void   memstats_report(void);   // печатает утечки при выходе

#define MT_MALLOC(size)       mt_malloc(size)
#define MT_REALLOC(ptr, size) mt_realloc(ptr, size)
#define MT_FREE(ptr)          mt_free(ptr)

#endif
