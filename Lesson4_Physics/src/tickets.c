// tickets.c — файл заданий урока 4.
// Здесь 4 функции, и в каждой спрятан баг. Писать заново ничего не нужно:
// найди и поправь одну-две строки. Номера заданий совпадают с README.
#include "tickets.h"

#include <stdlib.h>
#include <string.h>

// =====================================================================
// Задание 2. Питомцы на шариках (сцена 2)
// Симптом: имена над шариками превращаются в мусор.
// =====================================================================
cpBody* spawn_pet(cpSpace* space, cpVect pos, const char* name)
{
    cpFloat radius = 22;
    cpBody* body = cpSpaceAddBody(space, cpBodyNew(1, cpMomentForCircle(1, 0, radius, cpvzero)));
    cpBodySetPosition(body, pos);
    cpShape* shape = cpSpaceAddShape(space, cpCircleShapeNew(body, radius, cpvzero));
    cpShapeSetFriction(shape, 0.7);
    cpShapeSetElasticity(shape, 0.5);

    Pet* pet = malloc(sizeof(Pet));
    pet->magic = PET_MAGIC;
    strncpy(pet->name, name, sizeof pet->name - 1);
    pet->name[sizeof pet->name - 1] = '\0';
    pet->mood = 5;

    cpBodySetUserData(body, &pet);   // запоминаем питомца в теле
    return body;
}

// =====================================================================
// Задание 3. Кто кого вызывает (сцена 3)
// Симптом: шарик бьётся о стену, а вспыхивает пол. Счётчик стен всегда 0.
// =====================================================================
cpBool on_floor_begin(cpArbiter* arb, cpSpace* space, cpDataPointer data)
{
    Stats* stats = (Stats*)data;   // Chipmunk передаёт нам то, что мы положили в userData
    stats->floor_hits++;
    stats->floor_flash = 1;
    return cpTrue;                 // cpTrue — «да, пусть столкнутся»
}

cpBool on_wall_begin(cpArbiter* arb, cpSpace* space, cpDataPointer data)
{
    Stats* stats = (Stats*)data;
    stats->wall_hits++;
    stats->wall_flash = 1;
    return cpTrue;
}

void setup_handlers(cpSpace* space, Stats* stats)
{
    // «Когда шарик впервые коснётся пола — вызови вот эту функцию»
    cpCollisionHandler* floor_h = cpSpaceAddCollisionHandler(space, COLL_BALL, COLL_FLOOR);
    floor_h->beginFunc = on_floor_begin;
    floor_h->userData = stats;

    // То же самое для стен
    cpCollisionHandler* wall_h = cpSpaceAddCollisionHandler(space, COLL_BALL, COLL_WALL);
    wall_h->beginFunc = on_floor_begin;
    wall_h->userData = stats;
}

// =====================================================================
// Задание 4. Мост (сцена 4)
// Симптом: правый клик по доске — программа падает.
// =====================================================================
void remove_plank(Bridge* b, int i)
{
    // форма доски
    cpSpaceRemoveShape(b->space, b->shape[i]);
    cpShapeFree(b->shape[i]);
    b->shape[i] = NULL;

    // само тело доски
    cpSpaceRemoveBody(b->space, b->plank[i]);
    cpBodyFree(b->plank[i]);
    b->plank[i] = NULL;

    // шарниры слева и справа от доски
    for (int j = i; j <= i + 1; j++) {
        if (b->joint[j]) {
            cpSpaceRemoveConstraint(b->space, b->joint[j]);
            cpConstraintFree(b->joint[j]);
            b->joint[j] = NULL;
        }
    }
}

// =====================================================================
// Задание 5. Уборка (сцена 5)
// Симптом: после каждого выхода с уровня в куче остаются блоки — утечка.
// Сначала реши головоломку в сцене 5, потом расставь здесь строки в том же порядке.
// =====================================================================
void level_cleanup(Level* lv)
{
    cpSpaceFree(lv->space);

    // Этих строк не хватает. Раскомментируй и расставь в правильном порядке:
    //
    // for (int i = 0; i < lv->body_count; i++) {
    //     cpSpaceRemoveBody(lv->space, lv->bodies[i]);
    //     cpBodyFree(lv->bodies[i]);
    // }
    //
    // for (int i = 0; i < lv->body_count; i++)
    //     free(cpBodyGetUserData(lv->bodies[i]));      // питомцы; у ящиков там NULL — free(NULL) можно
    //
    // for (int i = 0; i < lv->shape_count; i++) {
    //     cpSpaceRemoveShape(lv->space, lv->shapes[i]);
    //     cpShapeFree(lv->shapes[i]);
    // }
}
