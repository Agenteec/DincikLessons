// engine.c — эталонная «Жизнь» и проверка заданий. Здесь всё сделано правильно.
#include "engine.h"

#include <string.h>

void grid_init(Grid* g, int w, int h)
{
    g->w = w;
    g->h = h;
    g->cur = mt_calloc((size_t)w * h, 1);
    g->next = mt_calloc((size_t)w * h, 1);
}

void grid_free(Grid* g)
{
    mt_free(g->cur);
    mt_free(g->next);
    g->cur = g->next = NULL;
}

void grid_clear(Grid* g) { memset(g->cur, 0, (size_t)g->w * g->h); }

void grid_random(Grid* g, int percent)
{
    for (int i = 0; i < g->w * g->h; i++) g->cur[i] = GetRandomValue(0, 99) < percent;
}

static int wrap(int v, int n) { return ((v % n) + n) % n; }

bool grid_get(const Grid* g, int x, int y) { return g->cur[wrap(y, g->h) * g->w + wrap(x, g->w)] != 0; }
void grid_set(Grid* g, int x, int y, bool alive) { g->cur[wrap(y, g->h) * g->w + wrap(x, g->w)] = alive; }

int ref_neighbors(const Grid* g, int x, int y)
{
    int n = 0;
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (dx || dy) n += grid_get(g, x + dx, y + dy);
    return n;
}

void ref_step(Grid* g)
{
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            int n = ref_neighbors(g, x, y);
            bool alive = grid_get(g, x, y);
            g->next[y * g->w + x] = (alive && (n == 2 || n == 3)) || (!alive && n == 3);
        }
    unsigned char* t = g->cur;
    g->cur = g->next;
    g->next = t;
}

// ---------- рисование ----------

static float cell_size(const Grid* g, Rectangle area)
{
    float cw = area.width / g->w, ch = area.height / g->h;
    return cw < ch ? cw : ch;
}

static Vector2 grid_origin(const Grid* g, Rectangle area, float c)
{
    return (Vector2){ area.x + (area.width - c * g->w) / 2, area.y + (area.height - c * g->h) / 2 };
}

Rectangle grid_cell_rect(const Grid* g, Rectangle area, int x, int y)
{
    float c = cell_size(g, area);
    Vector2 o = grid_origin(g, area, c);
    return (Rectangle){ o.x + x * c, o.y + y * c, c, c };
}

bool grid_cell_at(const Grid* g, Rectangle area, Vector2 m, int* x, int* y)
{
    float c = cell_size(g, area);
    Vector2 o = grid_origin(g, area, c);
    int cx = (int)((m.x - o.x) / c), cy = (int)((m.y - o.y) / c);
    if (m.x < o.x || m.y < o.y || cx >= g->w || cy >= g->h) return false;
    *x = cx;
    *y = cy;
    return true;
}

float grid_draw(const Grid* g, Rectangle area, Color alive)
{
    float c = cell_size(g, area);
    Vector2 o = grid_origin(g, area, c);
    float gap = c >= 10 ? 2.0f : (c >= 5 ? 1.0f : 0.0f);
    DrawRectangle((int)o.x, (int)o.y, (int)(c * g->w), (int)(c * g->h), viz_tint(COL_PANEL, 160));
    for (int y = 0; y < g->h; y++)
        for (int x = 0; x < g->w; x++) {
            Rectangle r = { o.x + x * c + gap / 2, o.y + y * c + gap / 2, c - gap, c - gap };
            bool on = g->cur[y * g->w + x] != 0;
            // клетки — маленькие плитки: в Material чуть скруглённые, в Metro — квадратные
            if (c >= 10 && g_theme.radius > 0) DrawRectangleRounded(r, 0.25f, 4, on ? alive : COL_CELL);
            else DrawRectangleRec(r, on ? alive : (c >= 10 ? COL_CELL : viz_tint(COL_CELL, 120)));
        }
    return c;
}

// ---------- узоры ----------

