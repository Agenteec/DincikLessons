// Сцена 2. Соседи: 8 клеток вокруг, поле замкнуто как бублик. И сюрприз: в C (-1) % 12 == -1.
#include "engine.h"
#include "scenes.h"

#include <stdio.h>

#define N 12

static Grid g;
static const char* message;
static char  message_buf[200];
static Color message_color;

static Demo demo;
static int  demo_x, demo_y, demo_k, demo_count;   // k — какой сосед из 8 сейчас

static const int DX[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
static const int DY[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

static const char* loop_code[] = {
    "for (dy = -1; dy <= 1; dy++)",
    "  for (dx = -1; dx <= 1; dx++) {",
    "    nx = заворачиваем(x + dx);",
    "    ny = заворачиваем(y + dy);",
    "    count += g->cur[cell_index(g, nx, ny)];",
    "  }",
};

static Rectangle field(void) { return (Rectangle){ FIELD_X, FIELD_Y, FIELD_W, FIELD_H }; }

static void init(void)
{
    grid_init(&g, N, N);
    // узоры у краёв: тут ошибки с заворачиванием видно сразу
    int cells[][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 11, 11 }, { 0, 5 }, { 0, 6 }, { 0, 7 },
                       { 5, 0 }, { 6, 1 }, { 4, 2 }, { 5, 2 }, { 6, 2 }, { 8, 8 }, { 9, 8 } };
    for (int i = 0; i < 14; i++) grid_set(&g, cells[i][0], cells[i][1], true);
    demo_stop(&demo);
    message = "Цифра в клетке — сколько соседей насчитала твоя count_neighbors. Красные — где она ошиблась.";
    message_color = COL_DIM;
}

static void unload(void) { grid_free(&g); }

static bool student_ok(void) { return tasks_check().index; }   // без задания 1 соседи ищутся не там

static void demo_step(void)
{
    if (demo_k >= 8) {
        demo_stop(&demo);
        snprintf(message_buf, sizeof message_buf, "У клетки (%d, %d) %d живых соседей. Твоя функция насчитала %d.",
                 demo_x, demo_y, demo_count, count_neighbors(&g, demo_x, demo_y));
        message = message_buf;
        message_color = demo_count == count_neighbors(&g, demo_x, demo_y) ? COL_GOOD : COL_BAD;
        return;
    }
    int rx = demo_x + DX[demo_k], ry = demo_y + DY[demo_k];
    int cx = rx % N, cy = ry % N;   // так считает C
    int wx = (rx + N) % N, wy = (ry + N) % N;
    bool alive = grid_get(&g, wx, wy);
    demo_count += alive;
    viz_log(TextFormat("(%d %% %d, %d %% %d) = (%d, %d)", rx, N, ry, N, cx, cy),
            (cx != wx || cy != wy) ? TextFormat("отрицательно! нужна клетка (%d, %d)", wx, wy)
                                   : TextFormat("%s, count = %d", alive ? "живая" : "мёртвая", demo_count));
    demo_k++;
}

static void update(float dt)
{
    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        return;
    }
    int x, y;
    bool over = grid_cell_at(&g, field(), GetMousePosition(), &x, &y);
    if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        grid_set(&g, x, y, !grid_get(&g, x, y));
        viz_log(TextFormat("клетка (%d, %d) = %d;", x, y, grid_get(&g, x, y)), NULL);
    }
    if (over && IsKeyPressed(KEY_D)) {
        demo_start(&demo);
        demo_x = x;
        demo_y = y;
        demo_k = 0;
        demo_count = 0;
        message = "";
        viz_log(TextFormat("count_neighbors(g, %d, %d);", x, y), "считаем вручную, по одному соседу");
    }
    if (IsKeyPressed(KEY_C)) grid_clear(&g);
}

static void draw(void)
{
    viz_title("Соседи", "У каждой клетки 8 соседей. Поле замкнуто: вылез за левый край — попал на правый.");

    grid_draw(&g, field(), COL_ACCENT);
    bool ok = student_ok();
    int wrong = 0;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            Rectangle r = grid_cell_rect(&g, field(), x, y);
            int ref = ref_neighbors(&g, x, y);
            int stu = ok ? count_neighbors(&g, x, y) : -1;
            if (ok && stu != ref) { wrong++; viz_frame(r, 2, COL_BAD); }
            if (ok) viz_code(TextFormat("%d", stu), r.x + 4, r.y + 2, 15, stu != ref ? COL_BAD : viz_tint(COL_TEXT, 120));
        }

    // подсветка соседей клетки под мышкой (или клетки из демо)
    int hx, hy;
    bool have = demo.active ? (hx = demo_x, hy = demo_y, true) : grid_cell_at(&g, field(), GetMousePosition(), &hx, &hy);
    if (have) {
        viz_frame(grid_cell_rect(&g, field(), hx, hy), 3, COL_HILITE);
        for (int k = 0; k < 8; k++) {
            if (demo.active && k >= demo_k) break;
            int rx = hx + DX[k], ry = hy + DY[k];
            bool wrapped = rx < 0 || ry < 0 || rx >= N || ry >= N;
            Rectangle r = grid_cell_rect(&g, field(), (rx + N) % N, (ry + N) % N);
            viz_frame((Rectangle){ r.x + 3, r.y + 3, r.width - 6, r.height - 6 }, 2, wrapped ? COL_PTR : COL_GOOD);
        }
    }

    if (demo.active) {
        viz_code_panel(PANEL_X, FIELD_Y, PANEL_W, loop_code, 6, demo_k < 8 ? 2 : 5);
        viz_code(TextFormat("count = %d", demo_count), PANEL_X, FIELD_Y + 200, 24, COL_HILITE);
        viz_text("Оранжевые рамки — соседи «через край».", PANEL_X, FIELD_Y + 240, 17, COL_PTR);
        demo_hint(&demo);
    } else {
        viz_code_panel(PANEL_X, FIELD_Y, PANEL_W, loop_code, 6, -1);
        float ty = FIELD_Y + 200;
        if (!ok) viz_text("Сначала почини задание 1: без него соседи ищутся не там.", PANEL_X, ty, 18, COL_BAD);
        else if (wrong) viz_text(TextFormat("Неверно у %d клеток. Посмотри, где они: у каких краёв?", wrong), PANEL_X, ty, 18, COL_BAD);
        else viz_text("Все числа совпали с правильными!", PANEL_X, ty, 18, COL_GOOD);
        viz_text("Попробуй в уме: чему в C равно (-1) % 12 ?", PANEL_X, ty + 34, 18, COL_TEXT);
        viz_text("Зелёные рамки — соседи, оранжевые — соседи через край.", PANEL_X, ty + 62, 17, COL_DIM);
        viz_hint("наведи на клетку — её соседи     D на клетке — посчитать соседей по шагам     ЛКМ — клетка     C — очистить");
    }
    tasks_draw(PANEL_X, FIELD_Y + 440);
    viz_message(message, message_color);
}

const Scene scene_neighbors = { "Соседи", init, update, draw, unload };
