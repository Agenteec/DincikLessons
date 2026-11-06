// Сцена 4. Новый размер поля: выделить новую память, перенести рисунок и не забыть старую.
#include "engine.h"
#include "scenes.h"

#include <stdio.h>

static Grid  g;
static float want_w = 24, want_h = 18;   // ползунки
static bool  running;
static float timer;
static int   resizes;
static int   leaked_blocks;              // сколько блоков прибавилось за все изменения размера
static const char* message;
static char  message_buf[220];
static Color message_color;

static Rectangle field(void) { return (Rectangle){ FIELD_X, FIELD_Y, FIELD_W, FIELD_H }; }

static void init(void)
{
    grid_init(&g, 24, 18);
    want_w = 24;
    want_h = 18;
    pattern_stamp(&g, &PATTERNS[0], 1, 1);
    pattern_stamp(&g, &PATTERNS[1], 8, 8);
    running = false;
    resizes = 0;
    leaked_blocks = 0;
    message = "Тяни ползунки справа: поле поменяет размер через твою grid_resize. Следи за «Кучей».";
    message_color = COL_DIM;
}

static void unload(void) { grid_free(&g); }

static void apply_resize(void)
{
    int w = (int)(want_w + 0.5f), h = (int)(want_h + 0.5f);
    if (w == g.w && h == g.h) return;
    long long seq = mt_seq();
    int before = mt_live_blocks();
    grid_resize(&g, w, h);   // задание 4
    int delta = mt_live_blocks() - before;
    resizes++;
    leaked_blocks += delta;
    viz_log(TextFormat("grid_resize(&g, %d, %d);", w, h), delta == 0 ? "блоков столько же — старые освобождены"
                                                                       : TextFormat("в куче +%d блока!", delta));
    mem_log_since(seq);
    if (delta > 0) {
        snprintf(message_buf, sizeof message_buf,
                 "После изменения размера в куче стало на %d блока больше. Кто освободит старые cur и next?", delta);
        message_color = COL_BAD;
    } else {
        snprintf(message_buf, sizeof message_buf, "Размер %d × %d. Старые буферы освобождены — утечки нет.", w, h);
        message_color = COL_GOOD;
    }
    message = message_buf;
}

static void update(float dt)
{
    if (IsKeyPressed(KEY_SPACE)) {
        if (student_step_safe(&g)) running = !running;
        else {
            message = "Поле не квадратное, а задание 1 ещё не починено: life_step вылезла бы за край массива.";
            message_color = COL_BAD;
        }
    }
    if (running && !student_step_safe(&g)) running = false;
    timer += dt;
    if (running && timer > 0.12f) {
        timer = 0;
        if (tasks_check().step) life_step(&g);
        else ref_step(&g);   // пока задание 3 не починено, шагаем эталоном — здесь смотрим на память
    }
    int x, y;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && grid_cell_at(&g, field(), GetMousePosition(), &x, &y))
        grid_set(&g, x, y, !grid_get(&g, x, y));
    if (IsKeyPressed(KEY_R)) { grid_random(&g, 25); viz_log("// случайное поле", NULL); }
    if (IsKeyPressed(KEY_C)) grid_clear(&g);
}

static void draw(void)
{
    viz_title("Размер поля", "Новый размер — новая память. Старую надо вернуть, иначе она потеряется навсегда.");

    grid_draw(&g, field(), COL_ACCENT);

    float y = FIELD_Y;
    viz_text("Ширина и высота (тяни мышью или крути колесо)", PANEL_X, y, 17, COL_DIM);
    struct { const char* name; float* v; float max; } sliders[2] = { { "w", &want_w, 60 }, { "h", &want_h, 40 } };
    for (int i = 0; i < 2; i++) {
        float sy = y + 30 + i * 40;
        viz_code(sliders[i].name, PANEL_X, sy + 4, 20, COL_TEXT);
        Rectangle r = { PANEL_X + 30, sy, 300, 30 };
        int st = viz_drag_value(r, sliders[i].v, 4, sliders[i].max, 0.15f, 1);
        viz_code(TextFormat("%d", (int)(*sliders[i].v + 0.5f)), r.x + 10, sy + 4, 20, COL_HILITE);
        if (st == VIZ_DRAG_DONE) apply_resize();
    }
    viz_code(TextFormat("сейчас: %d × %d = %d клеток", g.w, g.h, g.w * g.h), PANEL_X, y + 120, 18, COL_TEXT);
    viz_code(TextFormat("cur  = %s", viz_addr(g.cur)), PANEL_X, y + 150, 18, COL_ADDR);
    viz_code(TextFormat("next = %s", viz_addr(g.next)), PANEL_X, y + 176, 18, COL_ADDR);

    viz_code(TextFormat("изменений размера: %d", resizes), PANEL_X, y + 220, 18, COL_DIM);
    viz_code(TextFormat("лишних блоков в куче: %d", leaked_blocks), PANEL_X, y + 246, 18, leaked_blocks > 0 ? COL_BAD : COL_GOOD);
    if (!tasks_check().step) viz_text("Задание 3 ещё не починено — поле шагает эталонным кодом.", PANEL_X, y + 280, 16, COL_DIM);

    mem_panel(PANEL_X, y + 330, PANEL_W);
    tasks_draw(PANEL_X, y + 440);

    viz_message(message, message_color);
    viz_hint("ползунки — новый размер     ПРОБЕЛ — пуск/пауза     ЛКМ — клетка     R — случайно     C — очистить");
}

const Scene scene_resize = { "Размер", init, update, draw, unload };
