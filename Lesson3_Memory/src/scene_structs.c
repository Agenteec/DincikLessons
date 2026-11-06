// Сцена 3. Структуры: поля в памяти, точка и стрелка, передача по указателю и по значению.
#include "scenes.h"
#include "student.h"
#include "viz.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

static Ball red;
static Ball blue;
static bool running;
static const char* message;
static Color message_color;
static bool inspect_blue;   // TAB — какой мячик смотрим в инспекторе

// пошаговое демо «копия против адреса»
static Demo    demo;
static Vector2 ghost_pos, ghost_target;   // копия синего
static float   ghost_alpha, ghost_alpha_target;
static Vector2 red_target;
static bool    red_animating;
static float   ptr_alpha, ptr_alpha_target;

static const Rectangle FIELD = { 30, 150, 620, 460 };
static const Rectangle PTR_BOX = { 60, 540, 130, 50 };

static const char* demo_code[] = {
    "ball_move_copy(blue, dt);",
    "    Ball b = blue;       // КОПИЯ",
    "    b.pos.x += ...;      // двигаем копию",
    "}                        // копия исчезла",
    "ball_move(&red, dt);",
    "    Ball* b = &red;      // АДРЕС",
    "    b->pos.x += ...;     // двигаем оригинал",
    "}                        // red сдвинулся!",
};

// Передаём КОПИЮ мячика: двигается копия, а настоящий blue остаётся на месте.
static void ball_move_copy(Ball b, float dt)
{
    b.pos.x += b.vel.x * dt;
    b.pos.y += b.vel.y * dt;
}

static void bounce(Ball* b)
{
    if ((b->pos.x < FIELD.x + b->radius && b->vel.x < 0) ||
        (b->pos.x > FIELD.x + FIELD.width - b->radius && b->vel.x > 0))
        b->vel.x = -b->vel.x;
    if ((b->pos.y < FIELD.y + b->radius && b->vel.y < 0) ||
        (b->pos.y > FIELD.y + FIELD.height - b->radius && b->vel.y > 0))
        b->vel.y = -b->vel.y;
}

static Vector2 clamp_to_field(Vector2 p, float r)
{
    if (p.x < FIELD.x + r) p.x = FIELD.x + r;
    if (p.x > FIELD.x + FIELD.width - r) p.x = FIELD.x + FIELD.width - r;
    if (p.y < FIELD.y + r) p.y = FIELD.y + r;
    if (p.y > FIELD.y + FIELD.height - r) p.y = FIELD.y + FIELD.height - r;
    return p;
}

static void init(void)
{
    red  = (Ball){ { 200, 300 }, { 180, 120 }, 26, (Color){ 240, 90, 90, 255 } };
    blue = (Ball){ { 450, 450 }, { -150, 160 }, 26, (Color){ 27, 161, 226, 255 } };
    running = false;
    message = "";
    demo_stop(&demo);
    ghost_alpha = ghost_alpha_target = 0;
    ptr_alpha = ptr_alpha_target = 0;
    red_animating = false;
}

static void demo_step(void)
{
    const float shift = 0.6f;   // «dt» для демо — сдвиг заметный глазу

    switch (demo.line) {
    case 0:   // Ball b = blue;
        viz_log("ball_move_copy(blue, dt);", NULL);
        viz_log("Ball b = blue;", TextFormat("копия %d байт по новому адресу", (int)sizeof(Ball)));
        ghost_pos = ghost_target = blue.pos;
        ghost_alpha_target = 1;
        demo.line = 1;
        break;
    case 1:   // b.pos += ...
        viz_log("b.pos.x += b.vel.x * dt;", "двигается копия");
        ghost_target = clamp_to_field((Vector2){ blue.pos.x + blue.vel.x * shift, blue.pos.y + blue.vel.y * shift }, blue.radius);
        demo.line = 2;
        break;
    case 2:   // копия исчезла
        viz_log("}", "копия стёрта, blue не изменился");
        ghost_alpha_target = 0;
        demo.line = 3;
        break;
    case 3:   // ball_move(&red, dt);
        viz_log("ball_move(&red, dt);", TextFormat("передаём адрес %s", viz_addr(&red)));
        demo.line = 4;
        break;
    case 4:   // Ball* b = &red;
        viz_log("Ball* b = &red;", "b — стрелка на настоящий red");
        ptr_alpha_target = 1;
        demo.line = 5;
        break;
    case 5:   // b->pos += ...
        viz_log("b->pos.x += b->vel.x * dt;", "двигается оригинал");
        red_target = clamp_to_field((Vector2){ red.pos.x + red.vel.x * shift, red.pos.y + red.vel.y * shift }, red.radius);
        red_animating = true;
        demo.line = 6;
        break;
    case 6:
        viz_log("}", "b стёрт, но red уже сдвинут");
        ptr_alpha_target = 0;
        demo.line = 7;
        break;
    default:
        demo_stop(&demo);
        message = "Запомни: чтобы функция изменила оригинал, ей нужен АДРЕС. Теперь задание 3.";
        message_color = COL_GOOD;
        break;
    }
}

