// Сцена 1. Двумерное поле в одномерной памяти: какая клетка по какому индексу.
#include "engine.h"
#include "scenes.h"

#include <stdio.h>

#define N 12

static Grid g;
static const char* message;
static char  message_buf[200];
static Color message_color;

static Demo demo;
static int  demo_row = -1;

static Rectangle field(void) { return (Rectangle){ FIELD_X, FIELD_Y, FIELD_W, FIELD_H }; }

static void init(void)
{
    grid_init(&g, N, N);
    // буква «Г»: её легко узнать, даже если она отразится
    for (int y = 2; y <= 8; y++) grid_set(&g, 3, y, true);
    for (int x = 3; x <= 7; x++) grid_set(&g, x, 2, true);
    demo_stop(&demo);
    message = "Кликай по клеткам. Справа — та же память, но как она лежит на самом деле: одной строкой.";
    message_color = COL_DIM;
}

static void unload(void) { grid_free(&g); }

static void demo_step(void)
{
    if (demo_row + 1 >= N) {
        demo_stop(&demo);
        demo_row = -1;
        message = "Ряды лежат друг за другом. Чтобы дойти до ряда y, надо пропустить y целых рядов по w клеток.";
        message_color = COL_GOOD;
        return;
    }
    demo_row++;
    viz_log(TextFormat("// ряд y = %d", demo_row), TextFormat("индексы %d ... %d", demo_row * N, demo_row * N + N - 1));
}

static void update(float dt)
{
    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        if (!demo.active && demo_row >= 0) demo_row = -1;
        return;
    }
    int x, y;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && grid_cell_at(&g, field(), GetMousePosition(), &x, &y)) {
        int i = cell_index(&g, x, y);   // задание 1
        if (i < 0 || i >= N * N) {
            snprintf(message_buf, sizeof message_buf, "cell_index вернула %d — это за пределами массива (0..%d)!", i, N * N - 1);
            message = message_buf;
            message_color = COL_BAD;
        } else {
            g.cur[i] = !g.cur[i];
            viz_log(TextFormat("g->cur[cell_index(g, %d, %d)] ^= 1;", x, y), TextFormat("индекс %d", i));
            int rx = i % N, ry = i / N;
            if (rx != x || ry != y) {
                snprintf(message_buf, sizeof message_buf,
                         "Кликнула (%d, %d), а индекс %d — это клетка (%d, %d). Смотри задание 1.", x, y, i, rx, ry);
                message_color = COL_BAD;
            } else {
                snprintf(message_buf, sizeof message_buf, "Клетка (%d, %d) лежит по индексу %d — всё верно.", x, y, i);
                message_color = COL_GOOD;
            }
            message = message_buf;
        }
    }
    if (IsKeyPressed(KEY_D)) { demo_start(&demo); demo_row = -1; message = ""; }
    if (IsKeyPressed(KEY_C)) { grid_clear(&g); viz_log("memset(g->cur, 0, w * h);", "все клетки мёртвые"); }
}

static void draw_strip(float x, float y, int hl_ref, int hl_student)
{
    const int per_row = 24;
    const float c = 20;
    viz_text(TextFormat("g->cur — одна строка из w × h = %d байт", N * N), x, y, 18, COL_TEXT);
    y += 30;
    for (int i = 0; i < N * N; i++) {
        Rectangle r = { x + (i % per_row) * (c + 1), y + (i / per_row) * (c + 6), c, c };
        int row = i / N;   // какому ряду поля принадлежит байт
        Color band = row % 2 ? viz_tint(COL_ACCENT, 50) : viz_tint(COL_PTR, 40);
        if (demo.active && row == demo_row) band = viz_tint(COL_HILITE, 140);
        DrawRectangleRec(r, g.cur[i] ? COL_TEXT : band);
        if (i % N == 0) viz_code(TextFormat("%d", i), r.x, r.y + c, 12, COL_DIM);
    }
    if (hl_ref >= 0) {
        Rectangle r = { x + (hl_ref % per_row) * (c + 1), y + (hl_ref / per_row) * (c + 6), c, c };
        viz_frame((Rectangle){ r.x - 2, r.y - 2, r.width + 4, r.height + 4 }, 2, COL_HILITE);
    }
    if (hl_student >= 0 && hl_student < N * N && hl_student != hl_ref) {
        Rectangle r = { x + (hl_student % per_row) * (c + 1), y + (hl_student / per_row) * (c + 6), c, c };
        viz_frame((Rectangle){ r.x - 2, r.y - 2, r.width + 4, r.height + 4 }, 2, COL_BAD);
    }
}

static void draw(void)
{
    viz_title("Клетки в памяти", "Двумерного массива в памяти нет: поле лежит одной длинной строкой, ряд за рядом.");

    grid_draw(&g, field(), COL_ACCENT);
    if (demo.active && demo_row >= 0)
        for (int x = 0; x < N; x++) viz_frame(grid_cell_rect(&g, field(), x, demo_row), 2, COL_HILITE);

    int x, y, ref = -1, stu = -1;
    if (!demo.active && grid_cell_at(&g, field(), GetMousePosition(), &x, &y)) {
        ref = y * N + x;
        stu = cell_index(&g, x, y);
        viz_frame(grid_cell_rect(&g, field(), x, y), 2, COL_HILITE);
        if (stu >= 0 && stu < N * N && stu != ref)
            viz_frame(grid_cell_rect(&g, field(), stu % N, stu / N), 3, COL_BAD);

        float ty = FIELD_Y + 200;
        viz_code(TextFormat("клетка: x = %d, y = %d", x, y), PANEL_X, ty, 20, COL_TEXT);
        viz_code(TextFormat("cell_index(g, %d, %d) = %d", x, y, stu), PANEL_X, ty + 30, 20, stu == ref ? COL_GOOD : COL_BAD);
        if (stu != ref) viz_text("Красная рамка — куда на самом деле попадёт клик.", PANEL_X, ty + 62, 17, COL_BAD);
    } else if (!demo.active) {
        viz_text("Наведи мышку на клетку: жёлтая рамка покажет её байт в строке.", PANEL_X, FIELD_Y + 200, 17, COL_DIM);
    }
    draw_strip(PANEL_X, FIELD_Y, ref, stu);
    viz_text("Полосы одного цвета — один ряд поля.", PANEL_X, FIELD_Y + 300, 17, COL_DIM);
    tasks_draw(PANEL_X, FIELD_Y + 440);

    viz_message(message, message_color);
    if (demo.active) demo_hint(&demo);
    else viz_hint("ЛКМ — оживить/убить клетку     D — как ряды лежат в памяти, по шагам     C — очистить");
}

const Scene scene_cells = { "Клетки", init, update, draw, unload };
