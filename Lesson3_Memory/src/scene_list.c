// Сцена 5. Связный список: узлы где попало в памяти, стрелки next, Node** как «указатель на стрелку».
#include "scenes.h"
#include "student.h"
#include "viz.h"

#include <stdlib.h>

#define NODE_W 120.0f
#define NODE_H 46.0f
#define GHOST_MAX 8

static Node* head;
static int   next_value;
static const char* message;
static Color message_color;

// перетаскивание колёсиком
static Node*   dragging;
static Vector2 drag_offset;

// анимации
static const Node* popped;      // только что добавленный узел
static float       pop;
typedef struct { Vector2 pos; int value; float alpha; } Ghost;
static Ghost ghosts[GHOST_MAX];  // освобождённые узлы тают на месте

// проигрывание кода list_push_front после клика
static int   push_line = -1;
static float push_timer;

// пошаговое удаление
static Demo   demo;
static Node** pp;
static Node*  target;
static bool   unlinked;

static const Rectangle FIELD = { 340, 140, 830, 470 };
static const Rectangle HEAD_BOX = { 40, 200, 130, 50 };

static const char* push_code[] = {
    "n = malloc(sizeof(Node));",
    "n->value = value; ...",
    "n->next = *head;",
    "*head = n;",
};

static const char* remove_code[] = {
    "Node** pp = &head;",
    "while (*pp != target)",
    "    pp = &(*pp)->next;",
    "*pp = target->next;",
    "free(target);",
};

static int list_length(void)
{
    int n = 0;
    for (Node* it = head; it; it = it->next) n++;
    return n;
}

static void free_all(void)
{
    while (head) {
        Node* next = head->next;
        free(head);
        head = next;
    }
}

static void init(void)
{
    free_all();
    next_value = 1;
    dragging = NULL;
    popped = NULL;
    push_line = -1;
    demo_stop(&demo);
    for (int i = 0; i < GHOST_MAX; i++) ghosts[i].alpha = 0;
    message = "Подсказка: сначала напиши list_push_front, иначе в списке нечего будет удалять.";
    message_color = COL_DIM;
}

static void add_ghost(Vector2 pos, int value)
{
    int slot = 0;
    for (int i = 0; i < GHOST_MAX; i++)
        if (ghosts[i].alpha < ghosts[slot].alpha) slot = i;
    ghosts[slot] = (Ghost){ pos, value, 1.0f };
}

static Rectangle node_rect_at(Vector2 pos)
{
    return (Rectangle){ pos.x - NODE_W / 2, pos.y - NODE_H / 2, NODE_W, NODE_H };
}

static Rectangle node_rect(const Node* n) { return node_rect_at(n->pos); }

static Rectangle next_field(const Node* n)
{
    Rectangle r = node_rect(n);
    return (Rectangle){ r.x + NODE_W / 2, r.y, NODE_W / 2, NODE_H };
}

static Node* node_under_mouse(void)
{
    for (Node* it = head; it; it = it->next)
        if (CheckCollisionPointRec(GetMousePosition(), node_rect(it))) return it;
    return NULL;
}

static void demo_step(void)
{
    switch (demo.line) {
    case 0:
    case 2:   // проверяем условие цикла
        if (demo.line == 0) viz_log("Node** pp = &head;", "pp смотрит на переменную head");
        viz_log("while (*pp != target)", *pp != target ? TextFormat("*pp = узел %d, не он", (*pp)->value) : "нашли!");
        if (*pp != target) {
            demo.line = 1;
        } else {
            viz_log("*pp = target->next;", target->next ? TextFormat("стрелка теперь на узел %d", target->next->value)
                                                         : "стрелка теперь NULL");
            *pp = target->next;   // стрелка перецепится плавно
            unlinked = true;
            demo.line = 3;
        }
        break;
    case 1:   // pp = &(*pp)->next;
        viz_log("pp = &(*pp)->next;", TextFormat("pp → поле next узла %d", (*pp)->value));
        pp = &(*pp)->next;
        demo.line = 2;
        break;
    case 3:   // free(target);
        viz_log("free(target);", TextFormat("узел %d по %s освобождён", target->value, viz_addr(target)));
        add_ghost(target->pos, target->value);
        if (dragging == target) dragging = NULL;
        free(target);
        target = NULL;
        demo.line = 4;
        break;
    default:
        demo_stop(&demo);
        message = "Готово! Теперь напиши то же самое в list_remove (задание 5б) и удаляй правой кнопкой.";
        message_color = COL_GOOD;
        break;
    }
}

