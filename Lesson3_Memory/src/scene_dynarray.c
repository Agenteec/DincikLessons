// Сцена 4. Динамический массив: malloc/realloc, count и cap, переезд данных.
#include "scenes.h"
#include "student.h"
#include "viz.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_DRAWN_SLOTS 20
#define SLOT_W 22.0f
#define STRIP_X0 150.0f
#define STRIP_W  460.0f
#define LOG_LINES 2

static BallVec vec;
static int reallocs;
static char log_text[LOG_LINES][96];
static const char* message;
static Color message_color;

// анимации
static float pop;                 // 0..1 — появление последней ячейки
static float ghost_x, ghost_alpha; // старая полоска после realloc
static int   ghost_cap;
static const void* ghost_addr;

// проигрывание кода vec_push по строкам после каждого клика
static int   play[6];
static int   play_len, play_idx;
static float play_timer;

static const Rectangle FIELD = { 30, 345, 1140, 285 };

static const char* push_code[] = {
    "if (v->count == v->cap) {",
    "    v->cap = v->cap ? v->cap * 2 : 4;",
    "    v->data = realloc(..., v->cap * sizeof(Ball));",
    "}",
    "v->data[v->count] = b;",
    "v->count++;",
};

// Где рисовать полоску: позиция зависит от адреса, поэтому при переезде полоска «переезжает».
static float strip_x(const void* data, int cap)
{
    int slots = cap < MAX_DRAWN_SLOTS ? cap : MAX_DRAWN_SLOTS;
    int range = (int)(STRIP_W - slots * SLOT_W);
    if (!data || range <= 0) return STRIP_X0;
    uint32_t h = (uint32_t)(((uintptr_t)data >> 4) * 2654435761u);
    return STRIP_X0 + (float)(h % (uint32_t)range);
}

static void log_add(const char* s)
{
    for (int i = LOG_LINES - 1; i > 0; i--)
        snprintf(log_text[i], sizeof log_text[i], "%s", log_text[i - 1]);
    snprintf(log_text[0], sizeof log_text[0], "%s", s);
}

static void vec_clear(void)
{
    free(vec.data);
    vec = (BallVec){ 0 };
}

static void init(void)
{
    vec_clear();
    reallocs = 0;
    for (int i = 0; i < LOG_LINES; i++) log_text[i][0] = '\0';
    message = "";
    pop = 1;
    ghost_alpha = 0;
    play_len = 0;
}

static Ball random_ball(Vector2 pos)
{
    Color palette[] = { { 240, 90, 90, 255 }, { 27, 161, 226, 255 }, { 120, 220, 140, 255 },
                        { 255, 200, 80, 255 }, { 200, 120, 240, 255 }, { 255, 150, 200, 255 } };
    Ball b;
    b.pos = pos;
    b.vel = (Vector2){ (float)GetRandomValue(-200, 200), (float)GetRandomValue(-300, -100) };
    b.radius = (float)GetRandomValue(10, 22);
    b.color = palette[GetRandomValue(0, 5)];
    return b;
}

static void start_playback(bool grew)
{
    int with_realloc[] = { 0, 1, 2, 3, 4, 5 };
    int without[] = { 0, 4, 5 };
    const int* src = grew ? with_realloc : without;
    play_len = grew ? 6 : 3;
    for (int i = 0; i < play_len; i++) play[i] = src[i];
    play_idx = 0;
    play_timer = 0;
}

static void update(float dt)
{
    pop = viz_approach(pop, 1, 10, dt);
    ghost_alpha = viz_approach(ghost_alpha, 0, 1.5f, dt);
    if (play_len > 0) {
        play_timer += dt;
        if (play_timer > 0.35f) {
            play_timer = 0;
            if (++play_idx >= play_len + 3) play_len = 0;   // немного подержать последнюю строку
        }
    }

    Vector2 m = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, FIELD)) {
        Ball* old_data = vec.data;
        int old_count = vec.count, old_cap = vec.cap;
        float old_x = strip_x(old_data, old_cap);

        vec_push(&vec, random_ball(m));
        viz_log("vec_push(&vec, ball);",
                vec.count == old_count ? "ничего не произошло"
                : vec.cap != old_cap ? TextFormat("realloc! cap %d → %d, count = %d", old_cap, vec.cap, vec.count)
                                     : TextFormat("count = %d из cap = %d", vec.count, vec.cap));

        if (vec.count == old_count) {
            message = "Мячик не добавился: vec_push пока пустая. student.c, задание 4.";
            message_color = COL_BAD;
        } else {
            message = "";
            pop = 0;
            start_playback(vec.cap != old_cap);
            if (vec.data != old_data) {
                reallocs++;
                if (old_data) {
                    ghost_x = old_x;
                    ghost_cap = old_cap;
                    ghost_addr = old_data;
                    ghost_alpha = 1;
                    message = "Места не хватило: realloc выделил новый блок, скопировал мячики и освободил старый.";
                    message_color = COL_HILITE;
                }
                log_add(old_data ? TextFormat("realloc: %s \xE2\x86\x92 %s, cap = %d",
                                              viz_addr(old_data), viz_addr(vec.data), vec.cap)
                                 : TextFormat("первый malloc: %s, cap = %d", viz_addr(vec.data), vec.cap));
            }
        }
    }
    if (IsKeyPressed(KEY_BACKSPACE) && vec.count > 0) {
        vec.count--;
        viz_log("vec.count--;", TextFormat("count = %d, память не освобождаем", vec.count));
    }
    if (IsKeyPressed(KEY_C)) {
        bool had = vec.data != NULL;
        init();
        viz_log("free(vec.data); vec = (BallVec){ 0 };", had ? "data = NULL, count = cap = 0" : "free(NULL) — можно");
    }

    // физика: пробегаем по массиву через указатель на элемент
    for (int i = 0; i < vec.count; i++) {
        Ball* b = &vec.data[i];
        b->vel.y += 700.0f * dt;
        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;

        float floor_y = FIELD.y + FIELD.height - b->radius;
        if (b->pos.y > floor_y) { b->pos.y = floor_y; b->vel.y *= -0.8f; }
        if (b->pos.x < FIELD.x + b->radius && b->vel.x < 0) b->vel.x = -b->vel.x;
        if (b->pos.x > FIELD.x + FIELD.width - b->radius && b->vel.x > 0) b->vel.x = -b->vel.x;
    }
}

