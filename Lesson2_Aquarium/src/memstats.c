#include "memstats.h"

#include <stdio.h>
#include <stdlib.h>

// Перед каждым блоком храним его размер. 16 байт — чтобы не сломать выравнивание.
#define HEADER_SIZE 16

static int    g_blocks = 0;
static size_t g_bytes  = 0;

void* mt_malloc(size_t size)
{
    unsigned char* base = malloc(HEADER_SIZE + size);
    if (!base) return NULL;

    *(size_t*)base = size;
    g_blocks++;
    g_bytes += size;
    return base + HEADER_SIZE;
}

void* mt_realloc(void* ptr, size_t size)
{
    if (!ptr) return mt_malloc(size);

    unsigned char* base = (unsigned char*)ptr - HEADER_SIZE;
    size_t old_size = *(size_t*)base;

    unsigned char* new_base = realloc(base, HEADER_SIZE + size);
    if (!new_base) return NULL;

    *(size_t*)new_base = size;
    g_bytes = g_bytes - old_size + size;
    return new_base + HEADER_SIZE;
}

void mt_free(void* ptr)
{
    if (!ptr) return;

    unsigned char* base = (unsigned char*)ptr - HEADER_SIZE;
    g_blocks--;
    g_bytes -= *(size_t*)base;
    free(base);
}

int memstats_blocks(void) { return g_blocks; }
size_t memstats_bytes(void) { return g_bytes; }

void memstats_report(void)
{
    if (g_blocks == 0)
        printf("memstats: утечек нет\n");
    else
        printf("memstats: НЕ освобождено %d блоков, %zu байт\n", g_blocks, g_bytes);
}
