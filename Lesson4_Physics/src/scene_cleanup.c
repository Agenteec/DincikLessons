// Сцена 5. Уборка: cpSpaceFree не освобождает тела и формы внутри — это делает хозяин, в правильном порядке.
#include "phys.h"
#include "scenes.h"
#include "tickets.h"

#include <stdio.h>
#include <string.h>

static Level lv;
static bool  level_alive;
static int   level_tag;   // метка памяти текущего уровня (см. mt_set_tag)
static int   level_no;
static int   last_leak_blocks = -1;
static size_t last_leak_bytes;
static int   total_leak_blocks;
static size_t total_leak_bytes;
static int   exits;
static const char* message;
static char  message_buf[220];
static Color message_color;

// ---------- головоломка: 4 строки уборки ----------
enum { CARD_SHAPES, CARD_PETS, CARD_BODIES, CARD_SPACE, CARD_COUNT };

static const char* card_code[CARD_COUNT] = {
    "for (формы) { cpSpaceRemoveShape; cpShapeFree; }",
    "for (тела) free(cpBodyGetUserData(body));",
    "for (тела) { cpSpaceRemoveBody; cpBodyFree; }",
    "cpSpaceFree(space);",
};
static const char* group_name[CARD_COUNT] = { "формы", "питомцы", "тела", "space" };
static const int   shown_order[CARD_COUNT] = { CARD_BODIES, CARD_PETS, CARD_SPACE, CARD_SHAPES };   // вперемешку

static int   chosen[CARD_COUNT];      // порядок, который выбрала она
static int   chosen_count;
static int   sim_step = -1;           // -1 — не проигрываем; иначе сколько шагов уже выполнено
static float sim_timer;
static int   sim_fail = -1;           // на каком шаге всё сломалось
static bool  freed[CARD_COUNT];
static const char* sim_reason;
static float flash;

static const Rectangle CARDS = { PANEL_X, FIELD_Y + 26, PANEL_W, 4 * 50.0f };

static void set_message(Color c, const char* fmt, int a, const char* b)
{
    snprintf(message_buf, sizeof message_buf, fmt, a, b);
    message = message_buf;
    message_color = c;
}

static void build_level(void)
{
    level_no++;
    level_tag = 1 + level_no % 63;   // всё, что выделится для этого уровня, получит его метку
    int old_tag = mt_set_tag(level_tag);

    memset(&lv, 0, sizeof lv);
    lv.space = phys_new_space();
    lv.shape_count = phys_add_walls(lv.space, 0, 0, lv.shapes);

    Color box_c[4] = { { 66, 165, 245, 255 }, { 102, 187, 106, 255 }, { 171, 71, 188, 255 }, { 38, 198, 218, 255 } };
    for (int i = 0; i < 4; i++) {
        cpBody* b = phys_add_box(lv.space, cpv(FIELD_X + 120 + i * 120, FIELD_Y + 200), 50, 50, box_c[i]);
        lv.bodies[lv.body_count++] = b;
    }
    const char* names[3] = { "Мурка", "Кекс", "Луна" };
    for (int i = 0; i < 3; i++) {
        cpBody* b = phys_add_ball(lv.space, cpv(FIELD_X + 180 + i * 130, FIELD_Y + 160), 22, (Color){ 255, 167, 38, 255 });
        Pet* pet = mt_malloc(sizeof(Pet));
        pet->magic = PET_MAGIC;
        snprintf(pet->name, sizeof pet->name, "%s", names[i]);
        pet->mood = 5;
        cpBodySetUserData(b, pet);
        lv.bodies[lv.body_count++] = b;
    }
    level_alive = true;
    mt_set_tag(old_tag);
}

static void collect_shape(cpBody* body, cpShape* shape, void* data)
{
    (void)body; (void)data;
    lv.shapes[lv.shape_count++] = shape;
}

static void finish_level_lists(void)
{
    // формы тел тоже кладём в список — level_cleanup должен их освободить
    for (int i = 0; i < lv.body_count; i++) cpBodyEachShape(lv.bodies[i], collect_shape, NULL);
}

static void reset_puzzle(void)
{
    chosen_count = 0;
    sim_step = -1;
    sim_fail = -1;
    sim_reason = NULL;
    for (int i = 0; i < CARD_COUNT; i++) freed[i] = false;
}

