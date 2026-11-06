// Сцена 5. Галерея: большое поле, знаменитые узоры, просто смотреть и играть.
#include "engine.h"
#include "scenes.h"

#define GW 120
#define GH 80

static Grid  g;
static int   selected;
static bool  running;
static float timer;
static float speed = 12;   // поколений в секунду
static int   generation;

static Rectangle field(void) { return (Rectangle){ FIELD_X, FIELD_Y, FIELD_W, FIELD_H }; }

static bool use_student(void)
{
    TaskStatus s = tasks_check();
    return s.index && s.neighbors && s.step;
}

static void init(void)
{
    grid_init(&g, GW, GH);
    pattern_stamp(&g, &PATTERNS[4], 4, 4);   // ружьё Госпера
    selected = 0;
    running = true;
    generation = 0;
}

static void unload(void) { grid_free(&g); }

static int population(void)
{
    int n = 0;
    for (int i = 0; i < GW * GH; i++) n += g.cur[i];
    return n;
}

static void update(float dt)
{
    if (IsKeyPressed(KEY_SPACE)) running = !running;
    timer += dt;
    if (running && timer > 1.0f / speed) {
        timer = 0;
        if (use_student()) life_step(&g);   // её код — когда задания 1–3 починены
        else ref_step(&g);
        generation++;
    }
    int x, y;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && grid_cell_at(&g, field(), GetMousePosition(), &x, &y)) {
        pattern_stamp(&g, &PATTERNS[selected], x, y);
        viz_log(TextFormat("pattern_stamp(&g, \"%s\", %d, %d);", PATTERNS[selected].name, x, y), PATTERNS[selected].note);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && grid_cell_at(&g, field(), GetMousePosition(), &x, &y))
        grid_set(&g, x, y, true);   // ПКМ — рисовать клетки
    if (IsKeyPressed(KEY_R)) { grid_random(&g, 20); generation = 0; }
    if (IsKeyPressed(KEY_C)) { grid_clear(&g); generation = 0; }
}

static void draw_preview(void)
{
    int x, y;
    if (!grid_cell_at(&g, field(), GetMousePosition(), &x, &y)) return;
    const Pattern* p = &PATTERNS[selected];
    for (int r = 0; r < p->h; r++) {
        const char* row = r < 12 ? p->rows[r] : p->rows[0];
        for (int c = 0; c < p->w && row[c]; c++)
            if (row[c] == 'O') {
                Rectangle cell = grid_cell_rect(&g, field(), (x + c) % GW, (y + r) % GH);
                DrawRectangleRec(cell, viz_tint(COL_HILITE, 120));
            }
    }
}

static void draw(void)
{
    viz_title("Галерея", "Знаменитые узоры «Жизни». Здесь ничего не сломано — просто смотри и играй.");

    grid_draw(&g, field(), COL_ACCENT);
    draw_preview();
    viz_code(TextFormat("поколение %d    живых: %d", generation, population()), FIELD_X, FIELD_Y + FIELD_H - 22, 17, COL_DIM);

    // карточки узоров
    viz_text("Узор для клика (кликни карточку):", PANEL_X, FIELD_Y, 17, COL_DIM);
    for (int i = 0; i < PATTERN_COUNT; i++) {
        Rectangle r = { PANEL_X, FIELD_Y + 28 + i * 52, PANEL_W, 46 };
        bool hover = CheckCollisionPointRec(GetMousePosition(), r);
        viz_fill(r, i == selected ? viz_tint(COL_ACCENT, g_theme.hl_alpha + 40) : (hover ? COL_CELL : COL_PANEL));
        viz_text(PATTERNS[i].name, r.x + 14, r.y + 4, 19, i == selected ? COL_ACCENT : COL_TEXT);
        viz_text(PATTERNS[i].note, r.x + 14, r.y + 25, 15, COL_DIM);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selected = i;
    }

    float sy = FIELD_Y + 300;
    viz_text("Скорость, поколений в секунду", PANEL_X, sy, 17, COL_DIM);
    viz_drag_value((Rectangle){ PANEL_X, sy + 24, 300, 30 }, &speed, 1, 60, 0.2f, 2);
    viz_code(TextFormat("%d", (int)(speed + 0.5f)), PANEL_X + 10, sy + 28, 20, COL_HILITE);

    viz_text(use_student() ? "Поле считает ТВОЯ life_step!" : "Пока работает эталонный код — почини задания 1–3, и здесь заработает твой.",
             PANEL_X, sy + 72, 17, use_student() ? COL_GOOD : COL_DIM);
    tasks_draw(PANEL_X, FIELD_Y + 440);

    viz_hint("ЛКМ — поставить узор     ПКМ — рисовать     ПРОБЕЛ — пауза     R — случайно     C — очистить");
}

const Scene scene_gallery = { "Галерея", init, update, draw, unload };