static void update(float dt)
{
    pop = viz_approach(pop, 1, 8, dt);
    for (int i = 0; i < GHOST_MAX; i++) {
        ghosts[i].alpha = viz_approach(ghosts[i].alpha, 0, 1.5f, dt);
        ghosts[i].pos.y -= 20 * dt;
    }
    if (push_line >= 0) {
        push_timer += dt;
        if (push_timer > 0.4f) {
            push_timer = 0;
            if (++push_line > 6) push_line = -1;   // строки 0..3, потом чуть подержать
        }
    }

    // перетаскивание колёсиком — работает всегда, и в демо тоже
    Vector2 m = GetMousePosition();
    Node* hovered = node_under_mouse();
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) && hovered) {
        dragging = hovered;
        drag_offset = (Vector2){ m.x - hovered->pos.x, m.y - hovered->pos.y };
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) dragging = NULL;
    if (dragging) {
        Vector2 p = { m.x - drag_offset.x, m.y - drag_offset.y };
        if (p.x < FIELD.x + NODE_W / 2) p.x = FIELD.x + NODE_W / 2;
        if (p.x > FIELD.x + FIELD.width - NODE_W / 2) p.x = FIELD.x + FIELD.width - NODE_W / 2;
        if (p.y < FIELD.y + NODE_H / 2) p.y = FIELD.y + NODE_H / 2;
        if (p.y > FIELD.y + FIELD.height - NODE_H / 2) p.y = FIELD.y + FIELD.height - NODE_H / 2;
        dragging->pos = p;
    }

    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hovered && CheckCollisionPointRec(m, FIELD)) {
        Node* old_head = head;
        list_push_front(&head, next_value, m);
        viz_log(TextFormat("list_push_front(&head, %d, pos);", next_value),
                head == old_head ? "head не изменился" : TextFormat("head = %s (узел %d)", viz_addr(head), next_value));
        if (head == old_head) {
            message = "Узел не появился: list_push_front пока пустая. student.c, задание 5а.";
            message_color = COL_BAD;
        } else {
            next_value++;
            popped = head;
            pop = 0;
            push_line = 0;
            push_timer = 0;
            message = "Новый узел стал первым: head теперь указывает на него.";
            message_color = COL_GOOD;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hovered) {
        int before = list_length();
        Vector2 pos = hovered->pos;
        int value = hovered->value;
        const char* addr = viz_addr(hovered);
        list_remove(&head, hovered);   // после этого hovered может быть уже освобождён — не трогаем его
        viz_log(TextFormat("list_remove(&head, %s);", addr),
                list_length() == before ? "ничего не удалилось" : TextFormat("узел %d удалён, длина %d", value, list_length()));
        if (list_length() == before) {
            message = "Узел не удалился: list_remove пока пустая. Сначала посмотри демо (D на узле).";
            message_color = COL_BAD;
        } else {
            add_ghost(pos, value);
            dragging = NULL;
            message = "Удалено через твою list_remove!";
            message_color = COL_GOOD;
        }
    }

    if (IsKeyPressed(KEY_D) && hovered) {
        demo_start(&demo);
        target = hovered;
        pp = &head;
        unlinked = false;
        message = "";
    }
    if (IsKeyPressed(KEY_C)) { init(); viz_log("// free всех узлов, head = NULL", NULL); }
}

// куда сейчас смотрит pp: на переменную head или на поле next какого-то узла
static Rectangle pp_target_rect(void)
{
    if (pp == &head) return HEAD_BOX;
    for (Node* it = head; it; it = it->next)
        if (&it->next == pp) return next_field(it);
    return (Rectangle){ 0 };
}

static void draw_node(const Node* n, bool is_target)
{
    Rectangle r = node_rect(n);
    if (n == popped && pop < 1) {
        float s = 0.3f + 0.7f * viz_ease(pop);
        r = (Rectangle){ n->pos.x - NODE_W * s / 2, n->pos.y - NODE_H * s / 2, NODE_W * s, NODE_H * s };
    }
    Rectangle nf = { r.x + r.width / 2, r.y, r.width / 2, r.height };
    Color fill = is_target ? (Color){ 162, 0, 37, 255 } : (n == dragging ? (Color){ 0, 110, 160, 255 } : COL_CELL);

    viz_fill(r, fill);
    viz_frame(r, 2.0f, COL_DIM);
    DrawLineEx((Vector2){ nf.x, nf.y + 4 }, (Vector2){ nf.x, nf.y + r.height - 4 }, 1.5f, COL_DIM);

    viz_code(TextFormat("%d", n->value), r.x + 18, r.y + 10, 24, COL_TEXT);
    viz_code(n->next ? "next" : "NULL", nf.x + 8, nf.y + 13, 18, n->next ? COL_PTR : COL_DIM);
    viz_code(viz_addr(n), r.x, r.y + r.height + 2, 15, COL_ADDR);
}

