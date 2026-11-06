// Сцена 3. Одно поколение: читаем из cur, пишем в next, потом меняем указатели местами.
#include "engine.h"
#include "scenes.h"

#include <stdio.h>
#include <string.h>

#define N 16

static Grid g;
static Grid demo_g;           // копия для демо — настоящее поле в демо не трогаем
static bool running;
static float timer;
static int  generation;
static int  wrong_cells = -1; // сколько клеток не совпало с правильным шагом в последний раз
static const char* message;
static char  message_buf[200];
static Color message_color;

static Demo demo;
static int  demo_row;
static bool demo_swapped;

static const char* step_code[] = {
    "for (каждая клетка) {",
    "    n = count_neighbors(g, x, y);   // из cur",
    "    g->next[i] = правило(cur[i], n);",
    "}",
    "tmp = g->cur;",
    "g->cur = g->next;",
    "g->next = tmp;",
};

static Rectangle field(void) { return (Rectangle){ FIELD_X, FIELD_Y, 480, FIELD_H }; }

static void reset(void)
{
    grid_clear(&g);
    memset(g.next, 0, N * N);
    pattern_stamp(&g, &PATTERNS[0], 2, 2);     // планер
    for (int x = 10; x <= 12; x++) grid_set(&g, x, 11, true);   // мигалка
    generation = 0;
    wrong_cells = -1;
}

static void init(void)
{
    grid_init(&g, N, N);
    grid_init(&demo_g, N, N);
    reset();
    running = false;
    demo_stop(&demo);
    message = "S — один шаг твоей life_step, ПРОБЕЛ — запустить. Планер должен лететь вниз-вправо.";
    message_color = COL_DIM;
}

static void unload(void)
{
    grid_free(&g);
    grid_free(&demo_g);
}

static void student_step(void)
{
    // рядом считаем правильный шаг, чтобы сравнить
    Grid ref;
    grid_init(&ref, N, N);
    memcpy(ref.cur, g.cur, N * N);
    ref_step(&ref);

    life_step(&g);   // задание 3
    generation++;

    wrong_cells = 0;
    for (int i = 0; i < N * N; i++) wrong_cells += (g.cur[i] != 0) != (ref.cur[i] != 0);
    grid_free(&ref);
}

static void demo_step(void)
{
    if (!demo_swapped && demo_row < N) {
        // считаем один ряд: читаем из cur, пишем в next
        for (int x = 0; x < N; x++) {
            int n = ref_neighbors(&demo_g, x, demo_row);
            bool alive = grid_get(&demo_g, x, demo_row);
            demo_g.next[demo_row * N + x] = (alive && (n == 2 || n == 3)) || (!alive && n == 3);
        }
        viz_log(TextFormat("// ряд %d: читаем cur, пишем next", demo_row), "cur пока не меняется!");
        demo_row++;
        return;
    }
    if (!demo_swapped) {
        unsigned char* t = demo_g.cur;
        demo_g.cur = demo_g.next;
        demo_g.next = t;
        demo_swapped = true;
        viz_log("tmp = g->cur; g->cur = g->next; g->next = tmp;", "поменялись только два адреса");
        return;
    }
    demo_stop(&demo);
    memcpy(g.cur, demo_g.cur, N * N);   // результат демо показываем на поле
    generation++;
    message = "Пока считаем, читаем только из cur. Иначе соседи уже наполовину «из будущего».";
    message_color = COL_GOOD;
}

static void update(float dt)
{
    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        return;
    }
    if (IsKeyPressed(KEY_SPACE)) running = !running;
    timer += dt;
    bool step = IsKeyPressed(KEY_S) || (running && timer > 0.18f);
    if (step) {
        timer = 0;
        student_step();
        if (IsKeyPressed(KEY_S)) viz_log("life_step(&g);", TextFormat("поколение %d", generation));
        if (wrong_cells > 0) {
            snprintf(message_buf, sizeof message_buf, "Поколение %d: %d клеток не совпало с правильным шагом.", generation, wrong_cells);
            message = message_buf;
            message_color = COL_BAD;
        } else {
            message = "Шаг верный!";
            message_color = COL_GOOD;
        }
    }
    int x, y;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && grid_cell_at(&g, field(), GetMousePosition(), &x, &y))
        grid_set(&g, x, y, !grid_get(&g, x, y));
    if (IsKeyPressed(KEY_D)) {
        running = false;
        memcpy(demo_g.cur, g.cur, N * N);
        memset(demo_g.next, 0, N * N);
        demo_row = 0;
        demo_swapped = false;
        demo_start(&demo);
        message = "";
    }
    if (IsKeyPressed(KEY_R)) { reset(); running = false; viz_log("// сначала", NULL); }
}