static void draw_strip(float x, float y, int cap, int count, const Ball* data, float alpha, bool animate_last)
{
    int shown = cap < MAX_DRAWN_SLOTS ? cap : MAX_DRAWN_SLOTS;
    for (int i = 0; i < shown; i++) {
        Rectangle r = { x + i * SLOT_W, y, SLOT_W - 3, 30 };
        if (data && i < count) {
            Color c = data[i].color;
            c.a = (unsigned char)(c.a * alpha);
            if (animate_last && i == count - 1) {
                float s = viz_ease(pop);
                r = (Rectangle){ r.x + r.width * (1 - s) / 2, r.y + r.height * (1 - s) / 2, r.width * s, r.height * s };
            }
            DrawRectangleRec(r, c);
        } else {
            Color c = COL_DIM;
            c.a = (unsigned char)(c.a * alpha);
            DrawRectangleLinesEx(r, 1.5f, c);
        }
    }
    if (cap > MAX_DRAWN_SLOTS) viz_code("...", x + shown * SLOT_W + 4, y + 4, 20, COL_DIM);
}

static void draw(void)
{
    viz_title("Динамический массив",
              "Размер заранее неизвестен: память берём через malloc/realloc. count — сколько лежит, cap — на сколько хватит.");

    float sy = 160;
    viz_code("vec.data", 30, sy + 4, 20, COL_PTR);

    // старый блок после realloc: строкой ниже, пунктиром и тает
    if (ghost_alpha > 0.02f) {
        draw_strip(ghost_x, sy + 38, ghost_cap, 0, NULL, ghost_alpha, false);
        int ghost_slots = ghost_cap < MAX_DRAWN_SLOTS ? ghost_cap : MAX_DRAWN_SLOTS;
        viz_code(TextFormat("free(%s)", viz_addr(ghost_addr)), ghost_x + ghost_slots * SLOT_W + 8, sy + 44, 15,
                 (Color){ 255, 70, 50, (unsigned char)(255 * ghost_alpha) });
    }

    if (vec.data) {
        Vector2 sx = viz_smooth(&vec, 0, (Vector2){ strip_x(vec.data, vec.cap), sy });
        viz_arrow((Vector2){ 125, sy + 15 }, (Vector2){ sx.x - 4, sy + 15 }, COL_PTR);
        draw_strip(sx.x, sy, vec.cap, vec.count, vec.data, 1, true);
    } else {
        viz_code("= NULL", 30, sy + 30, 18, COL_DIM);
    }

    viz_code(TextFormat("count = %d    cap = %d", vec.count, vec.cap), STRIP_X0, sy + 76, 20, COL_TEXT);
    viz_code(TextFormat("выделено: %d \xC3\x97 %d = %d байт", vec.cap, (int)sizeof(Ball), vec.cap * (int)sizeof(Ball)),
             STRIP_X0, sy + 100, 19, COL_DIM);
    viz_code(TextFormat("vec.data = %s    переездов: %d", viz_addr(vec.data), reallocs), STRIP_X0, sy + 124, 19, COL_ADDR);
    for (int i = 0; i < LOG_LINES; i++)
        viz_code(log_text[i], STRIP_X0, sy + 150 + i * 18, 15, i == 0 ? COL_HILITE : COL_DIM);

    int hl = (play_len > 0) ? play[play_idx < play_len ? play_idx : play_len - 1] : -1;
    viz_code_panel(640, 150, 530, push_code, 6, hl);

    viz_frame(FIELD, 2.0f, COL_DIM);
    for (int i = 0; i < vec.count; i++)
        DrawCircleV(vec.data[i].pos, vec.data[i].radius, vec.data[i].color);

    viz_message(message, message_color);
    viz_hint("ЛКМ — vec_push(&vec, мячик)     BACKSPACE — убрать последний     C — free и очистить");
}

static void unload(void) { vec_clear(); }

const Scene scene_dynarray = { "Дин. массив", init, update, draw, unload };
