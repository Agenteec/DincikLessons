// Сцена 7. Дерево: рекурсивное удаление ветки и стек вызовов.
// Здесь писать ничего не нужно: смотрим, угадываем порядок и ловим баг «free слишком рано».
#include "scenes.h"
#include "viz.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct TreeNode {
    int value;
    struct TreeNode* left;
    struct TreeNode* right;

    // только для картинки
    Vector2 target;   // где узел должен стоять
    bool    freed;    // в демо: free(n) уже был, но память ещё «висит» до конца демо
    int     guess;    // режим «угадай»: какой по счёту она его назвала (0 — не назван)
    int     verdict;  // 0 — ещё не освобождён, 1 — угадала, 2 — нет
} TreeNode;

#define MAX_DEPTH 5
#define GHOST_MAX 32
#define STACK_MAX 16
#define NODE_R 22.0f

static TreeNode* root;
static const char* message;
static char message_buf[200];
static Color message_color;

typedef struct { Vector2 pos; int value; float alpha; } Ghost;
static Ghost ghosts[GHOST_MAX];

static const Rectangle TREE_AREA = { 30, 130, 1150, 285 };
static const Rectangle STACK_BOX = { 610, 420, 560, 228 };

// ---------- пошаговое демо ----------
typedef struct {
    TreeNode* n;
    int       line;   // какая строка сейчас у этого кадра
    Vector2   slot;   // где рисовать «NULL», если n == NULL
    float     age;
} Frame;

static Demo      demo;
static bool      bad_version;      // W — показать неправильный порядок
static int       phase;            // 0 — вызов, 1 — рекурсия, 2 — *link = NULL, 3 — конец
static Frame     stack[STACK_MAX];
static int       sp;
static TreeNode* sub;              // корень удаляемой ветки
static TreeNode** sub_link;        // поле, которое на неё указывает
static int       freed_count;
static float     crash_flash;

// ---------- режим «угадай порядок» ----------
static bool predicting;     // сейчас она кликает узлы
static int  guesses, sub_size, right_guesses;

static const char* good_code[] = {
    "tree_free(sub);        // удаляем ветку",
    "void tree_free(TreeNode* n) {",
    "    if (n == NULL) return;",
    "    tree_free(n->left);",
    "    tree_free(n->right);",
    "    free(n);",
    "}",
    "*link = NULL;          // отцепили от родителя",
};

static const char* bad_code[] = {
    "tree_free_bad(sub);    // версия с багом",
    "void tree_free_bad(TreeNode* n) {",
    "    if (n == NULL) return;",
    "    free(n);",
    "    tree_free_bad(n->left);   // n уже free!",
    "    tree_free_bad(n->right);",
    "}",
    "*link = NULL;",
};

static void set_message(Color c, const char* fmt, int a, int b)
{
    snprintf(message_buf, sizeof message_buf, fmt, a, b);
    message = message_buf;
    message_color = c;
}

// ---------- обычные операции с деревом ----------

static int depth_of(const TreeNode* n, int value)
{
    int d = 0;
    while (n && n->value != value) { n = value < n->value ? n->left : n->right; d++; }
    return d;
}

static TreeNode* insert(TreeNode** link_to, int value, TreeNode** parent_out)
{
    TreeNode* parent = NULL;
    while (*link_to) {
        parent = *link_to;
        if (value == parent->value) return NULL;
        link_to = value < parent->value ? &parent->left : &parent->right;
    }
    TreeNode* n = calloc(1, sizeof *n);
    n->value = value;
    n->target = parent ? parent->target : (Vector2){ 600, 165 };
    *link_to = n;
    if (parent_out) *parent_out = parent;
    return n;
}

static void add_ghost(TreeNode* n)
{
    int slot = 0;
    for (int i = 0; i < GHOST_MAX; i++)
        if (ghosts[i].alpha < ghosts[slot].alpha) slot = i;
    ghosts[slot] = (Ghost){ viz_smooth(n, 0, n->target), n->value, 1.0f };
}

// настоящее рекурсивное удаление — то самое, что показывает демо
static int tree_free(TreeNode* n)
{
    if (n == NULL) return 0;
    int count = tree_free(n->left);
    count += tree_free(n->right);
    add_ghost(n);
    free(n);
    return count + 1;
}

static int tree_size(const TreeNode* n) { return n ? 1 + tree_size(n->left) + tree_size(n->right) : 0; }