// стрелка из поля-указателя (head или node->next) к узлу; конец едет плавно
static void draw_link(const void* field_key, Vector2 from, const Node* to)
{
    Rectangle tr = node_rect(to);
    Vector2 end = { tr.x, tr.y + NODE_H / 2 };
    if (end.x < from.x) end = (Vector2){ tr.x + NODE_W / 2, from.y < tr.y ? tr.y : tr.y + NODE_H };
    viz_arrow(from, viz_smooth(field_key, 0, end), COL_PTR);
}

static Vector2 next_anchor(const Node* n)
{
    Rectangle nf = next_field(n);
    return (Vector2){ nf.x + nf.width - 10, nf.y + NODE_H / 2 };
}

static void draw(void)
{
    viz_title("Связный список",
              "Узлы лежат где попало, их связывают только стрелки next. Node** — указатель НА СТРЕЛКУ.");

    viz_frame(FIELD, 2.0f, (Color){ 55, 55, 55, 255 });

    // освобождённые узлы тают
    for (int i = 0; i < GHOST_MAX; i++) {
        if (ghosts[i].alpha < 0.02f) continue;
        unsigned char a = (unsigned char)(255 * ghosts[i].alpha);
        Rectangle r = node_rect_at(ghosts[i].pos);
        viz_frame(r, 2.0f, (Color){ 255, 70, 50, a });
        viz_code(TextFormat("%d", ghosts[i].value), r.x + 18, r.y + 10, 24, (Color){ 255, 70, 50, a });
        viz_code("free", r.x + 66, r.y + 13, 18, (Color){ 255, 70, 50, a });
    }

    // head
    viz_fill(HEAD_BOX, g_theme.ptr_tile);
    viz_code("Node* head", HEAD_BOX.x, HEAD_BOX.y - 24, 20, COL_TEXT);
    viz_code(viz_addr(head), HEAD_BOX.x + 30, HEAD_BOX.y + 14, 20, COL_TEXT);
    if (head) draw_link(&head, (Vector2){ HEAD_BOX.x + HEAD_BOX.width - 10, HEAD_BOX.y + 25 }, head);

    for (Node* it = head; it; it = it->next)
        if (it->next) draw_link(&it->next, next_anchor(it), it->next);
    // отцепленный, но ещё не освобождённый узел
    if (demo.active && unlinked && target && target->next)
        draw_link(&target->next, next_anchor(target), target->next);

    for (Node* it = head; it; it = it->next) draw_node(it, demo.active && it == target);
    if (demo.active && unlinked && target) draw_node(target, true);

    viz_text(TextFormat("Длина списка: %d", list_length()), 40, 290, 20, COL_TEXT);

    if (demo.active) {
        viz_code_panel(20, 340, 310, remove_code, 5, demo.line);
        Rectangle t = pp_target_rect();
        if (t.width > 0) {
            Vector2 p = viz_smooth(&pp, 0, (Vector2){ t.x, t.y });
            Vector2 s = viz_smooth(&pp, 1, (Vector2){ t.width, t.height });
            Rectangle box = { p.x - 5, p.y - 5, s.x + 10, s.y + 10 };
            viz_frame(box, 3.0f, COL_HILITE);
            viz_code("pp", box.x + box.width / 2 - 10, box.y + box.height + 4, 22, COL_HILITE);
        }
        viz_text("Жёлтая рамка — то, на что указывает pp.", 20, 500, 18, COL_HILITE);
        viz_text("*pp — стрелка внутри этой рамки.", 20, 524, 18, COL_HILITE);
        demo_hint(&demo);
    } else {
        viz_code_panel(20, 340, 310, push_code, 4, push_line >= 0 && push_line < 4 ? push_line : (push_line >= 4 ? 3 : -1));
        viz_text("так работает list_push_front", 20, 470, 16, COL_DIM);
        viz_hint("ЛКМ — push_front   ПКМ — list_remove   D на узле — пошаговое удаление   КОЛЁСИКО — тащить узел   C — очистить");
    }
    viz_message(message, message_color);
}

static void unload(void) { free_all(); }

const Scene scene_list = { "Список", init, update, draw, unload };