static void update(float dt)
{
    ghost_alpha = viz_approach(ghost_alpha, ghost_alpha_target, 6, dt);
    ptr_alpha = viz_approach(ptr_alpha, ptr_alpha_target, 6, dt);
    ghost_pos = viz_lerp2(ghost_pos, ghost_target, 1.0f - expf(-5 * dt));
    if (red_animating) {
        red.pos = viz_lerp2(red.pos, red_target, 1.0f - expf(-5 * dt));
        if (fabsf(red.pos.x - red_target.x) < 0.5f && fabsf(red.pos.y - red_target.y) < 0.5f) {
            red.pos = red_target;
            red_animating = false;
        }
    }

    if (demo.active) {
        bool busy = red_animating ||
                    fabsf(ghost_pos.x - ghost_target.x) + fabsf(ghost_pos.y - ghost_target.y) > 1.0f ||
                    fabsf(ghost_alpha - ghost_alpha_target) > 0.05f;
        if (demo_next(&demo, dt, busy)) demo_step();
        return;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        running = !running;
        viz_log(running ? "running = true;" : "running = false;",
                running ? "каждый кадр: ball_move(&red, dt);" : "пауза");
    }
    if (IsKeyPressed(KEY_TAB)) {
        inspect_blue = !inspect_blue;
        viz_log(inspect_blue ? "// смотрим Ball blue" : "// смотрим Ball red", NULL);
    }
    if (IsKeyPressed(KEY_R)) init();
    if (IsKeyPressed(KEY_D)) {
        demo_start(&demo);
        running = false;
        message = "";
        return;
    }

    // клик — «пнуть» красный мячик в сторону мышки
    Vector2 m = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, FIELD)) {
        red.vel = (Vector2){ (m.x - red.pos.x) * 1.5f, (m.y - red.pos.y) * 1.5f };
        viz_log(TextFormat("red.vel = (Vector2){ %d, %d };", (int)red.vel.x, (int)red.vel.y), "пнули: поле vel внутри red");
        running = true;
    }

    if (!running) return;

    Vector2 before = red.pos;
    ball_move(&red, dt);
    bounce(&red);

    ball_move_copy(blue, dt);
    bounce(&blue);

    if (before.x == red.pos.x && before.y == red.pos.y && (red.vel.x != 0 || red.vel.y != 0)) {
        message = "Красный стоит: ball_move пока пустая. Посмотри демо (D), потом student.c, задание 3.";
        message_color = COL_BAD;
    } else {
        message = "Красный едет: функция получила АДРЕС и меняет настоящий мячик.";
        message_color = COL_GOOD;
    }
}

// Инспектор: поля структуры можно менять мышью, рядом видно, какие байты лежат в памяти.

static void field_bytes(const void* p, int n, char* out, int out_size)
{
    const unsigned char* b = (const unsigned char*)p;
    int len = 0;
    for (int i = 0; i < n && len < out_size - 4; i++) len += snprintf(out + len, out_size - len, "%02X ", b[i]);
}

