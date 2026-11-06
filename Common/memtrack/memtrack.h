// memtrack.h — счётчик памяти: через него идут все calloc/free Chipmunk и malloc/free в tickets.c.
// Сцены рисуют по нему «кучу»: сколько блоков живо, сколько байт, что выделилось только что.
#ifndef MEMTRACK_H
#define MEMTRACK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* mt_malloc(size_t n);
void* mt_calloc(size_t count, size_t size);
void* mt_realloc(void* p, size_t n);
void  mt_free(void* p);

typedef struct {
    int         kind;   // +1 — выделили, -1 — освободили, 0 — realloc (блок переехал или вырос)
    size_t      size;
    const void* ptr;
    long long   seq;    // номер события по порядку
} MtEvent;

// Метка владельца: всё, что выделено, пока стоит метка, помнит её. Так сцена «Уборка» считает
// только блоки своего уровня, а не память других сцен.
int       mt_set_tag(int tag);   // возвращает прежнюю метку
int       mt_tag_blocks(int tag);
size_t    mt_tag_bytes(int tag);

int       mt_live_blocks(void);
size_t    mt_live_bytes(void);
long long mt_seq(void);                                       // номер последнего события
int       mt_events_since(long long seq, MtEvent* out, int max);   // события после seq, по порядку

#ifdef __cplusplus
}
#endif

#endif
