// aquarium.h — все структуры данных аквариума и функции модулей.
#ifndef AQUARIUM_H
#define AQUARIUM_H

#include "raylib.h"

#define SCREEN_W 1000
#define SCREEN_H 600
#define NAME_LEN 8

// ---------- Рыбки (fish.c) ----------
typedef struct {
    char    name[NAME_LEN];
    Color   color;
    Vector2 pos;
    Vector2 vel;
    float   size;
    float   hunger;     // 0 — сытая, 1 — очень голодная
} Fish;

void        fish_init(Fish* f, const char* name, Color color, Vector2 pos);
void        fish_update(Fish* f, float dt);
void        fish_swim_to(Fish* f, Vector2 target, float dt);
void        fish_feed(Fish* f);
void        fish_draw(const Fish* f);
const char* fish_status(const Fish* f);

// ---------- Корм (food.c) ----------
typedef struct Food {
    Vector2      pos;
    struct Food* next;
} Food;

typedef struct {
    Food* head;
    int   count;
} FoodList;

void  food_drop(FoodList* list, Vector2 pos);
void  food_update(FoodList* list, Fish* fish, int fish_count, float dt);
Food* food_nearest(const FoodList* list, Vector2 from, float max_dist);
void  food_draw(const FoodList* list);
void  food_free_all(FoodList* list);

// ---------- Пузырьки (bubbles.c) ----------
typedef struct {
    Vector2 pos;
    float   radius;
    float   speed;
    float   wobble;
} Bubble;

typedef struct {
    Bubble** items;
    int      count;
    int      cap;
} BubblePool;

void bubbles_add(BubblePool* pool, Vector2 pos);
void bubbles_update(BubblePool* pool, float dt);
void bubbles_draw(const BubblePool* pool);
void bubbles_free_all(BubblePool* pool);

// ---------- Весь аквариум ----------
typedef struct {
    Fish*      fish;
    int        fish_count;
    FoodList   food;
    BubblePool bubbles;
} Aquarium;

// ---------- Интерфейс (hud.c) ----------
void hud_draw(const Aquarium* aq);

#endif
