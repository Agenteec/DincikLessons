// tickets_fixed.c — исправленная версия tickets.c (собирается в LifeLab_Teacher). Дине не показывать.
#include "life.h"

#include <stdlib.h>

int cell_index(const Grid* g, int x, int y)
{
    return y * g->w + x;   // ИСПРАВЛЕНО: ряд y целиком (y * w клеток) + сдвиг x внутри ряда
}

int count_neighbors(const Grid* g, int x, int y)
{
    int count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = (x + dx + g->w) % g->w;   // ИСПРАВЛЕНО: в C (-1) % w == -1, поэтому сначала + w
            int ny = (y + dy + g->h) % g->h;
            count += g->cur[cell_index(g, nx, ny)];
        }
    }
    return count;
}

void life_step(Grid* g)
{
    for (int y = 0; y < g->h; y++) {
        for (int x = 0; x < g->w; x++) {
            int n = count_neighbors(g, x, y);
            int i = cell_index(g, x, y);
            int alive = g->cur[i];
            g->next[i] = (alive && (n == 2 || n == 3)) || (!alive && n == 3);   // ИСПРАВЛЕНО: пишем в next
        }
    }

    unsigned char* tmp = g->cur;
    g->cur = g->next;
    g->next = tmp;
}

void grid_resize(Grid* g, int w, int h)
{
    unsigned char* cur = calloc(w * h, 1);
    unsigned char* next = calloc(w * h, 1);

    for (int y = 0; y < h && y < g->h; y++)
        for (int x = 0; x < w && x < g->w; x++)
            cur[y * w + x] = g->cur[y * g->w + x];

    free(g->cur);    // ИСПРАВЛЕНО: старые буферы больше не нужны
    free(g->next);
    g->cur = cur;
    g->next = next;
    g->w = w;
    g->h = h;
}
