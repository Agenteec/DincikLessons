// phys.c — помощники для сцен. Здесь всё сделано правильно: это «эталонный» код, который можно читать.
#include "phys.h"
#include "chipmunk/chipmunk_structs.h"   // нужен только чтобы узнать тип формы (круг, отрезок, многоугольник)

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

cpSpace* phys_new_space(void)
{
    cpSpace* space = cpSpaceNew();
    cpSpaceSetGravity(space, cpv(0, 900));   // экранные координаты: y растёт вниз
    cpSpaceSetIterations(space, 20);
    return space;
}

void phys_step(cpSpace* space, float dt)
{
    // несколько маленьких шагов стабильнее одного большого
    if (dt > 1.0f / 20) dt = 1.0f / 20;
    int steps = (int)ceilf(dt / (1.0f / 240));
    for (int i = 0; i < steps; i++) cpSpaceStep(space, dt / steps);
}

int phys_add_walls(cpSpace* space, cpCollisionType floor_type, cpCollisionType wall_type, cpShape** out)
{
    cpBody* ground = cpSpaceGetStaticBody(space);
    float l = FIELD_X, r = FIELD_X + FIELD_W, t = FIELD_Y - 400, b = FIELD_Y + FIELD_H;
    cpVect seg[3][2] = {
        { cpv(l, b), cpv(r, b) },   // пол
        { cpv(l, t), cpv(l, b) },   // левая стена
        { cpv(r, t), cpv(r, b) },   // правая стена
    };
    for (int i = 0; i < 3; i++) {
        cpShape* s = cpSpaceAddShape(space, cpSegmentShapeNew(ground, seg[i][0], seg[i][1], 4));
        cpShapeSetFriction(s, 0.8);
        cpShapeSetElasticity(s, 0.6);
        cpShapeSetCollisionType(s, i == 0 ? floor_type : wall_type);
        shape_set_color(s, COL_DIM);
        if (out) out[i] = s;
    }
    return 3;
}

cpBody* phys_add_box(cpSpace* space, cpVect pos, cpFloat w, cpFloat h, Color c)
{
    cpBody* body = cpSpaceAddBody(space, cpBodyNew(1, cpMomentForBox(1, w, h)));
    cpBodySetPosition(body, pos);
    cpShape* shape = cpSpaceAddShape(space, cpBoxShapeNew(body, w, h, 2));
    cpShapeSetFriction(shape, 0.7);
    cpShapeSetElasticity(shape, 0.2);
    shape_set_color(shape, c);
    return body;
}

cpBody* phys_add_ball(cpSpace* space, cpVect pos, cpFloat r, Color c)
{
    cpBody* body = cpSpaceAddBody(space, cpBodyNew(1, cpMomentForCircle(1, 0, r, cpvzero)));
    cpBodySetPosition(body, pos);
    cpShape* shape = cpSpaceAddShape(space, cpCircleShapeNew(body, r, cpvzero));
    cpShapeSetFriction(shape, 0.7);
    cpShapeSetElasticity(shape, 0.6);
    shape_set_color(shape, c);
    return body;
}

// Цвет храним прямо в userData формы: 4 байта цвета влезают в 8 байт указателя.
// Это тоже трюк с void*: не обязательно адрес, главное — договориться, как читать.
void shape_set_color(cpShape* shape, Color c)
{
    uintptr_t packed = ((uintptr_t)c.r << 24) | ((uintptr_t)c.g << 16) | ((uintptr_t)c.b << 8) | 1;
    cpShapeSetUserData(shape, (cpDataPointer)packed);
}

static Color shape_color(const cpShape* shape)
{
    uintptr_t packed = (uintptr_t)cpShapeGetUserData(shape);
    if (!(packed & 1)) return COL_ACCENT;
    return (Color){ (unsigned char)(packed >> 24), (unsigned char)(packed >> 16), (unsigned char)(packed >> 8), 255 };
}

static void color_each(cpBody* body, cpShape* shape, void* data)
{
    (void)body;
    shape_set_color(shape, *(Color*)data);
}

void body_set_color(cpBody* body, Color c) { cpBodyEachShape(body, color_each, &c); }

static Vector2 v2(cpVect v) { return (Vector2){ (float)v.x, (float)v.y }; }