static bool in_subtree(const TreeNode* top, const TreeNode* n)
{
    if (!top) return false;
    return top == n || in_subtree(top->left, n) || in_subtree(top->right, n);
}

static TreeNode** find_link(TreeNode** link_to, const TreeNode* n)
{
    if (!*link_to) return NULL;
    if (*link_to == n) return link_to;
    TreeNode** l = find_link(&(*link_to)->left, n);
    return l ? l : find_link(&(*link_to)->right, n);
}

static void clear_marks(TreeNode* n)
{
    if (!n) return;
    n->freed = false;
    n->guess = 0;
    n->verdict = 0;
    clear_marks(n->left);
    clear_marks(n->right);
}

// раскладка: x — порядок слева направо (in-order), y — глубина
static void layout_walk(TreeNode* n, int depth, int* index, float x0, float step)
{
    if (!n) return;
    layout_walk(n->left, depth + 1, index, x0, step);
    n->target = (Vector2){ x0 + (*index)++ * step, 165 + depth * 56.0f };
    layout_walk(n->right, depth + 1, index, x0, step);
}

static void layout(void)
{
    int count = tree_size(root);
    float step = count > 1 ? fminf(80.0f, 1080.0f / (count - 1)) : 0;
    int index = 0;
    layout_walk(root, 0, &index, 600 - step * (count - 1) / 2, step);
}

static void free_everything(void)
{
    // без призраков: просто освобождаем
    while (root) {
        TreeNode** l = &root;
        while ((*l)->left || (*l)->right) l = (*l)->left ? &(*l)->left : &(*l)->right;
        free(*l);
        *l = NULL;
    }
}

static void init(void)
{
    free_everything();
    int values[] = { 50, 30, 70, 20, 40, 60, 85, 10, 35, 45, 65, 90 };
    for (int i = 0; i < (int)(sizeof values / sizeof values[0]); i++) insert(&root, values[i], NULL);
    layout();
    for (int i = 0; i < GHOST_MAX; i++) ghosts[i].alpha = 0;
    demo_stop(&demo);
    predicting = false;
    sp = 0;
    message = "Наведи мышку на узел и нажми D: посмотришь, как рекурсия удаляет ветку.";
    message_color = COL_DIM;
}

// ---------- шаги демо ----------

static void push(TreeNode* n, Vector2 slot)
{
    if (sp < STACK_MAX) stack[sp++] = (Frame){ n, 2, slot, 0 };
}

static void ret(void)
{
    sp--;
    if (sp == 0) { phase = 2; return; }
    stack[sp - 1].line++;   // вернулись в родителя: он переходит к следующей строке
}

static Vector2 child_slot(const TreeNode* parent, bool left)
{
    return (Vector2){ parent->target.x + (left ? -26.0f : 26.0f), parent->target.y + 44 };
}

static void start_demo(TreeNode* target)
{
    demo_start(&demo);
    sub = target;
    sub_link = find_link(&root, target);
    phase = 0;
    sp = 0;
    freed_count = 0;
    message = "";
}

static void finish_crash(void)
{
    crash_flash = 1;
    clear_marks(root);
    sub = NULL;
    demo_stop(&demo);
    sp = 0;
    message = "БАГ: free(n) случился раньше, чем мы прочитали n->left. Это use-after-free, как корм в аквариуме!";
    message_color = COL_BAD;
}

