// Сцена 1. Указатели: адрес (&), разыменование (*), swap.
#include "scenes.h"
#include "student.h"
#include "viz.h"

#include <limits.h>

static int  coins;
static int  gems;
static int* p;
static int  last_line;
static const char* message;
static Color message_color;

static float pulse_coins, pulse_gems;

// пошаговое демо swap
static Demo demo;
static int  tmp_value;       // INT_MIN — tmp ещё не заполнен
static float frame_alpha;    // рамка «внутри swap»

static const Rectangle RC = { 90, 230, 180, 90 };
static const Rectangle RG = { 380, 230, 180, 90 };
static const Rectangle RP = { 235, 460, 180, 70 };
static const Rectangle RA = { 60, 470, 150, 70 };
static const Rectangle RT = { 250, 470, 150, 70 };
static const Rectangle RB = { 440, 470, 150, 70 };

static const char* code[] = {
    "int coins = 10;",
    "int gems  = 3;",
    "int* p = &coins;   // p хранит АДРЕС",
    "p = &gems;         // [TAB]",
    "p = &coins;        // [TAB]",
    "*p += 1;           // [\xE2\x86\x91]",
    "*p -= 1;           // [\xE2\x86\x93]",
    "swap(&coins, &gems);  // [S]",
};

static const char* demo_code[] = {
    "swap(&coins, &gems);",
    "void swap(int* a, int* b) {",
    "    int tmp = *a;",
    "    *a = *b;",
    "    *b = tmp;",
    "}   // a, b, tmp исчезают",
};

static Vector2 center(Rectangle r) { return (Vector2){ r.x + r.width / 2, r.y + r.height / 2 }; }

static void init(void)
{
    coins = 10;
    gems = 3;
    p = &coins;
    last_line = 2;
    message = "";
    demo_stop(&demo);
    frame_alpha = 0;
}

static void demo_step(void)
{
    switch (demo.line) {
    case 0:   // вошли в функцию: появились a, b, tmp
        tmp_value = INT_MIN;
        viz_log("swap(&coins, &gems);", TextFormat("a = %s, b = %s", viz_addr(&coins), viz_addr(&gems)));
        demo.line = 1;
        break;
    case 1:   // int tmp = *a;
        viz_log("int tmp = *a;", TextFormat("tmp = %d (взяли из coins)", coins));
        viz_fly_start(TextFormat("%d", coins), center(RC), center(RT), COL_HILITE, &tmp_value, coins);
        demo.line = 2;
        break;
    case 2:   // *a = *b;
        viz_log("*a = *b;", TextFormat("coins = %d", gems));
        viz_fly_start(TextFormat("%d", gems), center(RG), center(RC), COL_HILITE, &coins, gems);
        pulse_coins = 1;
        demo.line = 3;
        break;
    case 3:   // *b = tmp;
        viz_log("*b = tmp;", TextFormat("gems = %d", tmp_value));
        viz_fly_start(TextFormat("%d", tmp_value), center(RT), center(RG), COL_HILITE, &gems, tmp_value);
        pulse_gems = 1;
        demo.line = 4;
        break;
    case 4:   // выход из функции
        viz_log("}", "a, b, tmp удалены со стека");
        demo.line = 5;
        break;
    default:
        demo_stop(&demo);
        message = "Вот так работает swap. Теперь напиши её в student.c (задание 1) и нажми S.";
        message_color = COL_GOOD;
        break;
    }
}

static void student_swap(void)
{
    int old_coins = coins, old_gems = gems;
    swap(&coins, &gems);
    last_line = 7;

    viz_log("swap(&coins, &gems);", TextFormat("coins = %d, gems = %d", coins, gems));
    if (coins == old_gems && gems == old_coins) {
        // возвращаем старые значения и даём им долететь
        int new_coins = coins, new_gems = gems;
        coins = old_coins;
        gems = old_gems;
        viz_fly_start(TextFormat("%d", new_coins), center(RG), center(RC), COL_GOOD, &coins, new_coins);
        viz_fly_start(TextFormat("%d", new_gems), center(RC), center(RG), COL_GOOD, &gems, new_gems);
        pulse_coins = pulse_gems = 1;
        message = "swap сработала! Функция получила адреса и поменяла сами переменные.";
        message_color = COL_GOOD;
    } else if (coins == old_coins && gems == old_gems) {
        message = "Ничего не поменялось: swap пока пустая. Посмотри демо (D) и открой student.c, задание 1.";
        message_color = COL_BAD;
    } else {
        message = "Что-то поменялось, но не так: coins и gems должны просто обменяться.";
        message_color = COL_BAD;
    }
}

