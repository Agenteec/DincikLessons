// tickets.h — то, что используют сцены из файла заданий tickets.c.
#ifndef TICKETS_H
#define TICKETS_H

#include "chipmunk/chipmunk.h"

// ---------- Задание 2: void* userData ----------
// К каждому шарику привязан питомец. Chipmunk хранит его как void* — «адрес чего угодно».
#define PET_MAGIC 0x50E7   // проверочное число: по нему программа понимает, что по адресу правда лежит Pet

typedef struct {
    int  magic;
    char name[16];
    int  mood;
} Pet;

cpBody* spawn_pet(cpSpace* space, cpVect pos, const char* name);

// ---------- Задание 3: колбэки ----------
enum { COLL_BALL = 1, COLL_FLOOR, COLL_WALL };

typedef struct {
    int   floor_hits;
    int   wall_hits;
    float floor_flash;   // 1 — вспышка, плавно гаснет
    float wall_flash;
} Stats;

void   setup_handlers(cpSpace* space, Stats* stats);
cpBool on_floor_begin(cpArbiter* arb, cpSpace* space, cpDataPointer data);
cpBool on_wall_begin(cpArbiter* arb, cpSpace* space, cpDataPointer data);

// ---------- Задание 4: мост ----------
// joint[i] соединяет доску i-1 и доску i (joint[0] и joint[PLANKS] держатся за берега).
#define PLANKS 8

typedef struct {
    cpSpace*      space;
    cpBody*       plank[PLANKS];
    cpShape*      shape[PLANKS];
    cpConstraint* joint[PLANKS + 1];
} Bridge;

void remove_plank(Bridge* b, int i);

// ---------- Задание 5: уборка ----------
#define LEVEL_MAX 32

typedef struct {
    cpSpace* space;
    cpBody*  bodies[LEVEL_MAX];   // только подвижные тела (у ящиков userData = NULL, у питомцев — Pet*)
    int      body_count;
    cpShape* shapes[LEVEL_MAX];   // все формы, включая пол и стены
    int      shape_count;
} Level;

void level_cleanup(Level* lv);

#endif
