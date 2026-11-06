// Сцена 6. static, const, sizeof — полезные мелочи.
#include "scenes.h"
#include "student.h"
#include "viz.h"

#define MAX_TICKETS 10

static int  good[MAX_TICKETS];
static int  bad[MAX_TICKETS];
static int  ticket_count;
static bool show_answers;

// анимации
static float slide;          // 0..1 — новый номерок въезжает справа
static float stack_t = -1;   // время с момента вызова broken_ticket; <0 — кадра на стеке нет
static float static_pulse;
static float quiz_alpha;

static const Rectangle STATIC_BOX = { 830, 170, 330, 80 };
static const Rectangle STACK_BOX = { 830, 290, 330, 80 };

// Обычная локальная переменная: создаётся заново при каждом вызове.
static int broken_ticket(void)
{
    int n = 0;
    n++;
    return n;
}

static void init(void)
{
    ticket_count = 0;
    show_answers = false;
    slide = 1;
    stack_t = -1;
}

static void update(float dt)
{
    slide = viz_approach(slide, 1, 9, dt);
    static_pulse = viz_approach(static_pulse, 0, 3, dt);
    quiz_alpha = viz_approach(quiz_alpha, show_answers ? 1.0f : 0.0f, 8, dt);
    if (stack_t >= 0) {
        stack_t += dt;
        if (stack_t > 2.0f) stack_t = -1;
    }

    if (IsKeyPressed(KEY_T)) {
        if (ticket_count == MAX_TICKETS) {
            for (int i = 1; i < MAX_TICKETS; i++) { good[i - 1] = good[i]; bad[i - 1] = bad[i]; }
            ticket_count--;
        }
        good[ticket_count] = next_ticket();
        bad[ticket_count] = broken_ticket();
        viz_log("next_ticket();", TextFormat("вернула %d (counter помнит)", good[ticket_count]));
        viz_log("broken_ticket();", TextFormat("вернула %d (n каждый раз = 0)", bad[ticket_count]));
        ticket_count++;
        slide = 0;
        stack_t = 0;
        static_pulse = 1;
    }
    if (IsKeyPressed(KEY_Q)) show_answers = !show_answers;
    if (IsKeyPressed(KEY_R)) init();
}

static void draw_tickets(float y, const char* label, const int* values, Color c)
{
    viz_code(label, 30, y, 20, COL_TEXT);
    for (int i = 0; i < ticket_count; i++) {
        Rectangle r = { 30 + i * 62.0f, y + 30, 54, 54 };
        if (i == ticket_count - 1) {
            float e = viz_ease(slide);
            r.x += (1 - e) * 200;
            c.a = (unsigned char)(255 * e);
        }
        viz_fill(r, c);
        const char* s = TextFormat("%d", values[i]);
        viz_code(s, r.x + 27 - viz_measure(s, 24).x / 2, r.y + 13, 24, COL_BG);
    }
}

// справа: где живут переменные двух функций
static void draw_memory(void)
{
    viz_text("Где живут переменные", STATIC_BOX.x, STATIC_BOX.y - 44, 20, COL_TEXT);

    // static-память: существует всю программу
    viz_fill(STATIC_BOX, COL_PANEL);
    viz_glow(STATIC_BOX, static_pulse, COL_GOOD);
    viz_text("статическая память — живёт всю программу", STATIC_BOX.x + 10, STATIC_BOX.y + 6, 15, COL_DIM);
    int last = ticket_count > 0 ? good[ticket_count - 1] : 0;
    viz_code(TextFormat("static int counter = %d", last), STATIC_BOX.x + 10, STATIC_BOX.y + 36, 21, COL_GOOD);

    // стек: кадр broken_ticket появляется на время вызова и исчезает
    viz_frame(STACK_BOX, 1.5f, COL_DIM);
    viz_text("стек — кадр живёт, пока функция работает", STACK_BOX.x + 10, STACK_BOX.y + 6, 15, COL_DIM);
    if (stack_t >= 0) {
        float a = stack_t < 0.2f ? stack_t / 0.2f : (stack_t > 1.4f ? 1.0f - (stack_t - 1.4f) / 0.6f : 1.0f);
        if (a < 0) a = 0;
        unsigned char ua = (unsigned char)(255 * a);
        Rectangle frame = { STACK_BOX.x + 8, STACK_BOX.y + 30, STACK_BOX.width - 16, 42 };
        viz_fill(frame, (Color){ 162, 0, 37, ua });
        const char* line = stack_t < 0.5f ? "broken_ticket(): int n = 0"
                         : stack_t < 1.4f ? "broken_ticket(): n++  \xE2\x86\x92 1"
                                          : "return \xE2\x80\x94 кадр стёрт";
        viz_code(line, frame.x + 10, frame.y + 10, 19, (Color){ 255, 255, 255, ua });
    }
}

static void draw(void)
{
    viz_title("static и другие мелочи",
              "static у локальной переменной: она живёт всю программу и помнит значение между вызовами.");

    draw_tickets(150, "next_ticket()     // твоя, со static", good, COL_GOOD);
    draw_tickets(260, "broken_ticket()   // int n = 0;", bad, COL_BAD);
    draw_memory();

    // sizeof
    float sx = 30, sy = 400;
    viz_text("Сколько байт занимает:", sx, sy, 22, COL_TEXT);
    struct { const char* name; int size; } sizes[] = {
        { "char", (int)sizeof(char) },   { "int", (int)sizeof(int) },
        { "float", (int)sizeof(float) }, { "double", (int)sizeof(double) },
        { "int*", (int)sizeof(int*) },   { "Ball", (int)sizeof(Ball) },
        { "Node", (int)sizeof(Node) },   { "BallVec", (int)sizeof(BallVec) },
    };
    for (int i = 0; i < 8; i++) {
        float x = sx + (i % 2) * 260, y = sy + 34 + (i / 2) * 28;
        viz_code(TextFormat("sizeof(%s)", sizes[i].name), x, y, 19, COL_DIM);
        viz_code(TextFormat("= %d", sizes[i].size), x + 175, y, 19, COL_HILITE);
    }
    viz_text("Любой указатель — 8 байт: это просто адрес.", sx, sy + 154, 18, COL_DIM);

    // мини-викторина
    float qx = 560, qy = 400;
    viz_text("Викторина (Q — показать ответы):", qx, qy, 22, COL_TEXT);
    const char* q[] = {
        "const int* a;     можно ли a = &x?   а *a = 5?",
        "int* const b;     можно ли b = &x?   а *b = 5?",
        "static int g;     (в начале .c файла) кто её видит?",
        "static int k;     (внутри функции) когда обнуляется?",
    };
    const char* ans[] = {
        "a = &x — можно, *a = 5 — нельзя (const число)",
        "b = &x — нельзя, *b = 5 — можно (const адрес)",
        "только этот .c файл: она спрятана от других файлов",
        "один раз при старте программы, дальше помнит значение",
    };
    Color ac = COL_GOOD;
    ac.a = (unsigned char)(255 * quiz_alpha);
    for (int i = 0; i < 4; i++) {
        viz_code(q[i], qx, qy + 36 + i * 52, 17, COL_TEXT);
        if (quiz_alpha > 0.02f) viz_text(ans[i], qx + 20 + (1 - quiz_alpha) * 30, qy + 58 + i * 52, 17, ac);
    }

    viz_hint("T — взять номерок у обеих функций     Q — ответы викторины     R — сброс");
}

static void unload(void) {}

const Scene scene_static = { "static", init, update, draw, unload };
