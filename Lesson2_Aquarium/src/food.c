#include "aquarium.h"
#include "memstats.h"
#include "raymath.h"

#define FOOD_FALL_SPEED 35.0f
#define FOOD_BOTTOM     (SCREEN_H - 25)

void food_drop(FoodList* list, Vector2 pos)
{
    Food* f = MT_MALLOC(sizeof *f);
    f->pos = pos;
    f->next = list->head;
    list->head = f;
    list->count++;
}

static void food_remove(FoodList* list, Food* target)
{
    Food** pp = &list->head;
    while (*pp && *pp != target)
        pp = &(*pp)->next;

    if (*pp) {
        *pp = target->next;
        MT_FREE(target);
        list->count--;
    }
}

void food_update(FoodList* list, Fish* fish, int fish_count, float dt)
{
    for (Food* f = list->head; f; f = f->next) {
        f->pos.y += FOOD_FALL_SPEED * dt;
        if (f->pos.y > FOOD_BOTTOM) f->pos.y = FOOD_BOTTOM;

        // кто-нибудь из рыбок дотянулся до корма?
        for (int i = 0; i <= fish_count; i++) {
            if (CheckCollisionPointCircle(f->pos, fish[i].pos, fish[i].size)) {
                fish_feed(&fish[i]);
                food_remove(list, f);
                break;
            }
        }
    }
}

Food* food_nearest(const FoodList* list, Vector2 from, float max_dist)
{
    Food* best = NULL;
    float best_dist = max_dist;

    for (Food* f = list->head; f; f = f->next) {
        float d = Vector2Distance(from, f->pos);
        if (d < best_dist) {
            best_dist = d;
            best = f;
        }
    }
    return best;
}

void food_draw(const FoodList* list)
{
    for (const Food* f = list->head; f; f = f->next)
        DrawCircleV(f->pos, 4.0f, (Color){ 200, 140, 60, 255 });
}

void food_free_all(FoodList* list)
{
    Food* f = list->head;
    while (f) {
        Food* next = f->next;
        MT_FREE(f);
        f = next;
    }
    list->head = NULL;
    list->count = 0;
}
