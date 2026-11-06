// Сцена 2. Массивы: ячейки подряд, индекс = сдвиг указателя, выход за границы.
#include "scenes.h"
#include "student.h"
#include "viz.h"

#include <stdio.h>

#define N 8

static int   heights[N];
static float shown_h[N];   // высота столбика на экране, плавно догоняет heights
static float pulse[N];
static int   idx;          // куда смотрит cur: cur = heights + idx
static int*  max_ptr;
static const char* message;
static char message_buf[160];   // TextFormat нельзя хранить: его буфер перезапишут следующие вызовы
static Color message_color;

// пошаговое демо find_max
static Demo demo;
static int  demo_i;
static int  demo_best;

#define CELL_W  100.0f
#define CELL_X0 150.0f
#define BASE_Y  470.0f
#define BAR_K   18.0f

static const char* demo_code[] = {
    "int* best = &arr[0];",
    "for (int i = 1; i < n; i++)",
    "    if (arr[i] > *best)",
    "        best = &arr[i];",
    "return best;",
};

static void init(void)
{
    int start[N] = { 3, 7, 2, 9, 5, 1, 8, 4 };
    for (int i = 0; i < N; i++) {
        heights[i] = start[i];
        shown_h[i] = (float)start[i];
        pulse[i] = 0;
    }
    idx = 0;
    max_ptr = NULL;
    message = "";
    demo_stop(&demo);
}

static void demo_step(void)
{
    switch (demo.line) {
    case 0:
        viz_log("int* best = &arr[0];", TextFormat("*best = %d", heights[0]));
        demo_i = 1;
        demo.line = 1;
        break;
    case 1:
        viz_log(TextFormat("i < n  →  %d < %d", demo_i, N), demo_i < N ? "да, заходим в цикл" : "нет, цикл закончен");
        demo.line = demo_i < N ? 2 : 4;
        break;
    case 2:
        viz_log(TextFormat("arr[%d] > *best  →  %d > %d", demo_i, heights[demo_i], heights[demo_best]),
                heights[demo_i] > heights[demo_best] ? "да" : "нет");
        if (heights[demo_i] > heights[demo_best]) {
            demo_best = demo_i;   // best = &arr[i]; — стрелка best переезжает
            pulse[demo_best] = 1;
            demo.line = 3;
        } else {
            demo_i++;
            demo.line = 1;
        }
        break;
    case 3:
        viz_log(TextFormat("best = &arr[%d];", demo_best), TextFormat("*best = %d", heights[demo_best]));
        demo_i++;
        demo.line = 1;
        break;
    default:
        viz_log("return best;", TextFormat("адрес arr[%d] = %s", demo_best, viz_addr(&heights[demo_best])));
        max_ptr = &heights[demo_best];
        demo_stop(&demo);
        snprintf(message_buf, sizeof message_buf, "Демо вернуло адрес [%d]. Теперь напиши find_max сама (задание 2) и нажми M.", demo_best);
        message = message_buf;
        message_color = COL_GOOD;
        break;
    }
}

static void update(float dt)
{
    for (int i = 0; i < N; i++) {
        shown_h[i] = viz_approach(shown_h[i], (float)heights[i], 10, dt);
        pulse[i] = viz_approach(pulse[i], 0, 4, dt);
    }

    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        return;
    }

    if ((IsKeyPressed(KEY_RIGHT) && idx < N) || (IsKeyPressed(KEY_LEFT) && idx > -1)) {
        bool right = IsKeyPressed(KEY_RIGHT);
        idx += right ? 1 : -1;
        viz_log(right ? "cur++;" : "cur--;",
                idx >= 0 && idx < N ? TextFormat("cur = heights + %d, *cur = %d", idx, heights[idx])
                                    : "cur за массивом! *cur нельзя");
    }

    bool inside = idx >= 0 && idx < N;
    if (inside && IsKeyPressed(KEY_UP)   && heights[idx] < 12) { heights[idx]++; max_ptr = NULL; pulse[idx] = 1;
                                                             viz_log("*cur += 1;", TextFormat("heights[%d] = %d", idx, heights[idx])); }
    if (inside && IsKeyPressed(KEY_DOWN) && heights[idx] > 0)  { heights[idx]--; max_ptr = NULL; pulse[idx] = 1;
                                                             viz_log("*cur -= 1;", TextFormat("heights[%d] = %d", idx, heights[idx])); }

    if (IsKeyPressed(KEY_M)) {
        max_ptr = find_max(heights, N);
        viz_log("int* max = find_max(heights, 8);",
                !max_ptr ? "max = NULL" : (max_ptr >= heights && max_ptr < heights + N)
                    ? TextFormat("max = &heights[%d], *max = %d", (int)(max_ptr - heights), *max_ptr)
                    : "адрес за массивом!");
        if (!max_ptr) {
            message = "find_max вернула NULL — она пока пустая. Посмотри демо (D), потом student.c, задание 2.";
            message_color = COL_BAD;
        } else if (max_ptr < heights || max_ptr >= heights + N) {
            message = "find_max вернула адрес ЗА пределами массива!";
            message_color = COL_BAD;
            max_ptr = NULL;
        } else {
            pulse[max_ptr - heights] = 1;
            snprintf(message_buf, sizeof message_buf, "find_max вернула адрес элемента [%d], там лежит %d.",
                     (int)(max_ptr - heights), *max_ptr);
            message = message_buf;
            message_color = COL_GOOD;
        }
    }
    if (IsKeyPressed(KEY_D)) {
        demo_start(&demo);
        demo_best = 0;
        demo_i = 0;
        max_ptr = NULL;
        message = "";
    }
    if (IsKeyPressed(KEY_R)) { init(); viz_log("// сброс массива", NULL); }
}

