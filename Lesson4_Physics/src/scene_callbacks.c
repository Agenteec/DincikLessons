// Сцена 3. Колбэки: мы отдаём Chipmunk АДРЕС своей функции, а он вызывает её, когда случится касание.
#include "phys.h"
#include "scenes.h"
#include "tickets.h"

#include <math.h>
#include <stdio.h>

static cpSpace* space;
static Stats    stats;
static cpShape* walls[3];   // [0] — пол, [1], [2] — стены
static const char* message;
static Color    message_color;

// пошаговое демо: как Chipmunk вызывает функцию по указателю
static Demo  demo;
static Stats demo_stats;   // демо считает отдельно, настоящие счётчики не трогает
static int   demo_target;  // 0 — пол, 1 — стена: про какое касание рассказываем

static const char* chipmunk_code[] = {
    "// внутри cpSpaceStep — это код Chipmunk",
    "h = найти обработчик(BALL, тип второго);",
    "if (касание только началось)",
    "    h->beginFunc(arb, space, h->userData);",
    "// ...а дальше выполняется НАША функция:",
    "Stats* stats = (Stats*)data;",
    "stats->..._hits++;",
};

static cpCollisionHandler* handler(int which)
{
    // повторный вызов не создаёт новый обработчик, а возвращает уже существующий
    return cpSpaceAddCollisionHandler(space, COLL_BALL, which == 0 ? COLL_FLOOR : COLL_WALL);
}

static const char* func_name(cpCollisionBeginFunc f)
{
    if (f == on_floor_begin) return "on_floor_begin";
    if (f == on_wall_begin)  return "on_wall_begin";
    return "(по умолчанию)";
}

static void init(void)
{
    space = phys_new_space();
    phys_add_walls(space, COLL_FLOOR, COLL_WALL, walls);
    stats = (Stats){ 0 };
    setup_handlers(space, &stats);   // функция из tickets.c
    demo_stop(&demo);
    message = "Кликни по полю: шарик полетит в сторону клика. Следи, что вспыхивает.";
    message_color = COL_DIM;
}

static void unload(void) { phys_free_space(space, false); }

static void mark_ball(cpBody* body, cpShape* shape, void* data)
{
    (void)body; (void)data;
    cpShapeSetCollisionType(shape, COLL_BALL);
}

static void demo_step(void)
{
    cpCollisionHandler* h = handler(demo_target);
    bool calls_floor = h->beginFunc == on_floor_begin;
    switch (demo.line) {
    case 0:
        viz_log("cpSpaceStep(space, dt);", demo_target ? "шарик коснулся СТЕНЫ" : "шарик коснулся ПОЛА");
        break;
    case 1:
        viz_log(TextFormat("h = %s_h;", demo_target ? "wall" : "floor"), TextFormat("обработчик %s", viz_addr(h)));
        break;
    case 2:
        viz_log("if (касание только началось)", "да — значит beginFunc");
        break;
    case 3:
        viz_log("h->beginFunc(arb, space, h->userData);",
                TextFormat("переход по адресу %s → %s", viz_addr((void*)h->beginFunc), func_name(h->beginFunc)));
        break;
    case 4:
        break;
    case 5:
        viz_log("Stats* stats = (Stats*)data;", TextFormat("data = %s — это наш stats", viz_addr(h->userData)));
        break;
    case 6:
        if (calls_floor) demo_stats.floor_hits++, demo_stats.floor_flash = 1;
        else             demo_stats.wall_hits++, demo_stats.wall_flash = 1;
        viz_log(calls_floor ? "stats->floor_hits++;" : "stats->wall_hits++;",
                calls_floor == (demo_target == 0) ? "посчитали правильно" : "посчитали НЕ ТО касание!");
        break;
    default:
        demo_stop(&demo);
        if (calls_floor != (demo_target == 0)) {
            message = "Касание стены попало в on_floor_begin: в tickets.c (задание 3) у wall_h не та функция.";
            message_color = COL_BAD;
        } else {
            message = "Chipmunk вызвал ту функцию, чей адрес лежит в beginFunc. Это и есть колбэк.";
            message_color = COL_GOOD;
        }
        return;
    }
    demo.line++;
}

static void update(float dt)
{
    stats.floor_flash = viz_approach(stats.floor_flash, 0, 2.5f, dt);
    stats.wall_flash = viz_approach(stats.wall_flash, 0, 2.5f, dt);
    demo_stats.floor_flash = viz_approach(demo_stats.floor_flash, 0, 1.5f, dt);
    demo_stats.wall_flash = viz_approach(demo_stats.wall_flash, 0, 1.5f, dt);

    phys_step(space, dt);

    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        return;
    }

    Vector2 m = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, field_rect())) {
        // шарик вылетает из центра в сторону клика
        cpVect from = cpv(FIELD_X + FIELD_W / 2, FIELD_Y + 80);
        cpBody* b = phys_add_ball(space, from, 16, (Color){ 255, 167, 38, 255 });
        cpBodyEachShape(b, mark_ball, NULL);
        cpBodySetVelocity(b, cpvmult(cpvnormalize(cpvsub(cpv(m.x, m.y), from)), 700));
        viz_log("cpShapeSetCollisionType(shape, COLL_BALL);", "теперь касания шарика ловят обработчики");
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        cpBody* b = phys_body_at(space, m);
        if (b) phys_remove_body(space, b);
    }
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_W)) {
        demo_target = IsKeyPressed(KEY_W) ? 1 : 0;
        demo_start(&demo);
        demo_stats = stats;
        message = "";
    }
    if (IsKeyPressed(KEY_R)) { unload(); init(); viz_log("// новый мир", NULL); }
}

