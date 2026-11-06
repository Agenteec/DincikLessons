// Сцена 2. void* userData: Chipmunk хранит «адрес чего угодно», а мы потом приводим его к Pet*.
#include "phys.h"
#include "scenes.h"
#include "tickets.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static cpSpace* space;
static int      pets;
static const char* message;
static char     message_buf[200];
static Color    message_color;
static bool     bug_seen;   // хотя бы раз userData указал не на Pet

// пошаговое демо
static Demo     demo;
static cpBody*  demo_body;
static bool     demo_ok;          // userData указывает на Pet в куче
static const void* demo_heap;     // адрес Pet в куче (берём из счётчика памяти)
static const void* demo_ud;       // что лежит в userData
static float    frame_alpha, heap_alpha, body_alpha;

static const char* NAMES[] = { "Мурка", "Барсик", "Пушок", "Кекс", "Луна", "Тоша", "Ириска", "Персик" };

static const char* code_good[] = {
    "Pet* pet = malloc(sizeof(Pet));",
    "pet->magic = PET_MAGIC;  // + имя",
    "cpBodySetUserData(body, pet);",
    "return body;   // кадр spawn_pet стёрт",
    "Pet* p = cpBodyGetUserData(body);",
};
static const char* code_bad[] = {
    "Pet* pet = malloc(sizeof(Pet));",
    "pet->magic = PET_MAGIC;  // + имя",
    "cpBodySetUserData(body, &pet);  // баг",
    "return body;   // кадр spawn_pet стёрт",
    "Pet* p = cpBodyGetUserData(body);",
};

// userData указывает на стек? Сравниваем с адресом своей локальной переменной: стек — это близко.
static bool points_to_stack(const void* p)
{
    char here;
    intptr_t d = (intptr_t)((const char*)p - &here);
    return d > -(1 << 20) && d < (1 << 20);
}

// Настоящий ли Pet лежит по адресу из userData?
static const Pet* valid_pet(cpBody* body)
{
    const Pet* p = (const Pet*)cpBodyGetUserData(body);   // void* → Pet*: компилятор верит нам на слово
    if (!p || points_to_stack(p)) return NULL;
    return p->magic == PET_MAGIC ? p : NULL;
}

static void set_message(Color c, const char* text)
{
    snprintf(message_buf, sizeof message_buf, "%s", text);
    message = message_buf;
    message_color = c;
}

static void init(void)
{
    space = phys_new_space();
    phys_add_walls(space, 0, 0, NULL);
    pets = 0;
    bug_seen = false;
    demo_stop(&demo);
    demo_body = NULL;
    frame_alpha = heap_alpha = body_alpha = 0;
    set_message(COL_DIM, "Кликни по полю: появится шарик-питомец, его имя хранится в userData тела.");
}

static void free_pet(cpBody* body, void* data)
{
    (void)data;
    const Pet* p = valid_pet(body);
    if (p) mt_free((void*)p);   // мусорный адрес освобождать нельзя — упадём
}

static void unload(void)
{
    cpSpaceEachBody(space, free_pet, NULL);
    phys_free_space(space, false);
}

static const void* last_alloc_of_size(long long seq, size_t size)
{
    MtEvent ev[32];
    int n = mt_events_since(seq, ev, 32);
    for (int i = n - 1; i >= 0; i--)
        if (ev[i].kind > 0 && ev[i].size == size) return ev[i].ptr;
    return NULL;
}

static cpBody* spawn(Vector2 at)
{
    long long seq = mt_seq();
    const char* name = NAMES[pets % 8];
    cpBody* body = spawn_pet(space, cpv(at.x, at.y), name);   // функция из tickets.c
    body_set_color(body, (Color){ 255, 167, 38, 255 });
    pets++;

    demo_heap = last_alloc_of_size(seq, sizeof(Pet));
    demo_ud = cpBodyGetUserData(body);
    demo_ok = valid_pet(body) != NULL;
    if (!demo_ok) bug_seen = true;

    viz_log(TextFormat("spawn_pet(space, pos, \"%s\");", name),
            demo_ok ? TextFormat("userData = %s — это Pet в куче", viz_addr(demo_ud))
                    : TextFormat("userData = %s — это СТЕК, не Pet!", viz_addr(demo_ud)));
    return body;
}