static void demo_step(void)
{
    if (phase == 0) {
        viz_log(bad_version ? "tree_free_bad(sub);" : "tree_free(sub);",
                TextFormat("n = %s (узел %d)", viz_addr(sub), sub->value));
        push(sub, sub->target);
        phase = 1;
        return;
    }
    if (phase == 2) {
        int n = tree_free(sub);   // по-настоящему освобождаем отмеченные узлы
        *sub_link = NULL;
        sub = NULL;
        layout();
        viz_log("*link = NULL;", "родитель больше не смотрит на ветку");
        if (guesses > 0) {
            set_message(right_guesses == n ? COL_GOOD : COL_HILITE,
                        "Ветка удалена. Угадано на своём месте: %d из %d.", right_guesses, n);
            guesses = 0;
        } else {
            set_message(COL_GOOD, "Ветка удалена: %d узлов. Сначала дети, потом родитель — иначе нельзя.", n, 0);
        }
        phase = 3;
        return;
    }
    if (phase == 3) { demo_stop(&demo); return; }

    Frame* f = &stack[sp - 1];
    TreeNode* n = f->n;
    int free_line = bad_version ? 3 : 5;

    if (f->line == 2) {
        if (!n) {
            viz_log("if (n == NULL) return;", "n == NULL — сразу назад");
            ret();
        } else {
            viz_log("if (n == NULL) return;", TextFormat("n = узел %d, идём дальше", n->value));
            f->line = 3;
        }
    } else if (f->line == free_line) {
        n->freed = true;
        freed_count++;
        if (n->guess) {
            n->verdict = n->guess == freed_count ? 1 : 2;
            if (n->verdict == 1) right_guesses++;
        }
        viz_log("free(n);", TextFormat("узел %d по %s освобождён", n->value, viz_addr(n)));
        f->line++;
    } else if (bad_version && f->line == 4) {
        viz_log("tree_free_bad(n->left);", "n->left из free-памяти = 0xDDDDDDDD!");
        finish_crash();
    } else if (f->line == 3 || f->line == 4) {
        bool left = f->line == 3;
        TreeNode* child = left ? n->left : n->right;
        viz_log(left ? "tree_free(n->left);" : "tree_free(n->right);",
                child ? TextFormat("новый кадр: n = узел %d", child->value) : "новый кадр: n = NULL");
        push(child, child ? child->target : child_slot(n, left));
    } else {   // строка 6: "}"
        viz_log("}", TextFormat("кадр узла %d снят со стека", n->value));
        ret();
    }
}

// ---------- update ----------

static TreeNode* hit_walk(TreeNode* n, Vector2 m)
{
    if (!n) return NULL;
    if (CheckCollisionPointCircle(m, n->target, NODE_R + 4)) return n;
    TreeNode* l = hit_walk(n->left, m);
    return l ? l : hit_walk(n->right, m);
}

static void update(float dt)
{
    crash_flash = viz_approach(crash_flash, 0, 1.5f, dt);
    for (int i = 0; i < GHOST_MAX; i++) {
        ghosts[i].alpha = viz_approach(ghosts[i].alpha, 0, 1.2f, dt);
        ghosts[i].pos.y += 40 * dt;
    }
    for (int i = 0; i < sp; i++) stack[i].age += dt;

    if (demo.active) {
        if (demo_next(&demo, dt, false)) demo_step();
        if (!demo.active && phase < 2 && sub) {   // X посреди демо — ничего не освобождали
            clear_marks(root);
            sp = 0;
            guesses = 0;
            message = "Демо прервано, дерево не тронуто.";
            message_color = COL_DIM;
        }
        return;
    }

    Vector2 m = GetMousePosition();
    TreeNode* hovered = hit_walk(root, m);

    if (predicting) {
        if (IsKeyPressed(KEY_X)) { predicting = false; clear_marks(root); message = ""; return; }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hovered && in_subtree(sub, hovered) && !hovered->guess) {
            hovered->guess = ++guesses;
            if (guesses == sub_size) {
                predicting = false;
                right_guesses = 0;
                TreeNode* s = sub;
                start_demo(s);
                demo.autoplay = true;
                message = "Смотрим, совпадёт ли твой порядок с тем, что делает рекурсия...";
                message_color = COL_HILITE;
            }
        }
        return;
    }

    if (IsKeyPressed(KEY_W)) {
        bad_version = !bad_version;
        message = bad_version ? "Включена версия с багом: free(n) стоит ПЕРВЫМ. Нажми D на узле."
                              : "Включена правильная версия: сначала дети, потом free(n).";
        message_color = bad_version ? COL_BAD : COL_GOOD;
    }

    if (hovered && IsKeyPressed(KEY_D)) {
        clear_marks(root);
        guesses = 0;
        start_demo(hovered);
    }

    if (hovered && IsKeyPressed(KEY_P)) {
        bad_version = false;
        clear_marks(root);
        sub = hovered;
        sub_size = tree_size(hovered);
        guesses = 0;
        predicting = true;
        set_message(COL_HILITE, "Кликай узлы ветки в том порядке, в котором их освободит tree_free (%d шт.). X — отмена.",
                    sub_size, 0);
    }

    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        TreeNode** l = find_link(&root, hovered);
        int value = hovered->value;
        int n = tree_free(hovered);
        *l = NULL;
        layout();
        viz_log("tree_free(sub); *link = NULL;", TextFormat("ветка узла %d: освобождено %d", value, n));
        set_message(COL_GOOD, "Ветка узла %d удалена: %d узлов. D на узле — то же самое по шагам.", value, n);
    }

    if (!hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(m, TREE_AREA)) {
        int value = 0;
        for (int tries = 0; tries < 50 && !value; tries++) {
            int v = GetRandomValue(1, 99);
            if (depth_of(root, v) >= MAX_DEPTH) continue;   // слишком глубоко — не влезет на экран
            TreeNode* n = insert(&root, v, NULL);
            if (!n) continue;                                // такое число уже есть
            value = v;
            viz_smooth(n, 0, n->target);   // новый узел вылетает из родителя
            layout();
            viz_log(TextFormat("tree_insert(&root, %d);", v),
                    TextFormat("узел %d по %s, глубина %d", v, viz_addr(n), depth_of(root, v)));
        }
        if (!value) set_message(COL_DIM, "Не нашлось места: дерево слишком глубокое.", 0, 0);
    }

    if (IsKeyPressed(KEY_R)) { init(); viz_log("// новое дерево", NULL); }
}