static void draw_handler(Rectangle r, const char* name, cpCollisionHandler* h, bool hl, Rectangle* field_out)
{
    viz_fill(r, COL_PANEL);
    if (hl) viz_frame(r, 2, COL_ACCENT);
    viz_code(name, r.x + 10, r.y + 6, 18, COL_TEXT);
    Rectangle f1 = { r.x + 8, r.y + 32, r.width - 16, 24 };
    viz_fill(f1, COL_CELL);
    viz_code(TextFormat("beginFunc = %s", viz_addr((void*)h->beginFunc)), f1.x + 6, f1.y + 3, 16, COL_PTR);
    viz_code(TextFormat("userData  = %s", viz_addr(h->userData)), r.x + 14, r.y + 62, 16, COL_ADDR);
    *field_out = f1;
}

static void draw_pointers(float x, float y)
{
    viz_text("Указатели на функции", x, y, 18, COL_DIM);
    Rectangle hf = { x, y + 28, 240, 90 }, hw = { x, y + 130, 240, 90 };
    Rectangle ff = { x + 330, y + 40, 180, 50 }, fw = { x + 330, y + 150, 180, 50 };
    Rectangle bf, bw;
    bool step = demo.active;
    draw_handler(hf, "floor_h", handler(0), step && demo_target == 0 && demo.line >= 2, &bf);
    draw_handler(hw, "wall_h", handler(1), step && demo_target == 1 && demo.line >= 2, &bw);

    bool call = step && demo.line >= 4;
    cpCollisionBeginFunc tf = handler(demo_target)->beginFunc;
    Rectangle* boxes[2] = { &ff, &fw };
    cpCollisionBeginFunc funcs[2] = { on_floor_begin, on_wall_begin };
    for (int i = 0; i < 2; i++) {
        bool active = call && tf == funcs[i];
        viz_fill(*boxes[i], active ? viz_tint(COL_ACCENT, g_theme.hl_alpha) : COL_CELL);
        if (active) viz_glow(*boxes[i], 0.6f + 0.4f * sinf((float)GetTime() * 6), COL_ACCENT);
        viz_code(TextFormat("%s()", func_name(funcs[i])), boxes[i]->x + 10, boxes[i]->y + 6, 17, COL_TEXT);
        viz_code(viz_addr((void*)funcs[i]), boxes[i]->x + 10, boxes[i]->y + 28, 15, COL_ADDR);
    }
    // стрелки: куда на самом деле указывают beginFunc
    Rectangle* fields[2] = { &bf, &bw };
    for (int i = 0; i < 2; i++) {
        cpCollisionBeginFunc f = handler(i)->beginFunc;
        Rectangle* to = f == on_floor_begin ? &ff : (f == on_wall_begin ? &fw : NULL);
        if (!to) continue;
        bool wrong = (i == 0) != (to == &ff);
        Vector2 a = { fields[i]->x + fields[i]->width, fields[i]->y + 12 };
        Vector2 b = { to->x - 4, to->y + 25 };
        static char keys[2];
        viz_arrow(a, viz_smooth(&keys[i], 0, b), wrong ? COL_BAD : COL_PTR);
    }
}

static void draw(void)
{
    viz_title("Колбэки", "Мы говорим Chipmunk: «при касании вызови вот эту функцию» — и отдаём её АДРЕС.");

    // вспышки пола и стен
    const Stats* s = demo.active ? &demo_stats : &stats;
    phys_draw(space);
    Color glow_floor = viz_tint(COL_GOOD, (unsigned char)(255 * s->floor_flash));
    Color glow_wall = viz_tint(COL_HILITE, (unsigned char)(255 * s->wall_flash));
    DrawRectangle((int)FIELD_X, (int)(FIELD_Y + FIELD_H - 6), (int)FIELD_W, 12, glow_floor);
    DrawRectangle((int)FIELD_X - 6, (int)FIELD_Y, 12, (int)FIELD_H, glow_wall);
    DrawRectangle((int)(FIELD_X + FIELD_W - 6), (int)FIELD_Y, 12, (int)FIELD_H, glow_wall);

    viz_code(TextFormat("пол: %d", s->floor_hits), FIELD_X + 12, FIELD_Y + 10, 22, COL_GOOD);
    viz_code(TextFormat("стены: %d", s->wall_hits), FIELD_X + FIELD_W - 150, FIELD_Y + 10, 22, COL_HILITE);

    if (demo.active) {
        viz_code_panel(PANEL_X, FIELD_Y, PANEL_W, chipmunk_code, 7, demo.line - 1);
        draw_pointers(PANEL_X, FIELD_Y + 220);
        demo_hint(&demo);
    } else {
        draw_pointers(PANEL_X, FIELD_Y);
        viz_text("Имя функции без скобок — это её адрес:", PANEL_X, FIELD_Y + 260, 18, COL_TEXT);
        viz_code("floor_h->beginFunc = on_floor_begin;", PANEL_X, FIELD_Y + 290, 18, COL_ACCENT);
        viz_text("Скобки () — это уже вызов. Здесь мы не вызываем,", PANEL_X, FIELD_Y + 324, 17, COL_DIM);
        viz_text("а только говорим Chipmunk, КОГО вызывать потом.", PANEL_X, FIELD_Y + 346, 17, COL_DIM);
        viz_hint("ЛКМ — бросить шарик     D — касание пола по шагам     W — касание стены по шагам     R — сброс");
    }
    if (!demo.active) mem_panel(PANEL_X, FIELD_Y + 400, PANEL_W);
    viz_message(message, message_color);
}

const Scene scene_callbacks = { "Колбэки", init, update, draw, unload };