const Pattern PATTERNS[] = {
    { "Планер", "летит по диагонали", 3, 3, { ".O.", "..O", "OOO" } },
    { "Лёгкий корабль", "летит по прямой", 5, 4, { ".O..O", "O....", "O...O", "OOOO." } },
    { "Пульсар", "мигает с периодом 3", 13, 13,
      { "..OOO...OOO..", ".............", "O....O.O....O", "O....O.O....O", "O....O.O....O", "..OOO...OOO..",
        ".............", "..OOO...OOO..", "O....O.O....O", "O....O.O....O", "O....O.O....O", "............." } },
    { "R-пентамино", "5 клеток, живёт 1103 поколения", 3, 3, { ".OO", "OO.", ".O." } },
    { "Ружьё Госпера", "стреляет планерами", 36, 9,
      { "........................O...........", "......................O.O...........",
        "............OO......OO............OO", "...........O...O....OO............OO",
        "OO........O.....O...OO..............", "OO........O...O.OO....O.O...........",
        "..........O.....O.......O...........", "...........O...O....................",
        "............OO......................" } },
};
const int PATTERN_COUNT = (int)(sizeof PATTERNS / sizeof PATTERNS[0]);

void pattern_stamp(Grid* g, const Pattern* p, int x, int y)
{
    // у пульсара 13 рядов, а в массиве 12 — последний ряд такой же, как первый
    for (int r = 0; r < p->h; r++) {
        const char* row = r < 12 ? p->rows[r] : p->rows[0];
        for (int c = 0; c < p->w && row[c]; c++)
            if (row[c] == 'O') grid_set(g, x + c, y + r, true);
    }
}

// ---------- проверка заданий ----------

static TaskStatus g_status;
static bool g_checked;

static bool check_index(void)
{
    Grid g = { 7, 5, NULL, NULL };   // буферы не нужны: функция только считает число
    for (int y = 0; y < g.h; y++)
        for (int x = 0; x < g.w; x++)
            if (cell_index(&g, x, y) != y * g.w + x) return false;
    return true;
}

static void test_pattern(Grid* g)
{
    // несимметричный узор, касающийся всех краёв, — чтобы ошибки было видно
    grid_clear(g);
    int cells[][2] = { { 0, 0 }, { 1, 0 }, { 5, 1 }, { 0, 3 }, { 5, 5 }, { 2, 2 }, { 3, 2 }, { 3, 3 }, { 4, 5 } };
    for (int i = 0; i < 9; i++) grid_set(g, cells[i][0], cells[i][1], true);
}

static bool check_neighbors(void)
{
    if (!check_index()) return false;   // без задания 1 соседи посчитаются не там
    Grid g;
    grid_init(&g, 6, 6);
    test_pattern(&g);
    bool ok = true;
    for (int y = 0; y < g.h && ok; y++)
        for (int x = 0; x < g.w && ok; x++)
            ok = count_neighbors(&g, x, y) == ref_neighbors(&g, x, y);
    grid_free(&g);
    return ok;
}

static bool check_step(void)
{
    if (!check_neighbors()) return false;
    Grid a, b;
    grid_init(&a, 6, 6);
    grid_init(&b, 6, 6);
    test_pattern(&a);
    test_pattern(&b);
    bool ok = true;
    for (int i = 0; i < 4 && ok; i++) {
        life_step(&a);
        ref_step(&b);
        ok = memcmp(a.cur, b.cur, 36) == 0;
    }
    grid_free(&a);
    grid_free(&b);
    return ok;
}

static bool check_resize(void)
{
    Grid g;
    grid_init(&g, 5, 5);
    grid_set(&g, 1, 1, true);
    int before = mt_live_blocks();
    grid_resize(&g, 7, 6);
    int after = mt_live_blocks();
    bool ok = after == before && g.w == 7 && g.h == 6 && g.cur[1 * 7 + 1] == 1;
    grid_free(&g);
    return ok;
}

TaskStatus tasks_check(void)
{
    if (!g_checked) {
        g_status = (TaskStatus){ check_index(), check_neighbors(), check_step(), check_resize() };
        g_checked = true;
    }
    return g_status;
}

void tasks_draw(float x, float y)
{
    TaskStatus s = tasks_check();
    bool ok[4] = { s.index, s.neighbors, s.step, s.resize };
    viz_text("Задания:", x, y + 4, 17, COL_DIM);
    for (int i = 0; i < 4; i++) {
        Rectangle r = { x + 80 + i * 40, y, 32, 30 };
        viz_fill(r, ok[i] ? viz_tint(COL_GOOD, 200) : viz_tint(COL_BAD, 200));
        viz_code(TextFormat("%d", i + 1), r.x + 10, r.y + 5, 19, COL_BG);
    }
}

bool student_step_safe(const Grid* g)
{
    // с неисправленным заданием 1 на неквадратном поле её функция вылезет за край массива
    return tasks_check().index || g->w == g->h;
}
