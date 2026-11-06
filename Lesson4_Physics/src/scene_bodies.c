// Сцена 1. Мир и тела: что на самом деле происходит в памяти, когда в мире появляется ящик.
#include "phys.h"
#include "scenes.h"

#include "chipmunk/chipmunk_structs.h"   // только ради sizeof(cpSpace) в подписи

#include <stdio.h>

static cpSpace* space;
static int      boxes;
static const char* message;
static Color    message_color;

// последний ящик — его рисуем на схеме памяти справа
static cpBody*  last_body;
static cpShape* last_shape;
static size_t   last_body_size, last_shape_size;
static float    diagram_alpha;

// проигрывание кода после обычного клика
static int   play_line = -1;
static float play_timer;

// пошаговое демо: строка за строкой, по-настоящему
static Demo    demo;
static cpFloat demo_moment;
static cpVect  demo_pos;

static const char* spawn_code[] = {
    "cpFloat moment = cpMomentForBox(1, w, h);",
    "cpBody* body = cpBodyNew(1, moment);",
    "cpSpaceAddBody(space, body);",
    "cpShape* shape = cpBoxShapeNew(body, w, h, 2);",
    "cpSpaceAddShape(space, shape);",
};

static Color random_color(void)
{
    Color palette[] = { { 239, 83, 80, 255 }, { 66, 165, 245, 255 }, { 102, 187, 106, 255 },
                        { 255, 202, 40, 255 }, { 171, 71, 188, 255 }, { 38, 198, 218, 255 } };
    return palette[GetRandomValue(0, 5)];
}

// размер последнего блока, выделенного после seq (0 — если ничего не выделялось)
static size_t last_alloc_size(long long seq)
{
    MtEvent ev[32];
    int n = mt_events_since(seq, ev, 32);
    for (int i = n - 1; i >= 0; i--)
        if (ev[i].kind > 0) return ev[i].size;
    return 0;
}

static void init(void)
{
    space = phys_new_space();
    phys_add_walls(space, 0, 0, NULL);
    boxes = 0;
    last_body = NULL;
    last_shape = NULL;
    diagram_alpha = 0;
    play_line = -1;
    demo_stop(&demo);
    message = "Кликни по полю: появится ящик. Справа видно, сколько памяти он забрал.";
    message_color = COL_DIM;
}

// Демо прервали (X или смена сцены) посреди создания: убираем то, что уже успели выделить.
static void demo_abort(void)
{
    int line = demo.line;
    demo_stop(&demo);
    demo.line = 0;
    if (line < 2 || line > 4 || !last_body) return;
    if (line == 4 && last_shape) cpShapeFree(last_shape);              // форма создана, но не в мире
    if (line >= 3) cpSpaceRemoveBody(space, last_body);                  // тело уже в мире
    cpBodyFree(last_body);
    last_body = NULL;
    last_shape = NULL;
}

static void unload(void)
{
    demo_abort();
    phys_free_space(space, false);
}

static void spawn_box(Vector2 at)
{
    long long seq = mt_seq();
    cpFloat w = GetRandomValue(30, 60), h = GetRandomValue(30, 60);

    cpFloat moment = cpMomentForBox(1, w, h);
    long long before_body = mt_seq();
    cpBody* body = cpBodyNew(1, moment);
    last_body_size = last_alloc_size(before_body);
    cpSpaceAddBody(space, body);
    cpBodySetPosition(body, cpv(at.x, at.y));
    long long before_shape = mt_seq();
    cpShape* shape = cpBoxShapeNew(body, w, h, 2);
    last_shape_size = last_alloc_size(before_shape);
    cpSpaceAddShape(space, shape);
    cpShapeSetFriction(shape, 0.7);
    shape_set_color(shape, random_color());

    last_body = body;
    last_shape = shape;
    boxes++;
    viz_log("spawn_box();", TextFormat("ящик №%d", boxes));
    mem_log_since(seq);
    play_line = 0;
    play_timer = 0;
}

static void demo_step(void)
{
    long long seq = mt_seq();
    switch (demo.line) {
    case 0:
        demo_moment = cpMomentForBox(1, 50, 40);
        viz_log(spawn_code[0], TextFormat("moment = %.0f — просто число, памяти не нужно", demo_moment));
        demo.line = 1;
        break;
    case 1:
        last_body = cpBodyNew(1, demo_moment);
        last_body_size = last_alloc_size(seq);
        last_shape = NULL;
        cpBodySetPosition(last_body, cpv(demo_pos.x, demo_pos.y));
        viz_log(spawn_code[1], TextFormat("calloc(%zu) → body = %s", last_body_size, viz_addr(last_body)));
        demo.line = 2;
        break;
    case 2:
        cpSpaceAddBody(space, last_body);
        viz_log(spawn_code[2], "space запомнил адрес тела в своём массиве");
        mem_log_since(seq);
        demo.line = 3;
        break;
    case 3:
        last_shape = cpBoxShapeNew(last_body, 50, 40, 2);
        last_shape_size = last_alloc_size(seq);
        shape_set_color(last_shape, COL_ACCENT);
        viz_log(spawn_code[3], TextFormat("calloc(%zu) → shape = %s", last_shape_size, viz_addr(last_shape)));
        demo.line = 4;
        break;
    case 4:
        cpSpaceAddShape(space, last_shape);
        cpShapeSetFriction(last_shape, 0.7);
        boxes++;
        viz_log(spawn_code[4], "теперь ящик видно и он сталкивается");
        mem_log_since(seq);
        demo.line = 5;
        break;
    default:
        demo_stop(&demo);
        message = "Один ящик = 2 блока в куче: тело и форма. Иногда больше — Chipmunk расширяет свои массивы.";
        message_color = COL_GOOD;
        break;
    }
}

