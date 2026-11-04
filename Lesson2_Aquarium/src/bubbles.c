#include "aquarium.h"
#include "memstats.h"

#include <math.h>

void bubbles_add(BubblePool* pool, Vector2 pos)
{
    // массив указателей растёт в два раза, когда заканчивается место
    if (pool->count == pool->cap) {
        pool->cap = pool->cap ? pool->cap * 2 : 4;
        pool->items = MT_REALLOC(pool->items, pool->cap);
    }

    Bubble* b = MT_MALLOC(sizeof *b);
    b->pos = pos;
    b->radius = (float)GetRandomValue(3, 9);
    b->wobble = (float)GetRandomValue(0, 628) / 100.0f;

    pool->items[pool->count++] = b;
}

void bubbles_update(BubblePool* pool, float dt)
{
    float t = (float)GetTime();

    for (int i = 0; i < pool->count; ) {
        Bubble* b = pool->items[i];
        b->pos.y -= b->speed * dt;
        b->pos.x += sinf(t * 3.0f + b->wobble) * 0.6f;

        // улетел за верх экрана — удаляем, на его место ставим последний
        if (b->pos.y < -b->radius) {
            MT_FREE(b);
            pool->items[i] = pool->items[--pool->count];
        } else {
            i++;
        }
    }
}

void bubbles_draw(const BubblePool* pool)
{
    for (int i = 0; i < pool->count; i++) {
        const Bubble* b = pool->items[i];
        DrawCircleLinesV(b->pos, b->radius, (Color){ 220, 240, 255, 200 });
        DrawCircleV((Vector2){ b->pos.x - b->radius * 0.3f, b->pos.y - b->radius * 0.3f },
                    b->radius * 0.25f, (Color){ 255, 255, 255, 160 });
    }
}

void bubbles_free_all(BubblePool* pool)
{
    for (int i = 0; i < pool->count; i++)
        MT_FREE(pool->items[i]);
    MT_FREE(pool->items);

    pool->items = NULL;
    pool->count = 0;
    pool->cap = 0;
}
