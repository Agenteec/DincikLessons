// memtrack.c — перед каждым блоком лежит маленький заголовок с размером, так мы знаем, сколько освобождаем.
#include "memtrack.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t   size;
    unsigned magic;   // MT_LIVE — блок жив, MT_DEAD — уже освобождён
    int      tag;     // чей это блок (см. mt_set_tag)
} Header;             // 16 байт: выравнивание данных после заголовка не ломается

#define MT_LIVE 0x4D454D31u
#define MT_DEAD 0xDEADF4EEu
#define TAG_MAX 64

#define EVENT_MAX 256

static int       g_blocks;
static size_t    g_bytes;
static long long g_seq;
static MtEvent   g_events[EVENT_MAX];
static int       g_tag;
static int       g_tag_blocks[TAG_MAX];
static size_t    g_tag_bytes[TAG_MAX];

static int slot(int tag) { return ((tag % TAG_MAX) + TAG_MAX) % TAG_MAX; }

int mt_set_tag(int tag) { int old = g_tag; g_tag = tag; return old; }
int mt_tag_blocks(int tag) { return g_tag_blocks[slot(tag)]; }
size_t mt_tag_bytes(int tag) { return g_tag_bytes[slot(tag)]; }

static void record(int kind, size_t size, const void* ptr)
{
    g_seq++;
    g_events[g_seq % EVENT_MAX] = (MtEvent){ kind, size, ptr, g_seq };
}

void* mt_malloc(size_t n)
{
    Header* h = malloc(sizeof(Header) + n);
    if (!h) return NULL;
    h->size = n;
    h->magic = MT_LIVE;
    h->tag = g_tag;
    g_blocks++;
    g_bytes += n;
    g_tag_blocks[slot(h->tag)]++;
    g_tag_bytes[slot(h->tag)] += n;
    record(+1, n, h + 1);
    memset(h + 1, 0xCD, n);   // как отладочная куча MSVC: свежий malloc — мусор 0xCD, а не нули
    return h + 1;
}

void* mt_calloc(size_t count, size_t size)
{
    size_t n = count * size;
    void* p = mt_malloc(n);
    if (p) memset(p, 0, n);
    return p;
}

void mt_free(void* p)
{
    if (!p) return;
    Header* h = (Header*)p - 1;
    if (h->magic != MT_LIVE) abort();   // двойной free или чужой указатель — падаем сразу и честно
    h->magic = MT_DEAD;
    g_blocks--;
    g_bytes -= h->size;
    g_tag_blocks[slot(h->tag)]--;
    g_tag_bytes[slot(h->tag)] -= h->size;
    record(-1, h->size, p);
    memset(p, 0xDD, h->size);           // как отладочная куча MSVC: освобождённое — 0xDD
    free(h);
}

void* mt_realloc(void* p, size_t n)
{
    if (!p) return mt_malloc(n);
    if (n == 0) { mt_free(p); return NULL; }
    Header* old = (Header*)p - 1;
    if (old->magic != MT_LIVE) abort();
    size_t old_size = old->size;
    Header* h = realloc(old, sizeof(Header) + n);
    if (!h) return NULL;
    h->size = n;
    g_bytes = g_bytes - old_size + n;
    g_tag_bytes[slot(h->tag)] = g_tag_bytes[slot(h->tag)] - old_size + n;
    record(0, n, h + 1);
    return h + 1;
}

int       mt_live_blocks(void) { return g_blocks; }
size_t    mt_live_bytes(void) { return g_bytes; }
long long mt_seq(void) { return g_seq; }

int mt_events_since(long long seq, MtEvent* out, int max)
{
    if (seq < g_seq - EVENT_MAX) seq = g_seq - EVENT_MAX;
    int n = 0;
    for (long long s = seq + 1; s <= g_seq && n < max; s++)
        out[n++] = g_events[s % EVENT_MAX];
    return n;
}