static void draw_buffer(const unsigned char* buf, float x, float y, const char* label, int hl_row)
{
    const float c = 9;
    viz_code(TextFormat("%s  %s", label, viz_addr(buf)), x, y, 16, COL_ADDR);
    Rectangle box = { x, y + 24, c * N, c * N };
    DrawRectangleRec(box, viz_tint(COL_CELL, 160));
    for (int i = 0; i < N * N; i++)
        if (buf[i]) DrawRectangle((int)(x + (i % N) * c), (int)(y + 24 + (i / N) * c), (int)c - 1, (int)c - 1, COL_TEXT);
    if (hl_row >= 0 && hl_row < N) viz_frame((Rectangle){ x - 1, y + 23 + hl_row * c, c * N + 2, c + 2 }, 1.5f, COL_HILITE);
}

static void draw_buffers(float x, float y)
{
    const Grid* s = demo.active ? &demo_g : &g;
    // буферы рисуем на постоянных местах: левый и правый блок памяти
    const unsigned char* left = s->cur < s->next ? s->cur : s->next;
    const unsigned char* right = s->cur < s->next ? s->next : s->cur;
    float lx = x + 100, rx = x + 330;
    draw_buffer(left, lx, y, "блок А", demo.active && !demo_swapped ? (left == s->next ? demo_row - 1 : -1) : -1);
    draw_buffer(right, rx, y, "блок Б", demo.active && !demo_swapped ? (right == s->next ? demo_row - 1 : -1) : -1);

    // указатели g->cur и g->next — стрелки плавно переезжают при обмене
    Rectangle pc = { x, y + 190, 120, 40 }, pn = { x + 250, y + 190, 120, 40 };
    viz_fill(pc, g_theme.ptr_tile);
    viz_fill(pn, viz_tint(g_theme.ptr_tile, 160));
    viz_code("g->cur", pc.x + 14, pc.y + 9, 19, COL_TEXT);
    viz_code("g->next", pn.x + 10, pn.y + 9, 19, COL_TEXT);
    float bw = 9 * N;
    Vector2 to_cur = { (s->cur == left ? lx : rx) + bw / 2, y + 24 + bw + 4 };
    Vector2 to_next = { (s->next == left ? lx : rx) + bw / 2, y + 24 + bw + 4 };
    static char k1, k2;
    viz_arrow((Vector2){ pc.x + 60, pc.y }, viz_smooth(&k1, 0, to_cur), COL_PTR);
    viz_arrow((Vector2){ pn.x + 60, pn.y }, viz_smooth(&k2, 0, to_next), viz_tint(COL_PTR, 170));
    viz_text("Рисуем то, на что указывает g->cur.", x, y + 240, 16, COL_DIM);
}

static void draw(void)
{
    viz_title("Шаг: два буфера", "Новое поколение считаем в отдельный буфер next, а потом просто меняем указатели местами.");

    grid_draw(demo.active ? &demo_g : &g, field(), COL_ACCENT);
    if (demo.active && !demo_swapped && demo_row < N)
        for (int x = 0; x < N; x++) viz_frame(grid_cell_rect(&g, field(), x, demo_row), 2, COL_HILITE);
    viz_code(TextFormat("поколение %d", generation), FIELD_X + 490, FIELD_Y, 18, COL_DIM);

    int hl = !demo.active ? -1 : (!demo_swapped ? (demo_row < N ? 2 : 4) : 6);
    viz_code_panel(PANEL_X, FIELD_Y, PANEL_W, step_code, 7, hl);
    draw_buffers(PANEL_X, FIELD_Y + 205);
    tasks_draw(PANEL_X, FIELD_Y + 465);

    viz_message(message, message_color);
    if (demo.active) demo_hint(&demo);
    else viz_hint("S — шаг     ПРОБЕЛ — пуск/пауза     D — шаг правильно, по рядам     ЛКМ — клетка     R — сначала");
}

const Scene scene_step = { "Шаг", init, update, draw, unload };
