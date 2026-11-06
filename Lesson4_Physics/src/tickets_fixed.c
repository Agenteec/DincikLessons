// tickets_fixed.c — исправленная версия tickets.c (собирается в PhysicsLab_Teacher). Дине не показывать.
#include "tickets.h"

#include <stdlib.h>
#include <string.h>

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

    cpBodySetUserData(body, pet);    // ИСПРАВЛЕНО: адрес питомца в куче, а не адрес локальной переменной pet
    return body;
}

cpBool on_floor_begin(cpArbiter* arb, cpSpace* space, cpDataPointer data)
{
    Stats* stats = (Stats*)data;
    stats->floor_hits++;
    stats->floor_flash = 1;
    return cpTrue;
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
    cpCollisionHandler* floor_h = cpSpaceAddCollisionHandler(space, COLL_BALL, COLL_FLOOR);
    floor_h->beginFunc = on_floor_begin;
    floor_h->userData = stats;

    cpCollisionHandler* wall_h = cpSpaceAddCollisionHandler(space, COLL_BALL, COLL_WALL);
    wall_h->beginFunc = on_wall_begin;   // ИСПРАВЛЕНО: была скопирована on_floor_begin
    wall_h->userData = stats;
}

void remove_plank(Bridge* b, int i)
{
    // ИСПРАВЛЕНО: сначала шарниры — они хранят адрес доски
    for (int j = i; j <= i + 1; j++) {
        if (b->joint[j]) {
            cpSpaceRemoveConstraint(b->space, b->joint[j]);
            cpConstraintFree(b->joint[j]);
            b->joint[j] = NULL;
        }
    }

    cpSpaceRemoveShape(b->space, b->shape[i]);
    cpShapeFree(b->shape[i]);
    b->shape[i] = NULL;

    cpSpaceRemoveBody(b->space, b->plank[i]);
    cpBodyFree(b->plank[i]);
    b->plank[i] = NULL;
}

void level_cleanup(Level* lv)
{
    // ИСПРАВЛЕНО: формы (они ссылаются на тела) → питомцы (адрес берём из живого тела) → тела → пространство
    for (int i = 0; i < lv->shape_count; i++) {
        cpSpaceRemoveShape(lv->space, lv->shapes[i]);
        cpShapeFree(lv->shapes[i]);
    }

    for (int i = 0; i < lv->body_count; i++)
        free(cpBodyGetUserData(lv->bodies[i]));

    for (int i = 0; i < lv->body_count; i++) {
        cpSpaceRemoveBody(lv->space, lv->bodies[i]);
        cpBodyFree(lv->bodies[i]);
    }

    cpSpaceFree(lv->space);
}