static void draw_shape(cpShape* shape, void* data)
{
    (void)data;
    cpBody* body = cpShapeGetBody(shape);
    Color c = shape_color(shape);

    switch (shape->klass->type) {
    case CP_CIRCLE_SHAPE: {
        cpVect center = cpBodyLocalToWorld(body, cpCircleShapeGetOffset(shape));
        float r = (float)cpCircleShapeGetRadius(shape);
        DrawCircleV(v2(center), r, c);
        // чёрточка показывает, что шарик крутится
        cpVect edge = cpBodyLocalToWorld(body, cpvadd(cpCircleShapeGetOffset(shape), cpv(r * 0.8, 0)));
        DrawLineEx(v2(center), v2(edge), 2, (Color){ 0, 0, 0, 90 });
        break;
    }
    case CP_SEGMENT_SHAPE:
        DrawLineEx(v2(cpBodyLocalToWorld(body, cpSegmentShapeGetA(shape))),
                   v2(cpBodyLocalToWorld(body, cpSegmentShapeGetB(shape))),
                   (float)cpSegmentShapeGetRadius(shape) * 2, c);
        break;
    case CP_POLY_SHAPE: {
        int n = cpPolyShapeGetCount(shape);
        if (n < 3 || n > 16) break;
        Vector2 p[16];
        for (int i = 0; i < n; i++) p[i] = v2(cpBodyLocalToWorld(body, cpPolyShapeGetVert(shape, i)));
        for (int i = 1; i < n - 1; i++) {
            DrawTriangle(p[0], p[i], p[i + 1], c);
            DrawTriangle(p[0], p[i + 1], p[i], c);   // один из двух обходов всегда правильный
        }
        Color edge = { 0, 0, 0, 70 };
        for (int i = 0; i < n; i++) DrawLineEx(p[i], p[(i + 1) % n], 1.5f, edge);
        break;
    }
    default:
        break;
    }
}

void phys_draw(cpSpace* space)
{
    Rectangle f = field_rect();
    viz_fill(f, (Color){ COL_PANEL.r, COL_PANEL.g, COL_PANEL.b, 120 });
    BeginScissorMode((int)f.x - 6, (int)f.y, (int)f.width + 12, (int)f.height + 6);
    cpSpaceEachShape(space, draw_shape, NULL);
    EndScissorMode();
}

cpBody* phys_body_at(cpSpace* space, Vector2 p)
{
    cpPointQueryInfo info;
    cpShape* s = cpSpacePointQueryNearest(space, cpv(p.x, p.y), 0, CP_SHAPE_FILTER_ALL, &info);
    if (!s) return NULL;
    cpBody* b = cpShapeGetBody(s);
    return cpBodyGetType(b) == CP_BODY_TYPE_DYNAMIC ? b : NULL;
}

// ---------- уборка ----------
// Пока Chipmunk обходит списки, менять их нельзя. Поэтому сначала собираем адреса в массив, потом удаляем.
typedef struct { void** items; int count, cap; } PtrList;

static void list_add(PtrList* l, void* p)
{
    if (l->count == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 16;
        l->items = realloc(l->items, l->cap * sizeof *l->items);
    }
    l->items[l->count++] = p;
}

static void collect_constraint(cpBody* body, cpConstraint* c, void* data) { (void)body; list_add(data, c); }
static void collect_shape(cpBody* body, cpShape* s, void* data) { (void)body; list_add(data, s); }

void phys_remove_body(cpSpace* space, cpBody* body)
{
    PtrList cons = { 0 }, shapes = { 0 };
    cpBodyEachConstraint(body, collect_constraint, &cons);
    cpBodyEachShape(body, collect_shape, &shapes);

    for (int i = 0; i < cons.count; i++) {       // 1. шарниры — они помнят адрес тела
        cpSpaceRemoveConstraint(space, cons.items[i]);
        cpConstraintFree(cons.items[i]);
    }
    for (int i = 0; i < shapes.count; i++) {     // 2. формы — они тоже помнят адрес тела
        cpSpaceRemoveShape(space, shapes.items[i]);
        cpShapeFree(shapes.items[i]);
    }
    cpSpaceRemoveBody(space, body);              // 3. и только теперь само тело
    cpBodyFree(body);

    free(cons.items);
    free(shapes.items);
}

static void collect_space_constraint(cpConstraint* c, void* data) { list_add(data, c); }
static void collect_space_shape(cpShape* s, void* data) { list_add(data, s); }
static void collect_space_body(cpBody* b, void* data) { list_add(data, b); }

void phys_free_space(cpSpace* space, bool free_user_data)
{
    PtrList cons = { 0 }, shapes = { 0 }, bodies = { 0 };
    cpSpaceEachConstraint(space, collect_space_constraint, &cons);
    cpSpaceEachShape(space, collect_space_shape, &shapes);
    cpSpaceEachBody(space, collect_space_body, &bodies);

    for (int i = 0; i < cons.count; i++) { cpSpaceRemoveConstraint(space, cons.items[i]); cpConstraintFree(cons.items[i]); }
    for (int i = 0; i < shapes.count; i++) { cpSpaceRemoveShape(space, shapes.items[i]); cpShapeFree(shapes.items[i]); }
    for (int i = 0; i < bodies.count; i++) {
        if (free_user_data) mt_free(cpBodyGetUserData(bodies.items[i]));
        cpSpaceRemoveBody(space, bodies.items[i]);
        cpBodyFree(bodies.items[i]);
    }
    cpSpaceFree(space);

    free(cons.items);
    free(shapes.items);
    free(bodies.items);
}