static void update(float dt)
{
    pulse_coins = viz_approach(pulse_coins, 0, 4, dt);
    pulse_gems  = viz_approach(pulse_gems, 0, 4, dt);
    frame_alpha = viz_approach(frame_alpha, demo.active && demo.line >= 1 && demo.line <= 4 ? 1.0f : 0.0f, 8, dt);

    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        return;
    }
    if (viz_fly_busy()) return;

    if (IsKeyPressed(KEY_TAB)) {
        p = (p == &coins) ? &gems : &coins;
        last_line = (p == &gems) ? 3 : 4;
        viz_log(p == &gems ? "p = &gems;" : "p = &coins;", TextFormat("p = %s, *p = %d", viz_addr(p), *p));
        message = "";
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) {
        *p += IsKeyPressed(KEY_UP) ? 1 : -1;
        last_line = IsKeyPressed(KEY_UP) ? 5 : 6;
        if (p == &coins) pulse_coins = 1; else pulse_gems = 1;
        viz_log(IsKeyPressed(KEY_UP) ? "*p += 1;" : "*p -= 1;",
                TextFormat("%s = %d", p == &coins ? "coins" : "gems", *p));
        message = "";
    }
    if (IsKeyPressed(KEY_S)) student_swap();
    if (IsKeyPressed(KEY_D)) { demo_start(&demo); message = ""; }
    if (IsKeyPressed(KEY_R)) { init(); viz_log("// сброс: coins = 10, gems = 3", NULL); }
}

static void arrow_to(const void* key, Rectangle from, Rectangle to, Color c)
{
    Vector2 start = { from.x + from.width / 2, from.y };
    Vector2 end = viz_smooth(key, 0, (Vector2){ to.x + to.width / 2, to.y + to.height + 24 });
    viz_arrow(start, end, c);
}

static void draw_swap_frame(void)
{
    if (frame_alpha < 0.02f) return;
    unsigned char a = (unsigned char)(255 * frame_alpha);

    Rectangle frame = { 40, 420, 570, 150 };
    viz_fill(frame, (Color){ 38, 38, 38, (unsigned char)(200 * frame_alpha) });
    viz_text("внутри swap() — это её собственные переменные", frame.x + 12, frame.y + 6, 17, (Color){ 160, 160, 160, a });

    viz_cell(RA.x, RA.y, RA.width, RA.height, "int* a", viz_addr(&coins), NULL, viz_tint(g_theme.ptr_tile, a));
    viz_cell(RB.x, RB.y, RB.width, RB.height, "int* b", viz_addr(&gems), NULL, viz_tint(g_theme.ptr_tile, a));
    viz_cell(RT.x, RT.y, RT.width, RT.height, "int tmp",
             tmp_value == INT_MIN ? "?" : TextFormat("%d", tmp_value), NULL, (Color){ 60, 60, 60, a });

    Color ptr = COL_PTR;
    ptr.a = a;
    viz_arrow((Vector2){ RA.x + RA.width / 2, RA.y }, (Vector2){ RC.x + RC.width / 2, RC.y + RC.height + 24 }, ptr);
    viz_arrow((Vector2){ RB.x + RB.width / 2, RB.y }, (Vector2){ RG.x + RG.width / 2, RG.y + RG.height + 24 }, ptr);
}

static void draw(void)
{
    viz_title("Указатели",
              "Переменная — коробка в памяти, у коробки есть адрес. Указатель — коробка, в которой лежит адрес.");

    viz_cell(RC.x, RC.y, RC.width, RC.height, "int coins", TextFormat("%d", coins), &coins, COL_CELL);
    viz_cell(RG.x, RG.y, RG.width, RG.height, "int gems", TextFormat("%d", gems), &gems, COL_CELL);
    viz_glow(RC, pulse_coins, COL_HILITE);
    viz_glow(RG, pulse_gems, COL_HILITE);

    if (demo.active) {
        draw_swap_frame();
        viz_code_panel(650, 150, 520, demo_code, sizeof demo_code / sizeof demo_code[0], demo.line);
        viz_text("Следи за числами: они перелетают по адресам.", 650, 330, 20, COL_DIM);
        demo_hint(&demo);
    } else {
        draw_swap_frame();   // догорает после демо
        viz_cell(RP.x, RP.y, RP.width, RP.height, "int* p", viz_addr(p), &p, g_theme.ptr_tile);
        arrow_to(&p, RP, p == &coins ? RC : RG, COL_PTR);

        float ix = 30, iy = 580;
        viz_code(TextFormat("&coins = %s    &gems = %s", viz_addr(&coins), viz_addr(&gems)), ix, iy, 20, COL_ADDR);
        viz_code(TextFormat("p      = %s    (адрес %s)", viz_addr(p), p == &coins ? "coins" : "gems"), ix, iy + 26, 20, COL_PTR);
        viz_code(TextFormat("*p     = %d     (то, что лежит по адресу p)", *p), ix, iy + 52, 20, COL_TEXT);

        viz_code_panel(650, 150, 520, code, sizeof code / sizeof code[0], last_line);
        viz_text("Сначала угадай вслух, что изменится, а потом нажимай.", 650, 390, 20, COL_DIM);
        viz_text("& — «дай адрес»        * — «открой коробку по адресу»", 650, 420, 20, COL_TEXT);
        viz_hint("TAB — перенаправить p   \xE2\x86\x91/\xE2\x86\x93 — изменить *p   S — твоя swap   D — демо swap по шагам   R — сброс");
    }
    viz_message(message, message_color);
}

static void unload(void) {}

const Scene scene_pointers = { "Указатели", init, update, draw, unload };