static float cell_x(int i) { return CELL_X0 + i * CELL_W; }

// стрелка снизу под ячейкой i; key — чтобы стрелка плавно ездила
static void pointer_below(const void* key, int i, float y0, const char* label, Color c)
{
    Vector2 x = viz_smooth(key, 0, (Vector2){ cell_x(i) + CELL_W / 2, 0 });
    viz_arrow((Vector2){ x.x, y0 }, (Vector2){ x.x, BASE_Y + 110 }, c);
    viz_code(label, x.x + 10, y0 - 22, 20, c);
}

static void draw(void)
{
    viz_title("Массивы",
              "Массив — ячейки, которые лежат в памяти ПОДРЯД. Адрес следующей = адрес предыдущей + sizeof(int).");

    // «чужая» память по краям
    for (int i = -1; i <= N; i += N + 1) {
        Rectangle r = { cell_x(i) + 6, BASE_Y, CELL_W - 12, 56 };
        viz_frame(r, 2.0f, (Color){ 90, 60, 70, 255 });
        viz_code("???", r.x + 26, r.y + 16, 22, (Color){ 140, 90, 100, 255 });
        viz_text("чужая память", r.x - 4, r.y + 62, 16, (Color){ 140, 90, 100, 255 });
    }

    int best_shown = demo.active ? demo_best : (max_ptr ? (int)(max_ptr - heights) : -1);
    int cmp_i = demo.active && demo.line == 2 ? demo_i : -1;

    for (int i = 0; i < N; i++) {
        float x = cell_x(i) + 6;
        float bar_h = shown_h[i] * BAR_K;
        Color bar = (i == best_shown) ? COL_HILITE : (i == cmp_i ? g_theme.bar_cmp : g_theme.bar);
        Rectangle br = { x + 14, BASE_Y - 10 - bar_h, CELL_W - 40, bar_h };
        viz_fill(br, bar);
        viz_glow(br, pulse[i], COL_HILITE);

        bool selected = !demo.active && i == idx;
        viz_cell(x, BASE_Y, CELL_W - 12, 56, NULL, TextFormat("%d", heights[i]), &heights[i],
                 selected ? g_theme.ptr_tile : COL_CELL);
        viz_code(TextFormat("[%d]", i), x + 28, BASE_Y + 84, 18, COL_DIM);
    }

    if (demo.active) {
        if (demo.line >= 1 && demo.line <= 3 && demo_i < N)
            pointer_below(&demo_i, demo_i, 630, TextFormat("i = %d", demo_i), (Color){ 140, 220, 255, 255 });
        Vector2 bx = viz_smooth(&demo_best, 0, (Vector2){ cell_x(demo_best) + CELL_W / 2, 0 });
        float top = BASE_Y - 10 - shown_h[demo_best] * BAR_K;
        viz_code("best", bx.x - 22, top - 30, 20, COL_HILITE);

        viz_code_panel(690, 150, 480, demo_code, 5, demo.line);
        if (demo.line == 2)
            viz_code(TextFormat("arr[%d] = %d  >  *best = %d ?  %s", demo_i, heights[demo_i], heights[demo_best],
                                heights[demo_i] > heights[demo_best] ? "ДА" : "нет"),
                     30, 150, 22, COL_TEXT);
        demo_hint(&demo);
    } else {
        pointer_below(&idx, idx, 630, "cur", COL_PTR);

        if (max_ptr) {
            int mi = (int)(max_ptr - heights);
            viz_code("max", cell_x(mi) + CELL_W / 2 - 18, BASE_Y - 10 - shown_h[mi] * BAR_K - 28, 20, COL_HILITE);
        }

        float tx = 690, ty = 150;
        viz_code(TextFormat("sizeof(heights) = %d байта", (int)sizeof heights), tx, ty, 20, COL_TEXT);
        viz_code(TextFormat("sizeof(int)     = %d", (int)sizeof(int)), tx, ty + 26, 20, COL_TEXT);
        viz_code(TextFormat("элементов: %d / %d = %d", (int)sizeof heights, (int)sizeof(int),
                            (int)(sizeof heights / sizeof heights[0])), tx, ty + 52, 20, COL_TEXT);

        float lx = 30, ly = 150;
        viz_code(TextFormat("cur = heights + %d", idx), lx, ly, 22, COL_PTR);
        if (idx >= 0 && idx < N) {
            viz_code(TextFormat("*cur = %d", heights[idx]), lx, ly + 30, 22, COL_TEXT);
            viz_code(TextFormat("heights[%d] == *(heights + %d) == %d", idx, idx, heights[idx]), lx, ly + 60, 20, COL_DIM);
        } else {
            viz_text("cur смотрит ЗА массив. *cur читать нельзя: там чужие данные.", lx, ly + 30, 20, COL_BAD);
            viz_text("Это и был секретный баг №7 в аквариуме (i <= n).", lx, ly + 58, 18, COL_DIM);
        }
        viz_hint("\xE2\x86\x90/\xE2\x86\x92 — двигать cur   \xE2\x86\x91/\xE2\x86\x93 — изменить *cur   M — твоя find_max   D — демо по шагам   R — сброс");
    }
    viz_message(message, message_color);
}

static void unload(void) {}

const Scene scene_arrays = { "Массивы", init, update, draw, unload };