// ---------- draw ----------

static const TreeNode* current_node(void)
{
    return (demo.active && phase == 1 && sp > 0) ? stack[sp - 1].n : NULL;
}

static void draw_links(const TreeNode* n)
{
    if (!n) return;
    Vector2 p = viz_smooth(n, 0, n->target);
    const TreeNode* kids[2] = { n->left, n->right };
    for (int i = 0; i < 2; i++) {
        if (!kids[i]) continue;
        Vector2 c = viz_smooth(kids[i], 0, kids[i]->target);
        // стрелка на освобождённый узел — висячая, красная
        Color col = kids[i]->freed ? COL_BAD : COL_PTR;
        float dx = c.x - p.x, dy = c.y - p.y, len = sqrtf(dx * dx + dy * dy);
        if (len > 2 * NODE_R)
            viz_arrow((Vector2){ p.x + dx / len * NODE_R, p.y + dy / len * NODE_R },
                      (Vector2){ c.x - dx / len * NODE_R, c.y - dy / len * NODE_R }, col);
        draw_links(kids[i]);
    }
}

static void draw_nodes(const TreeNode* n, const TreeNode* hovered)
{
    if (!n) return;
    draw_nodes(n->left, hovered);
    draw_nodes(n->right, hovered);

    Vector2 p = viz_smooth(n, 0, n->target);
    bool in_sub = (demo.active || predicting) && in_subtree(sub, n);
    Color fill = n->freed ? g_theme.node_freed : (in_sub ? g_theme.node_sub : g_theme.node);
    Rectangle tile = { p.x - NODE_R, p.y - NODE_R, 2 * NODE_R, 2 * NODE_R };
    viz_fill(tile, fill);
    if (n == hovered) viz_frame(tile, 2, COL_TEXT);
    const char* s = TextFormat("%d", n->value);
    Vector2 sz = viz_measure(s, 20);
    viz_code(s, p.x - sz.x / 2 + 1, p.y - 10, 20, COL_TEXT);
    if (n->freed) viz_code("free", p.x - 18, p.y + NODE_R + 2, 15, COL_BAD);

    if (n == current_node()) {
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 8);
        float g = NODE_R + 5 + 2 * pulse;
        viz_frame((Rectangle){ p.x - g, p.y - g, 2 * g, 2 * g }, 3, COL_HILITE);
    }
    if (n->guess) {
        Color badge = n->verdict == 1 ? COL_GOOD : n->verdict == 2 ? COL_BAD : COL_PTR;
        Vector2 b = { p.x + NODE_R, p.y - NODE_R };
        viz_fill((Rectangle){ b.x - 11, b.y - 11, 22, 22 }, badge);
        const char* g = TextFormat("%d", n->guess);
        viz_code(g, b.x - viz_measure(g, 15).x / 2, b.y - 8, 15, COL_BG);
    }
}