static void draw_layout(float x, float y)
{
    Ball* ball = inspect_blue ? &blue : &red;
    const char* name = inspect_blue ? "blue" : "red";
    // цвет — это 4 байта (unsigned char), для ползунка временно переводим в float
    float col[4] = { ball->color.r, ball->color.g, ball->color.b, ball->color.a };
    struct { const char* name; size_t offset; float* value; float min, max, per_px, wheel; int bytes; } f[] = {
        { "pos.x",   offsetof(Ball, pos.x),   &ball->pos.x,  FIELD.x, FIELD.x + FIELD.width, 2.0f, 10, 4 },
        { "pos.y",   offsetof(Ball, pos.y),   &ball->pos.y,  FIELD.y, FIELD.y + FIELD.height, 2.0f, 10, 4 },
        { "vel.x",   offsetof(Ball, vel.x),   &ball->vel.x,  -600, 600, 4.0f, 20, 4 },
        { "vel.y",   offsetof(Ball, vel.y),   &ball->vel.y,  -600, 600, 4.0f, 20, 4 },
        { "radius",  offsetof(Ball, radius),  &ball->radius, 5, 80, 0.3f, 2, 4 },
        { "color.r", offsetof(Ball, color.r), &col[0], 0, 255, 1.5f, 8, 1 },
        { "color.g", offsetof(Ball, color.g), &col[1], 0, 255, 1.5f, 8, 1 },
        { "color.b", offsetof(Ball, color.b), &col[2], 0, 255, 1.5f, 8, 1 },
        { "color.a", offsetof(Ball, color.a), &col[3], 0, 255, 1.5f, 8, 1 },
    };

    viz_code(TextFormat("Ball %s;   sizeof(Ball) = %d", name, (int)sizeof(Ball)), x, y, 20, COL_TEXT);
    viz_code(TextFormat("&%s = %s", name, viz_addr(ball)), x, y + 26, 18, COL_ADDR);
    viz_code("смещ.  поле      значение (тяни)   байты", x, y + 54, 16, COL_DIM);

    for (int i = 0; i < (int)(sizeof f / sizeof f[0]); i++) {
        float ry = y + 78 + i * 31;
        viz_code(TextFormat("+%-2d", (int)f[i].offset), x, ry + 4, 18, COL_ADDR);
        viz_code(f[i].name, x + 56, ry + 4, 18, COL_TEXT);
        Rectangle slot = { x + 160, ry, 150, 27 };
        int st = viz_drag_value(slot, f[i].value, f[i].min, f[i].max, f[i].per_px, f[i].wheel);
        viz_code(f[i].bytes == 1 ? TextFormat("%d", (int)(*f[i].value + 0.5f)) : TextFormat("%.1f", *f[i].value),
                 slot.x + 8, ry + 4, 18, COL_HILITE);

        char hex[16] = "";
        field_bytes((const char*)ball + f[i].offset, f[i].bytes, hex, sizeof hex);
        viz_code(hex, x + 322, ry + 4, 17, COL_DIM);

        if (st != VIZ_DRAG_NONE && f[i].bytes == 1) {   // записываем цвет обратно в байты
            ball->color = (Color){ (unsigned char)(col[0] + 0.5f), (unsigned char)(col[1] + 0.5f),
                                   (unsigned char)(col[2] + 0.5f), (unsigned char)(col[3] + 0.5f) };
        }
        if (st == VIZ_DRAG_DONE) {
            viz_log(f[i].bytes == 1 ? TextFormat("%s.%s = %d;", name, f[i].name, (int)(*f[i].value + 0.5f))
                                    : TextFormat("%s.%s = %.1f;", name, f[i].name, *f[i].value),
                    TextFormat("пишем в байты %s + %d", viz_addr(ball), (int)f[i].offset));
            if (inspect_blue && (i == 2 || i == 3))
                message = "Скорость синего поменялась, но он всё равно стоит: ball_move_copy двигает копию!";
            message_color = COL_HILITE;
        }
    }
    viz_text("TAB — смотреть другой мячик. Колесо над полем — точная настройка.", x, y + 78 + 9 * 31 + 4, 16, COL_DIM);
}

static void draw(void)
{
    viz_title("Структуры",
              "Структура — несколько полей, сложенных в памяти друг за другом. Передать можно копию или адрес.");

    viz_frame(FIELD, 2.0f, COL_DIM);
    DrawCircleV(red.pos, red.radius, red.color);
    DrawCircleV(blue.pos, blue.radius, blue.color);

    // копия синего — полупрозрачный «призрак»
    if (ghost_alpha > 0.02f) {
        Color g = blue.color;
        g.a = (unsigned char)(110 * ghost_alpha);
        DrawCircleV(ghost_pos, blue.radius, g);
        DrawCircleLinesV(ghost_pos, blue.radius + 3, (Color){ 255, 255, 255, (unsigned char)(200 * ghost_alpha) });
        viz_code("b (копия)", ghost_pos.x - 45, ghost_pos.y + blue.radius + 6, 18,
                 (Color){ 255, 255, 255, (unsigned char)(255 * ghost_alpha) });
    }
    // указатель b на красный
    if (ptr_alpha > 0.02f) {
        unsigned char a = (unsigned char)(255 * ptr_alpha);
        viz_fill(PTR_BOX, viz_tint(g_theme.ptr_tile, a));
        viz_code("Ball* b", PTR_BOX.x + 20, PTR_BOX.y + 13, 20, (Color){ 255, 255, 255, a });
        Color c = COL_PTR;
        c.a = a;
        viz_arrow((Vector2){ PTR_BOX.x + PTR_BOX.width / 2, PTR_BOX.y }, viz_smooth(&red, 0, red.pos), c);
    }

    if (!demo.active) {
        viz_code("ball_move(&red, dt)", red.pos.x - 90, red.pos.y - red.radius - 26, 18, COL_TEXT);
        viz_code("ball_move_copy(blue, dt)", blue.pos.x - 110, blue.pos.y - blue.radius - 26, 18, COL_TEXT);
    }

    if (demo.active) {
        viz_code_panel(680, 150, 490, demo_code, 8, demo.line);
        viz_text("Синий получает копию: она двигается и исчезает.", 680, 400, 18, COL_DIM);
        viz_text("Красный получает адрес: двигается он сам.", 680, 424, 18, COL_DIM);
        demo_hint(&demo);
    } else {
        draw_layout(680, 150);
        float cx = 680, cy = 540;
        viz_code("red.pos.x     // точка — у самой структуры", cx, cy, 18, COL_TEXT);
        viz_code("p->pos.x      // стрелка — у указателя", cx, cy + 26, 18, COL_TEXT);
        viz_code("(*p).pos.x    // то же самое", cx, cy + 52, 18, COL_DIM);
        viz_hint("ПРОБЕЛ — старт/пауза   ЛКМ — пнуть красный   тяни поля справа — менять структуру   TAB — другой мячик   D — демо   R — сброс");
    }
    viz_message(message, message_color);
}

static void unload(void) {}

const Scene scene_structs = { "Структуры", init, update, draw, unload };