static void demo_step(void)
{
    switch (demo.line) {
    case 0: heap_alpha = 0.01f; viz_log(code_good[0], TextFormat("Pet в куче: %s", viz_addr(demo_heap))); break;
    case 1: viz_log(code_good[1], "magic и имя записаны в Pet"); break;
    case 2:
        body_alpha = 0.01f;
        viz_log(demo_ok ? code_good[2] : "cpBodySetUserData(body, &pet);",
                demo_ok ? "в userData — адрес Pet" : "в userData — адрес ПЕРЕМЕННОЙ pet на стеке");
        break;
    case 3: viz_log("return body;", "кадр spawn_pet стёрт вместе с переменной pet"); break;
    case 4:
        viz_log(code_good[4], demo_ok ? "p → Pet, имя читается" : "p → старый стек: там уже другие данные");
        break;
    default:
        demo_stop(&demo);
        demo_body = NULL;
        if (demo_ok) set_message(COL_GOOD, "userData хранит адрес Pet в куче — Pet живёт, пока его не освободят.");
        else set_message(COL_BAD, "Нашла? В tickets.c (задание 2) в userData кладут &pet — адрес на стеке, который сразу исчезает.");
        return;
    }
    demo.line++;
}

static void update(float dt)
{
    frame_alpha = viz_approach(frame_alpha, demo.active && demo.line >= 1 && demo.line <= 3 ? 1.0f : 0.0f, 5, dt);
    if (heap_alpha > 0) heap_alpha = viz_approach(heap_alpha, demo.active ? 1.0f : 0.0f, 5, dt);
    if (body_alpha > 0) body_alpha = viz_approach(body_alpha, demo.active ? 1.0f : 0.0f, 5, dt);

    if (demo.active && demo_body) {   // питомец из демо висит наверху, пока идёт рассказ
        cpBodySetPosition(demo_body, cpv(FIELD_X + FIELD_W / 2, FIELD_Y + 60));
        cpBodySetVelocity(demo_body, cpvzero);
    }
    phys_step(space, dt);

    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        if (!demo.active) demo_body = NULL;
        return;
    }

    Vector2 m = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, field_rect())) {
        spawn(m);
        if (bug_seen) set_message(COL_BAD, "Имена превращаются в мусор. Нажми D и посмотри, куда на самом деле указывает userData.");
        else set_message(COL_GOOD, "Имя берётся из userData: (Pet*)cpBodyGetUserData(body).");
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        cpBody* b = phys_body_at(space, m);
        if (b) {
            long long seq = mt_seq();
            free_pet(b, NULL);
            phys_remove_body(space, b);
            viz_log("free(pet); phys_remove_body(...);", "питомец и тело освобождены");
            mem_log_since(seq);
        }
    }
    if (IsKeyPressed(KEY_D)) {
        demo_start(&demo);
        heap_alpha = body_alpha = 0;
        demo_body = spawn((Vector2){ FIELD_X + FIELD_W / 2, FIELD_Y + 60 });
        message = "";
    }
    if (IsKeyPressed(KEY_R)) { unload(); init(); viz_log("// новый мир", NULL); }
}

static void draw_label(cpBody* body, void* data)
{
    (void)data;
    if (cpBodyGetType(body) != CP_BODY_TYPE_DYNAMIC) return;
    cpVect p = cpBodyGetPosition(body);
    const Pet* pet = valid_pet(body);
    if (pet) {
        Vector2 sz = viz_measure(pet->name, 18);
        viz_text(pet->name, (float)p.x - sz.x / 2, (float)p.y - 50, 18, COL_TEXT);
    } else {
        // показываем, что на самом деле лежит по этому адресу
        const unsigned char* raw = (const unsigned char*)cpBodyGetUserData(body);
        char hex[32] = "NULL";
        if (raw) snprintf(hex, sizeof hex, "%02X %02X %02X %02X?", raw[4], raw[5], raw[6], raw[7]);
        Vector2 sz = viz_measure(hex, 16);
        viz_code(hex, (float)p.x - sz.x / 2, (float)p.y - 50, 16, COL_BAD);
    }
}