static void init(void)
{
    build_level();
    finish_level_lists();
    reset_puzzle();
    message = "Нажми E — «выйти с уровня»: вызовется level_cleanup() из tickets.c. Смотри, сколько памяти останется.";
    message_color = COL_DIM;
}

static void unload(void)
{
    if (level_alive) phys_free_space(lv.space, true);   // правильная уборка: питомцы — через mt_free
    level_alive = false;
}

// ---------- проигрывание выбранного порядка (без настоящего free: только проверяем правила) ----------
static void sim_execute(int card)
{
    switch (card) {
    case CARD_SHAPES:
        if (freed[CARD_SPACE]) sim_reason = "cpSpaceRemoveShape(space, ...): space уже освобождён — читаем 0xDD...";
        else if (freed[CARD_BODIES]) sim_reason = "cpSpaceRemoveShape будит тело формы (shape->body), а тело уже освобождено!";
        break;
    case CARD_PETS:
        if (freed[CARD_BODIES]) sim_reason = "cpBodyGetUserData(body): тело уже освобождено — адрес питомца не достать.";
        break;
    case CARD_BODIES:
        if (freed[CARD_SPACE]) sim_reason = "cpSpaceRemoveBody(space, ...): space уже освобождён.";
        break;
    default:
        break;
    }
    freed[card] = true;
}

static void update(float dt)
{
    flash = viz_approach(flash, 0, 1.5f, dt);
    if (level_alive) {
        int old_tag = mt_set_tag(level_tag);   // Chipmunk выделяет память и во время шага
        phys_step(lv.space, dt);
        mt_set_tag(old_tag);
    }

    // проигрывание головоломки
    if (sim_step >= 0 && sim_fail < 0 && sim_step < CARD_COUNT) {
        sim_timer += dt;
        if (sim_timer > 0.8f) {
            sim_timer = 0;
            sim_execute(chosen[sim_step]);
            viz_log(card_code[chosen[sim_step]], sim_reason ? "ОШИБКА: use-after-free" : "ок");
            if (sim_reason) {
                sim_fail = sim_step;
                flash = 1;
                set_message(COL_BAD, "Шаг %d: %s", sim_step + 1, sim_reason);
            }
            sim_step++;
            if (sim_step == CARD_COUNT && sim_fail < 0)
                set_message(COL_GOOD, "Порядок верный! Расставь строки в tickets.c (задание %d) так же и нажми E.%s", 5, "");
        }
    }

    Vector2 m = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, CARDS) && sim_step < 0) {
        int row = (int)((m.y - CARDS.y) / 50);
        int card = shown_order[row];
        int at = -1;
        for (int i = 0; i < chosen_count; i++) if (chosen[i] == card) at = i;
        if (at >= 0) chosen_count = at;                 // повторный клик — отменить с этого места
        else chosen[chosen_count++] = card;
        if (chosen_count == CARD_COUNT) {
            sim_step = 0;
            sim_timer = 0;
            message = "Проверяем порядок...";
            message_color = COL_HILITE;
        }
    }
    if (IsKeyPressed(KEY_S)) { reset_puzzle(); message = ""; }

    if (IsKeyPressed(KEY_E) && level_alive) {
        exits++;
        viz_log("level_cleanup(&lv);", "функция из tickets.c");
        level_cleanup(&lv);   // задание 5
        level_alive = false;
        last_leak_blocks = mt_tag_blocks(level_tag);   // что осталось от этого уровня
        last_leak_bytes = mt_tag_bytes(level_tag);
        total_leak_blocks += last_leak_blocks;
        total_leak_bytes += last_leak_bytes;
        viz_log("// после уборки", last_leak_blocks > 0 ? TextFormat("утекло %d блоков, %s", last_leak_blocks, fmt_bytes(last_leak_bytes))
                                                       : "утечек нет!");
        if (last_leak_blocks > 0)
            set_message(COL_BAD, "Утекло %d блоков: их адреса потеряны навсегда. Реши головоломку справа и почини level_cleanup.%s",
                        last_leak_blocks, "");
        else set_message(COL_GOOD, "Чисто: после уборки в куче ничего не осталось.", 0, "");
        build_level();   // следующий уровень
        finish_level_lists();
    }
    if (IsKeyPressed(KEY_R)) {
        total_leak_blocks = 0;
        total_leak_bytes = 0;
        exits = 0;
        last_leak_blocks = -1;
        viz_log("// счётчик утечек обнулён (сама память так и потеряна)", NULL);
    }
}

