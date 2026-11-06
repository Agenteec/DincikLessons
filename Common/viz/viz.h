// viz.h — рисование «памяти»: ячейки, стрелки, панели кода, текст с кириллицей.
#ifndef VIZ_H
#define VIZ_H

#include "screen.h"

// Темы оформления: Metro и Material Design. F2 переключает.
typedef struct {
    const char* name;
    Color bg, panel, cell, text, dim, addr, ptr, good, bad, hilite, accent;
    Color ptr_tile;     // плитка с указателем
    Color node, node_sub, node_freed;   // узлы дерева
    Color bar, bar_cmp; // столбики массива
    float radius;       // скругление плиток в пикселях (Metro — 0)
    bool  shadows;      // тени-«высота» у больших плиток (Material)
    bool  metro_text;   // заголовки строчными тонким шрифтом
    unsigned char hl_alpha;   // насколько плотная подсветка строки кода
} Theme;

extern Theme g_theme;
void viz_theme_next(void);

#define COL_BG      (g_theme.bg)
#define COL_PANEL   (g_theme.panel)
#define COL_CELL    (g_theme.cell)
#define COL_TEXT    (g_theme.text)
#define COL_DIM     (g_theme.dim)
#define COL_ADDR    (g_theme.addr)
#define COL_PTR     (g_theme.ptr)
#define COL_GOOD    (g_theme.good)
#define COL_BAD     (g_theme.bad)
#define COL_HILITE  (g_theme.hilite)
#define COL_ACCENT  (g_theme.accent)

void viz_load_fonts(void);
void viz_unload_fonts(void);

void    viz_text(const char* s, float x, float y, float size, Color c);
void    viz_code(const char* s, float x, float y, float size, Color c);
void    viz_light(const char* s, float x, float y, float size, Color c);   // тонкий шрифт для заголовков
Vector2 viz_measure_light(const char* s, float size);
const char* viz_lower(const char* s);   // строчные буквы (латиница и кириллица)
const char* viz_upper(const char* s);   // ПРОПИСНЫЕ
void    viz_heading(const char* s, float x, float y, float size, Color c);   // заголовок в стиле темы
Vector2 viz_measure_heading(const char* s, float size);
Color   viz_tint(Color c, unsigned char alpha);

// Плоские плитки вместо скруглённых прямоугольников.
void viz_fill(Rectangle r, Color c);
void viz_frame(Rectangle r, float thick, Color c);
Vector2 viz_measure(const char* s, float size);

// Короткий адрес: последние 4 шестнадцатеричные цифры, например "…A3F0".
const char* viz_addr(const void* p);

// Ячейка памяти: имя сверху, значение внутри, адрес снизу. Возвращает прямоугольник ячейки.
Rectangle viz_cell(float x, float y, float w, float h,
                   const char* name, const char* value, const void* addr, Color fill);

void viz_arrow(Vector2 from, Vector2 to, Color c);
void viz_code_panel(float x, float y, float w, const char** lines, int count, int highlight);
void viz_title(const char* title, const char* subtitle);
void viz_hint(const char* text);
void viz_message(const char* text, Color c);

// Поле-ползунок: зажми мышь и тяни влево-вправо или крути колесо над полем.
// Возвращает VIZ_DRAG_NONE, VIZ_DRAG_MOVING (тянут) или VIZ_DRAG_DONE (отпустили / повернули колесо).
enum { VIZ_DRAG_NONE, VIZ_DRAG_MOVING, VIZ_DRAG_DONE };
int  viz_drag_value(Rectangle r, float* v, float min, float max, float per_px, float wheel_step);

// ---------- Анимации ----------
float   viz_ease(float t);                                   // плавный разгон и торможение, 0..1
Vector2 viz_lerp2(Vector2 a, Vector2 b, float t);
float   viz_approach(float value, float target, float speed, float dt);

// Точка, которая плавно догоняет target. key + sub — кто это (обычно адрес поля).
Vector2 viz_smooth(const void* key, int sub, Vector2 target);
void    viz_glow(Rectangle r, float strength, Color c);      // вспышка вокруг ячейки

// Значение вылетает из одной ячейки в другую; по прилёте пишет value в *dst (если dst не NULL).
void viz_fly_start(const char* text, Vector2 from, Vector2 to, Color c, int* dst, int value);
bool viz_fly_busy(void);

void viz_anim_update(float dt);   // вызывает main каждый кадр
void viz_anim_draw(void);
void viz_anim_reset(void);        // мгновенно завершить все полёты (при смене сцены)
void viz_frame_end(void);

// ---------- Консоль кода справа: каждое действие печатает строку на C ----------
// code — что выполнилось, note — что получилось (можно NULL). Строки копируются.
void viz_log(const char* code, const char* note);
void viz_log_clear(void);
void viz_log_draw(float dt);

// ---------- Пошаговые демо: N — шаг, A — автоплей, X — выход ----------
typedef struct {
    bool  active;
    bool  autoplay;
    int   line;     // подсвеченная строка кода
    float timer;
    bool  fast;     // F — быстрый автоплей
} Demo;

void demo_start(Demo* d);
void demo_stop(Demo* d);
bool demo_next(Demo* d, float dt, bool busy);   // true — пора делать следующий шаг
void demo_hint(const Demo* d);

#endif