static void draw_demo(float x, float y)
{
    Color frame_c = viz_tint(COL_CELL, (unsigned char)(255 * frame_alpha));
    // стек: кадр spawn_pet
    viz_text("стек", x, y, 16, COL_DIM);
    Rectangle frame = { x, y + 22, 230, 96 };
    Rectangle cell_pet = { x + 14, y + 60, 200, 44 };
    if (frame_alpha > 0.02f) {
        viz_fill(frame, viz_tint(COL_PANEL, (unsigned char)(255 * frame_alpha)));
        viz_text("кадр spawn_pet()", x + 12, y + 28, 15, viz_tint(COL_DIM, (unsigned char)(255 * frame_alpha)));
        viz_fill(cell_pet, frame_c);
        viz_code(TextFormat("Pet* pet = %s", viz_addr(demo_heap)), cell_pet.x + 10, cell_pet.y + 12, 18,
                 viz_tint(COL_TEXT, (unsigned char)(255 * frame_alpha)));
    } else if (demo.line >= 4) {
        viz_code("кадр стёрт", x + 14, y + 70, 18, COL_DIM);
    }

    // куча: Pet
    viz_text("куча", x + 290, y, 16, COL_DIM);
    Rectangle heap = { x + 290, y + 22, 220, 96 };
    if (heap_alpha > 0.02f) {
        unsigned char a = (unsigned char)(255 * heap_alpha);
        viz_fill(heap, viz_tint((Color){ 0, 120, 140, 255 }, a));
        viz_code(TextFormat("Pet  %s", viz_addr(demo_heap)), heap.x + 10, heap.y + 8, 17, viz_tint(COL_TEXT, a));
        if (demo.line >= 2) {
            viz_code("magic = 0x50E7", heap.x + 10, heap.y + 36, 17, viz_tint(COL_TEXT, a));
            viz_code(TextFormat("name  = \"%s\"", NAMES[(pets - 1) % 8]), heap.x + 10, heap.y + 60, 17, viz_tint(COL_TEXT, a));
        }
        if (frame_alpha > 0.02f)
            viz_arrow((Vector2){ cell_pet.x + cell_pet.width, cell_pet.y + 22 }, (Vector2){ heap.x - 4, heap.y + 50 },
                      viz_tint(COL_PTR, (unsigned char)(255 * frame_alpha)));
    }

    // тело и его userData
    Rectangle body = { x + 140, y + 160, 240, 70 };
    if (body_alpha > 0.02f) {
        unsigned char a = (unsigned char)(255 * body_alpha);
        viz_fill(body, viz_tint(g_theme.ptr_tile, a));
        viz_code("cpBody", body.x + 10, body.y + 8, 18, viz_tint(COL_TEXT, a));
        viz_code(TextFormat("userData = %s", viz_addr(demo_ud)), body.x + 10, body.y + 38, 17, viz_tint(COL_TEXT, a));
        Vector2 from = { body.x + 120, body.y };
        if (demo_ok) {
            viz_arrow(from, (Vector2){ heap.x + 110, heap.y + heap.height + 4 }, viz_tint(COL_GOOD, a));
        } else {
            bool dangling = demo.line >= 4;
            Vector2 to = { cell_pet.x + 100, cell_pet.y + cell_pet.height + 4 };
            viz_arrow(from, to, viz_tint(dangling ? COL_BAD : COL_PTR, a));
            if (dangling) viz_text("а тут уже ничего нет!", to.x - 90, to.y + 26, 17, COL_BAD);
        }
    }
}

static void draw(void)
{
    viz_title("void* userData", "Chipmunk не знает, что такое Pet. Он хранит просто адрес (void*), а мы приводим его обратно к Pet*.");

    phys_draw(space);
    cpSpaceEachBody(space, draw_label, NULL);

    int hl = demo.active ? demo.line - 1 : -1;
    viz_code_panel(PANEL_X, FIELD_Y, PANEL_W, (demo.active ? !demo_ok : bug_seen) ? code_bad : code_good, 5, hl);
    if (demo.active || frame_alpha > 0.02f || heap_alpha > 0.02f) draw_demo(PANEL_X, FIELD_Y + 168);
    else {
        viz_text("void* — «адрес чего угодно».", PANEL_X, FIELD_Y + 180, 19, COL_TEXT);
        viz_text("Положить можно любой адрес — компилятор не проверит,", PANEL_X, FIELD_Y + 210, 17, COL_DIM);
        viz_text("живёт ли то, на что он указывает.", PANEL_X, FIELD_Y + 232, 17, COL_DIM);
        viz_text("Над шариком красные байты — значит, userData указывает не на Pet.", PANEL_X, FIELD_Y + 266, 17, COL_DIM);
    }
    mem_panel(PANEL_X, FIELD_Y + 400, PANEL_W);

    viz_message(message, message_color);
    if (demo.active) demo_hint(&demo);
    else viz_hint("ЛКМ — питомец     ПКМ — убрать     D — как userData попадает в тело, по шагам     R — новый мир");
}

const Scene scene_userdata = { "userData", init, update, draw, unload };