static void draw_labels(void)
{
    for (int i = 0; i < lv.body_count; i++) {
        const Pet* p = cpBodyGetUserData(lv.bodies[i]);
        if (!p) continue;
        cpVect pos = cpBodyGetPosition(lv.bodies[i]);
        Vector2 sz = viz_measure(p->name, 17);
        viz_text(p->name, (float)pos.x - sz.x / 2, (float)pos.y - 46, 17, COL_TEXT);
    }
}

static void draw_puzzle(void)
{
    viz_text("Головоломка: в каком порядке убирать? Кликай строки по очереди.", PANEL_X, FIELD_Y, 17, COL_DIM);
    for (int row = 0; row < CARD_COUNT; row++) {
        int card = shown_order[row];
        Rectangle r = { CARDS.x + 44, CARDS.y + row * 50, CARDS.width - 44, 42 };
        int at = -1;
        for (int i = 0; i < chosen_count; i++) if (chosen[i] == card) at = i;

        Color fill = COL_PANEL;
        if (sim_step >= 0 && at >= 0 && at < sim_step) fill = at == sim_fail ? g_theme.node_freed : viz_tint(COL_GOOD, 90);
        viz_fill(r, fill);
        if (at == sim_fail && at >= 0) viz_glow(r, flash, COL_BAD);
        viz_code(card_code[card], r.x + 12, r.y + 11, 17, COL_TEXT);

        Rectangle badge = { CARDS.x, r.y + 4, 34, 34 };
        if (at >= 0) {
            viz_fill(badge, COL_ACCENT);
            viz_code(TextFormat("%d", at + 1), badge.x + 11, badge.y + 6, 20, COL_BG);
        } else {
            viz_frame(badge, 1.5f, COL_DIM);
        }
    }

    // что уже «освобождено» в проигрывании
    float gy = CARDS.y + CARDS.height + 16;
    for (int i = 0; i < CARD_COUNT; i++) {
        Rectangle g = { PANEL_X + i * 132.0f, gy, 122, 46 };
        bool bad = sim_fail >= 0 && chosen[sim_fail] == i;
        viz_fill(g, freed[i] ? viz_tint(COL_CELL, 120) : COL_CELL);
        if (bad) viz_frame(g, 2, COL_BAD);
        viz_text(group_name[i], g.x + 10, g.y + 4, 17, freed[i] ? COL_DIM : COL_TEXT);
        viz_code(freed[i] ? "free" : "живо", g.x + 10, g.y + 24, 15, freed[i] ? COL_BAD : COL_GOOD);
    }
}

static void draw(void)
{
    viz_title("Уборка", "cpSpaceFree освобождает только сам мир. Тела, формы и питомцев внутри должен освободить ты.");

    if (level_alive) {
        phys_draw(lv.space);
        draw_labels();
    }
    // табло утечек поверх поля
    viz_code(TextFormat("выходов с уровня: %d", exits), FIELD_X + 12, FIELD_Y + 10, 18, COL_DIM);
    if (last_leak_blocks >= 0)
        viz_code(last_leak_blocks > 0 ? TextFormat("утекло в последний раз: %d блоков, %s", last_leak_blocks, fmt_bytes(last_leak_bytes))
                                      : "в последний раз: утечек нет",
                 FIELD_X + 12, FIELD_Y + 34, 18, last_leak_blocks > 0 ? COL_BAD : COL_GOOD);
    if (total_leak_blocks > 0)
        viz_code(TextFormat("всего потеряно: %s", fmt_bytes(total_leak_bytes)), FIELD_X + 12, FIELD_Y + 58, 18, COL_BAD);

    draw_puzzle();
    mem_panel(PANEL_X, FIELD_Y + 400, PANEL_W);

    viz_message(message, message_color);
    viz_hint("E — выйти с уровня (level_cleanup из tickets.c)     клик по строкам — головоломка     S — заново     R — обнулить счётчик");
}

const Scene scene_cleanup = { "Уборка", init, update, draw, unload };