static void update(float dt)
{
    diagram_alpha = viz_approach(diagram_alpha, last_body ? 1.0f : 0.0f, 6, dt);
    if (play_line >= 0) {
        play_timer += dt;
        if (play_timer > 0.3f) { play_timer = 0; if (++play_line > 7) play_line = -1; }
    }

    // в демо тело без формы ещё не в мире — физику не останавливаем, но тело ждёт на месте
    if (demo.active && demo.line >= 2 && demo.line <= 4 && last_body) {
        cpBodySetPosition(last_body, cpv(demo_pos.x, demo_pos.y));
        cpBodySetVelocity(last_body, cpvzero);
    }
    phys_step(space, dt);

    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        if (!demo.active && demo.line >= 1 && demo.line <= 4) {   // нажали X
            demo.active = true;
            demo_abort();
            message = "Демо прервано: недостроенный ящик убран.";
            message_color = COL_DIM;
        }
        return;
    }

    Vector2 m = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, field_rect())) {
        spawn_box(m);
        message = "";
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        cpBody* b = phys_body_at(space, m);
        if (b) {
            long long seq = mt_seq();
            if (b == last_body) { last_body = NULL; last_shape = NULL; }
            phys_remove_body(space, b);
            boxes--;
            viz_log("phys_remove_body(space, body);", "форма и тело вернули память");
            mem_log_since(seq);
        }
    }
    if (IsKeyPressed(KEY_D)) {
        demo_start(&demo);
        demo_pos = cpv(FIELD_X + FIELD_W / 2, FIELD_Y + 60);
        last_body = NULL;
        last_shape = NULL;
        message = "";
    }
    if (IsKeyPressed(KEY_R)) { unload(); init(); viz_log("// новый мир", NULL); }
}

static void draw_block(Rectangle r, const char* type, size_t size, const void* addr, Color c, float a)
{
    unsigned char ua = (unsigned char)(255 * a);
    viz_fill(r, viz_tint(c, ua));
    viz_code(type, r.x + 12, r.y + 8, 20, viz_tint(COL_TEXT, ua));
    viz_code(TextFormat("%zu байт", size), r.x + 12, r.y + 34, 17, viz_tint(COL_TEXT, ua));
    viz_code(viz_addr(addr), r.x + r.width - 70, r.y + 8, 16, viz_tint(COL_ADDR, ua));
}

static void draw_diagram(float x, float y)
{
    viz_text("Последний ящик в памяти", x, y, 18, COL_DIM);
    Rectangle rs = { x, y + 30, 160, 60 };
    draw_block(rs, "cpSpace", sizeof(cpSpace), space, COL_CELL, 1);

    if (diagram_alpha < 0.02f) return;
    float a = diagram_alpha;
    Rectangle rb = { x + 200, y + 30, 150, 60 };
    Rectangle rh = { x + 200, y + 120, 150, 60 };
    if (last_body) {
        draw_block(rb, "cpBody", last_body_size, last_body, g_theme.ptr_tile, a);
        if (demo.line != 2 || !demo.active)
            viz_arrow((Vector2){ rs.x + rs.width, rs.y + 30 }, (Vector2){ rb.x - 4, rb.y + 30 }, viz_tint(COL_PTR, (unsigned char)(255 * a)));
    }
    if (last_shape) {
        draw_block(rh, "cpShape", last_shape_size, last_shape, (Color){ 0, 120, 140, 255 }, a);
        viz_arrow((Vector2){ rh.x + rh.width + 10, rh.y + 30 }, (Vector2){ rb.x + rb.width + 6, rb.y + 40 }, COL_PTR);
        viz_code("shape->body", rh.x + rh.width + 16, rh.y - 10, 16, COL_PTR);
        if (!demo.active || demo.line >= 5)
            viz_arrow((Vector2){ rs.x + rs.width / 2, rs.y + rs.height }, (Vector2){ rh.x - 4, rh.y + 30 }, COL_PTR);
    }
}

static void draw(void)
{
    viz_title("Мир и тела", "Chipmunk — чужая библиотека на C. Каждый ящик — это память, которую она выделила через calloc.");

    phys_draw(space);
    viz_code(TextFormat("ящиков: %d", boxes), FIELD_X + 10, FIELD_Y + 8, 18, COL_DIM);

    int hl = demo.active ? demo.line : (play_line >= 0 ? (play_line < 5 ? play_line : 4) : -1);
    viz_code_panel(PANEL_X, FIELD_Y, PANEL_W, spawn_code, 5, hl);
    draw_diagram(PANEL_X, FIELD_Y + 170);
    mem_panel(PANEL_X, FIELD_Y + 400, PANEL_W);

    viz_message(message, message_color);
    if (demo.active) demo_hint(&demo);
    else viz_hint("ЛКМ — ящик     ПКМ — убрать ящик     D — создание ящика по шагам     R — новый мир");
}

const Scene scene_bodies = { "Тела", init, update, draw, unload };