static void draw_stack(void)
{
    Rectangle r = STACK_BOX;
    viz_fill(r, COL_PANEL);
    viz_text("Стек вызовов: у каждого вызова свой кадр со своим n", r.x + 12, r.y + 8, 17, COL_DIM);
    if (sp == 0) {
        viz_text(phase >= 2 ? "Стек пуст: все вызовы вернулись." : "Пока пусто.", r.x + 12, r.y + 40, 18, COL_DIM);
        return;
    }
    const char* fname = bad_version ? "tree_free_bad" : "tree_free";
    for (int i = 0; i < sp; i++) {
        const Frame* f = &stack[i];
        float t = viz_ease(f->age / 0.25f);
        float y = r.y + r.height - 12 - (i + 1) * 24.0f;
        float x = r.x + 12 + (1 - t) * 60;
        bool top = i == sp - 1;
        if (top) viz_fill((Rectangle){ r.x + 6, y - 2, r.width - 12, 24 }, viz_tint(COL_ACCENT, g_theme.hl_alpha));
        Color c = COL_TEXT;
        c.a = (unsigned char)(255 * t);
        const char* who = f->n ? TextFormat("узел %d", f->n->value) : "NULL";
        viz_code(TextFormat("%s(n = %s)", fname, who), x, y, 18, c);
        viz_code(TextFormat("строка %d", f->line - 1), r.x + r.width - 100, y, 16, top ? (Color){ 255, 255, 255, c.a } : (Color){ 160, 160, 160, c.a });
    }
}

static void draw(void)
{
    viz_title("Дерево и рекурсия",
              "У узла два ребёнка. Чтобы удалить ветку, функция вызывает сама себя для детей, а потом освобождает узел.");

    if (crash_flash > 0.01f)
        DrawRectangle(0, WORLD_TOP - 10, CODE_X, SCREEN_H - WORLD_TOP - 24, (Color){ 255, 60, 70, (unsigned char)(90 * crash_flash) });

    Vector2 m = GetMousePosition();
    const TreeNode* hovered = demo.active ? NULL : hit_walk(root, m);

    draw_links(root);
    draw_nodes(root, hovered);

    for (int i = 0; i < GHOST_MAX; i++) {
        if (ghosts[i].alpha < 0.02f) continue;
        Color c = { 255, 70, 50, (unsigned char)(255 * ghosts[i].alpha) };
        viz_frame((Rectangle){ ghosts[i].pos.x - NODE_R, ghosts[i].pos.y - NODE_R, 2 * NODE_R, 2 * NODE_R }, 2, c);
        viz_code(TextFormat("%d", ghosts[i].value), ghosts[i].pos.x - 11, ghosts[i].pos.y - 10, 20, c);
    }

    // NULL-кадр: показываем, куда «смотрит» пустой ребёнок
    if (demo.active && phase == 1 && sp > 0 && !stack[sp - 1].n) {
        Vector2 s = viz_smooth(&stack[sp - 1], 0, stack[sp - 1].slot);
        viz_frame((Rectangle){ s.x - 12, s.y - 12, 24, 24 }, 2, COL_HILITE);
        viz_code("NULL", s.x - 20, s.y + 14, 16, COL_HILITE);
    }

    const char** code = bad_version ? bad_code : good_code;
    int hl = !demo.active ? -1 : phase == 0 ? 0 : phase >= 2 ? 7 : stack[sp - 1].line;
    viz_code_panel(30, 420, 560, code, 8, hl);
    draw_stack();

    if (hovered && !predicting) {
        const char* tip = TextFormat("узел %d: адрес %s  left = %s  right = %s", hovered->value, viz_addr(hovered),
                                     viz_addr(hovered->left), viz_addr(hovered->right));
        Vector2 tsz = viz_measure(tip, 16);
        float tx = fminf(m.x + 16, CODE_X - tsz.x - 20);
        viz_fill((Rectangle){ tx - 6, m.y + 18, tsz.x + 12, 26 }, COL_PANEL);
        viz_text(tip, tx, m.y + 22, 16, COL_ADDR);
    }

    viz_message(message, message_color);
    if (demo.active) demo_hint(&demo);
    else if (predicting) viz_hint("ЛКМ по узлам ветки — твой порядок освобождения     X — отмена");
    else viz_hint(bad_version
        ? "D на узле — демо С БАГОМ     W — вернуть правильную версию     ЛКМ — добавить узел     R — новое дерево"
        : "D на узле — демо     P на узле — угадай порядок     W — версия с багом     ЛКМ — добавить     ПКМ — удалить ветку     R — сброс");
}

static void unload(void) { free_everything(); }

const Scene scene_tree = { "Дерево", init, update, draw, unload };
