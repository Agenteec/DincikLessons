#include "viz.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>

// Шрифты растеризуются под КАЖДЫЙ нужный размер отдельно и рисуются по целым пикселям:
// буква из атласа ложится на экран один к одному, без масштабирования — поэтому текст чёткий.
enum { FAM_UI, FAM_MONO, FAM_LIGHT, FAM_SEMIBOLD, FAM_COUNT };
#define FONT_PX_MIN 8
#define FONT_PX_MAX 96

static const char* FONT_PATHS[FAM_COUNT][3] = {
    { "C:/Windows/Fonts/segoeui.ttf",      "C:/Windows/Fonts/arial.ttf",    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" },
    { "C:/Windows/Fonts/CascadiaCode.ttf", "C:/Windows/Fonts/consola.ttf",  "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf" },
    { "C:/Windows/Fonts/segoeuil.ttf",     "C:/Windows/Fonts/segoeui.ttf",  "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" },
    { "C:/Windows/Fonts/seguisb.ttf",      "C:/Windows/Fonts/segoeuib.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" },
};
static Font g_cache[FAM_COUNT][FONT_PX_MAX + 1];
static bool g_cache_own[FAM_COUNT][FONT_PX_MAX + 1];

Theme g_theme;

static const Theme THEMES[] = {
    {   "Metro",
        { 29, 29, 29, 255 }, { 43, 43, 43, 255 }, { 60, 60, 60, 255 }, { 255, 255, 255, 255 }, { 185, 185, 185, 255 },
        { 90, 195, 245, 255 }, { 240, 150, 9, 255 }, { 140, 191, 38, 255 }, { 255, 70, 50, 255 }, { 255, 196, 13, 255 },
        { 27, 161, 226, 255 },
        { 196, 100, 0, 255 },
        { 0, 90, 160, 255 }, { 130, 0, 200, 255 }, { 162, 0, 37, 255 },
        { 27, 161, 226, 255 }, { 140, 220, 255, 255 },
        0, false, true, 255 },
    {   "Material",
        { 18, 18, 18, 255 }, { 37, 37, 38, 255 }, { 50, 50, 52, 255 }, { 236, 236, 236, 255 }, { 180, 180, 180, 255 },
        { 128, 203, 196, 255 }, { 255, 183, 77, 255 }, { 129, 199, 132, 255 }, { 207, 102, 121, 255 }, { 255, 213, 79, 255 },
        { 187, 134, 252, 255 },
        { 230, 120, 30, 255 },
        { 55, 71, 79, 255 }, { 98, 0, 238, 255 }, { 176, 0, 32, 255 },
        { 3, 218, 198, 255 }, { 187, 134, 252, 255 },
        10, true, false, 70 },
};
static int g_theme_index = 1;   // по умолчанию — Material

void viz_theme_next(void)
{
    g_theme_index = (g_theme_index + 1) % (int)(sizeof THEMES / sizeof THEMES[0]);
    g_theme = THEMES[g_theme_index];
}

Color viz_tint(Color c, unsigned char alpha)
{
    c.a = (unsigned char)(c.a * alpha / 255);
    return c;
}
static bool g_own_light;
static bool g_own_font;
static bool g_own_mono;

// Латиница, кириллица и несколько символов (стрелки, тире, кавычки).
static int* font_codepoints(int* count)
{
    static int cps[256];
    int n = 0;
    for (int c = 32; c < 127; c++)     cps[n++] = c;
    for (int c = 0x400; c < 0x460; c++) cps[n++] = c;

    int extra[] = { 0x2190, 0x2191, 0x2192, 0x2193, 0x2014, 0x2013, 0xAB, 0xBB, 0x2026, 0xB7, 0xD7, 0x2116, 0x25CF };
    for (int i = 0; i < (int)(sizeof extra / sizeof extra[0]); i++) cps[n++] = extra[i];

    *count = n;
    return cps;
}

static int font_px_of(float size)
{
    int px = (int)(size + 0.5f);
    return px < FONT_PX_MIN ? FONT_PX_MIN : (px > FONT_PX_MAX ? FONT_PX_MAX : px);
}

static Font font_get(int fam, int px)
{
    Font* f = &g_cache[fam][px];
    if (f->texture.id != 0) return *f;
    int n;
    int* cps = font_codepoints(&n);
    for (int i = 0; i < 3; i++) {
        if (!FileExists(FONT_PATHS[fam][i])) continue;
        Font loaded = LoadFontEx(FONT_PATHS[fam][i], px, cps, n);
        if (loaded.texture.id != 0) {
            SetTextureFilter(loaded.texture, TEXTURE_FILTER_BILINEAR);
            *f = loaded;
            g_cache_own[fam][px] = true;
            return *f;
        }
    }
    *f = GetFontDefault();
    return *f;
}

// Мелкий текст делаем чуть крупнее.
static float text_size(float size) { return size < 18 ? size + 2 : size; }

static void draw_font(int fam, const char* s, float x, float y, float size, Color c)
{
    int px = font_px_of(size);
    DrawTextEx(font_get(fam, px), s, (Vector2){ floorf(x + 0.5f), floorf(y + 0.5f) }, (float)px, 0.0f, c);
}

static Vector2 measure_font(int fam, const char* s, float size)
{
    int px = font_px_of(size);
    return MeasureTextEx(font_get(fam, px), s, (float)px, 0.0f);
}

void viz_load_fonts(void)
{
    g_theme = THEMES[g_theme_index];
    // самые частые размеры загружаем сразу, остальные — при первом использовании
    int sizes[] = { 17, 18, 19, 20, 22 };
    for (int i = 0; i < 5; i++) { font_get(FAM_UI, sizes[i]); font_get(FAM_MONO, sizes[i]); }
}

void viz_unload_fonts(void)
{
    for (int f = 0; f < FAM_COUNT; f++)
        for (int px = 0; px <= FONT_PX_MAX; px++)
            if (g_cache_own[f][px]) { UnloadFont(g_cache[f][px]); g_cache_own[f][px] = false; g_cache[f][px] = (Font){ 0 }; }
}

void viz_heading(const char* s, float x, float y, float size, Color c)
{
    if (g_theme.metro_text) draw_font(FAM_LIGHT, viz_lower(s), x, y, size, c);
    else                    draw_font(FAM_SEMIBOLD, s, x, y + size * 0.12f, size * 0.8f, c);
}

Vector2 viz_measure_heading(const char* s, float size)
{
    if (g_theme.metro_text) return measure_font(FAM_LIGHT, viz_lower(s), size);
    return measure_font(FAM_SEMIBOLD, s, size * 0.8f);
}

const char* viz_upper(const char* s)
{
    static char buf[4][256];
    static int k = 0;
    k = (k + 1) % 4;
    unsigned char* o = (unsigned char*)buf[k];
    const unsigned char* in = (const unsigned char*)s;
    int n = 0;
    while (*in && n < 254) {
        unsigned char a = *in++;
        if (a >= 'a' && a <= 'z') { o[n++] = (unsigned char)(a - 32); continue; }
        if (a == 0xD0 && *in) {             // а..п -> А..П
            unsigned char b = *in++;
            o[n++] = 0xD0;
            o[n++] = (b >= 0xB0 && b <= 0xBF) ? (unsigned char)(b - 0x20) : b;
            continue;
        }
        if (a == 0xD1 && *in) {             // р..я -> Р..Я, ё -> Ё
            unsigned char b = *in++;
            if (b >= 0x80 && b <= 0x8F) { o[n++] = 0xD0; o[n++] = (unsigned char)(b + 0x20); }
            else if (b == 0x91)         { o[n++] = 0xD0; o[n++] = 0x81; }
            else                        { o[n++] = a; o[n++] = b; }
            continue;
        }
        o[n++] = a;
    }
    o[n] = 0;
    return buf[k];
}

void viz_light(const char* s, float x, float y, float size, Color c)
{
    draw_font(FAM_LIGHT, s, x, y, size, c);
}

Vector2 viz_measure_light(const char* s, float size)
{
    return measure_font(FAM_LIGHT, s, size);
}

const char* viz_lower(const char* s)
{
    static char buf[4][256];
    static int k = 0;
    k = (k + 1) % 4;
    unsigned char* o = (unsigned char*)buf[k];
    const unsigned char* in = (const unsigned char*)s;
    int n = 0;
    while (*in && n < 254) {
        unsigned char a = *in++;
        if (a >= 'A' && a <= 'Z') { o[n++] = (unsigned char)(a + 32); continue; }
        if (a == 0xD0 && *in) {             // кириллица U+0400..U+043F
            unsigned char b = *in++;
            if (b >= 0x90 && b <= 0x9F)      { o[n++] = 0xD0; o[n++] = (unsigned char)(b + 0x20); }   // А..П -> а..п
            else if (b >= 0xA0 && b <= 0xAF) { o[n++] = 0xD1; o[n++] = (unsigned char)(b - 0x20); }   // Р..Я -> р..я
            else if (b == 0x81)              { o[n++] = 0xD1; o[n++] = 0x91; }                        // Ё -> ё
            else                             { o[n++] = a; o[n++] = b; }
            continue;
        }
        o[n++] = a;
    }
    o[n] = 0;
    return buf[k];
}

static float roundness(Rectangle r)
{
    float m = r.width < r.height ? r.width : r.height;
    if (m <= 0) return 0;
    float k = 2 * g_theme.radius / m;
    return k > 1 ? 1 : k;
}

void viz_fill(Rectangle r, Color c)
{
    if (g_theme.radius <= 0) { DrawRectangleRec(r, c); return; }
    // Material: у больших плиток мягкая тень снизу — «приподнятая карточка»
    if (g_theme.shadows && r.width >= 40 && r.height >= 40 && c.a > 0) {
        unsigned char a = (unsigned char)(c.a / 4);
        DrawRectangleRounded((Rectangle){ r.x, r.y + 4, r.width, r.height }, roundness(r), 8, (Color){ 0, 0, 0, a });
        DrawRectangleRounded((Rectangle){ r.x - 1, r.y + 2, r.width + 2, r.height + 1 }, roundness(r), 8, (Color){ 0, 0, 0, a });
    }
    DrawRectangleRounded(r, roundness(r), 8, c);
}

void viz_frame(Rectangle r, float thick, Color c)
{
    if (g_theme.radius <= 0) DrawRectangleLinesEx(r, thick, c);
    else                     DrawRectangleRoundedLinesEx(r, roundness(r), 8, thick, c);
}

void viz_text(const char* s, float x, float y, float size, Color c)
{
    draw_font(FAM_UI, s, x, y, text_size(size), c);
}

void viz_code(const char* s, float x, float y, float size, Color c)
{
    draw_font(FAM_MONO, s, x, y, text_size(size), c);
}

Vector2 viz_measure(const char* s, float size)
{
    return measure_font(FAM_UI, s, text_size(size));
}

const char* viz_addr(const void* p)
{
    // static-массив буферов: функцию можно вызвать несколько раз в одной строке
    static char buf[8][16];
    static int i = 0;
    i = (i + 1) % 8;

    if (!p) snprintf(buf[i], sizeof buf[i], "NULL");
    else    snprintf(buf[i], sizeof buf[i], "\xE2\x80\xA6%04X", (unsigned)((uintptr_t)p & 0xFFFF));
    return buf[i];
}

Rectangle viz_cell(float x, float y, float w, float h,
                   const char* name, const char* value, const void* addr, Color fill)
{
    Rectangle r = { x, y, w, h };
    viz_fill(r, fill);

    if (name)  viz_code(name, x, y - 24, 20, COL_DIM);
    if (value) {
        float size = h > 50 ? 30.0f : 20.0f;
        Vector2 t = measure_font(FAM_MONO, value, size);
        viz_code(value, x + (w - t.x) / 2, y + (h - size) / 2, size, COL_TEXT);
    }
    if (addr)  viz_code(viz_addr(addr), x, y + h + 4, 16, COL_ADDR);
    return r;
}

void viz_arrow(Vector2 from, Vector2 to, Color c)
{
    float dx = to.x - from.x, dy = to.y - from.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;
    float ux = dx / len, uy = dy / len;

    Vector2 base = { to.x - ux * 14, to.y - uy * 14 };
    DrawLineEx(from, base, 3.0f, c);
    DrawCircleV(from, 5.0f, c);

    Vector2 left  = { base.x - uy * 8, base.y + ux * 8 };
    Vector2 right = { base.x + uy * 8, base.y - ux * 8 };
    DrawTriangle(to, left, right, c);
    DrawTriangle(to, right, left, c);   // одна из двух всегда в правильном порядке обхода
}

void viz_code_panel(float x, float y, float w, const char** lines, int count, int highlight)
{
    const float line_h = 26;
    viz_fill((Rectangle){ x, y, w, count * line_h + 20 }, COL_PANEL);
    if (g_theme.radius <= 0) DrawRectangle((int)x, (int)y, 4, (int)(count * line_h + 20), COL_ACCENT);

    // подсветка — плитка акцентного цвета, плавно переезжает на новую строку
    if (highlight >= 0 && highlight < count) {
        Vector2 hl = viz_smooth(lines, 0, (Vector2){ x + 4, y + 8 + highlight * line_h });
        viz_fill((Rectangle){ hl.x + (g_theme.radius > 0 ? 4 : 0), hl.y, w - (g_theme.radius > 0 ? 12 : 4), line_h },
                 viz_tint(COL_ACCENT, g_theme.hl_alpha));
    }
    for (int i = 0; i < count; i++)
        viz_code(lines[i], x + 16, y + 10 + i * line_h, 19, i == highlight ? (g_theme.metro_text ? COL_TEXT : COL_ACCENT) : viz_tint(COL_TEXT, 215));
}

void viz_title(const char* title, const char* subtitle)
{
    viz_heading(title, 27, WORLD_TOP - 6, 42, COL_TEXT);
    if (subtitle) viz_text(subtitle, 30, WORLD_TOP + 44, 18, COL_DIM);
}

void viz_hint(const char* text)
{
    if (g_theme.shadows) DrawRectangleGradientV(0, SCREEN_H - 40, SCREEN_W, 6, (Color){ 0, 0, 0, 0 }, (Color){ 0, 0, 0, 90 });
    DrawRectangle(0, SCREEN_H - 34, SCREEN_W, 34, g_theme.shadows ? COL_PANEL : (Color){ 31, 31, 31, 255 });
    if (!g_theme.shadows) DrawRectangle(0, SCREEN_H - 34, SCREEN_W, 1, (Color){ 55, 55, 55, 255 });
    viz_text(g_theme.metro_text ? viz_lower(text) : text, 16, SCREEN_H - 28, 18, COL_DIM);
}

static int g_cursor_want, g_cursor_now;   // курсор меняем один раз за кадр, в viz_frame_end

int viz_drag_value(Rectangle r, float* v, float min, float max, float per_px, float wheel_step)
{
    static const float* active;   // какое значение сейчас тянут
    static float start_x, start_v;
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);

    // подложка: заполненная часть показывает, где значение между min и max
    float k = (*v - min) / (max - min);
    k = k < 0 ? 0 : (k > 1 ? 1 : k);
    viz_fill(r, COL_CELL);
    viz_fill((Rectangle){ r.x, r.y, r.width * k, r.height }, viz_tint(COL_ACCENT, active == v ? 120 : (hover ? 80 : 50)));
    if (hover || active == v) viz_frame(r, 1.5f, COL_ACCENT);

    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { active = v; start_x = m.x; start_v = *v; }
    if (active == v) {
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { active = NULL; return VIZ_DRAG_DONE; }
        float nv = start_v + (m.x - start_x) * per_px;
        *v = nv < min ? min : (nv > max ? max : nv);
        g_cursor_want = MOUSE_CURSOR_RESIZE_EW;
        return VIZ_DRAG_MOVING;
    }
    if (hover) {
        g_cursor_want = MOUSE_CURSOR_RESIZE_EW;
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            float nv = *v + wheel * wheel_step;
            *v = nv < min ? min : (nv > max ? max : nv);
            return VIZ_DRAG_DONE;
        }
    }
    return VIZ_DRAG_NONE;
}

void viz_message(const char* text, Color c)
{
    if (!text || !text[0]) return;
    viz_text(text, 30, SCREEN_H - 70, 21, c);
}

// ---------------------------------------------------------------------
// Анимации
// ---------------------------------------------------------------------

float viz_ease(float t)
{
    if (t <= 0) return 0;
    if (t >= 1) return 1;
    return t * t * (3 - 2 * t);
}

Vector2 viz_lerp2(Vector2 a, Vector2 b, float t)
{
    return (Vector2){ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

float viz_approach(float value, float target, float speed, float dt)
{
    return value + (target - value) * (1.0f - expf(-speed * dt));
}

// Плавные точки: по ключу (обычно адрес) помним, где точка была в прошлом кадре.
#define SMOOTH_MAX 256

typedef struct {
    const void* key;
    int         sub;
    Vector2     value;
    bool        touched;
} SmoothEntry;

static SmoothEntry g_smooth[SMOOTH_MAX];
static int g_smooth_count;

Vector2 viz_smooth(const void* key, int sub, Vector2 target)
{
    for (int i = 0; i < g_smooth_count; i++) {
        SmoothEntry* e = &g_smooth[i];
        if (e->key == key && e->sub == sub) {
            float k = 1.0f - expf(-12.0f * GetFrameTime());
            e->value = viz_lerp2(e->value, target, k);
            e->touched = true;
            return e->value;
        }
    }
    if (g_smooth_count < SMOOTH_MAX)
        g_smooth[g_smooth_count++] = (SmoothEntry){ key, sub, target, true };
    return target;
}

static void smooth_collect(void)
{
    int n = 0;
    for (int i = 0; i < g_smooth_count; i++) {
        if (!g_smooth[i].touched) continue;
        g_smooth[i].touched = false;
        g_smooth[n++] = g_smooth[i];
    }
    g_smooth_count = n;
}

void viz_glow(Rectangle r, float strength, Color c)
{
    if (strength <= 0.01f) return;
    float pad = 4 + 8 * strength;
    c.a = (unsigned char)(220 * strength);
    viz_frame((Rectangle){ r.x - pad, r.y - pad, r.width + 2 * pad, r.height + 2 * pad }, 3.0f, c);
}

// Летящие значения: число вылетает из одной ячейки и по прилёте записывается в *dst.
#define FLY_MAX 16

typedef struct {
    bool    active;
    char    text[24];
    Vector2 from, to;
    float   t;
    Color   color;
    int*    dst;
    int     value;
} Fly;

static Fly g_flies[FLY_MAX];

void viz_fly_start(const char* text, Vector2 from, Vector2 to, Color c, int* dst, int value)
{
    for (int i = 0; i < FLY_MAX; i++) {
        if (g_flies[i].active) continue;
        Fly* f = &g_flies[i];
        *f = (Fly){ true, "", from, to, 0.0f, c, dst, value };
        snprintf(f->text, sizeof f->text, "%s", text);
        return;
    }
    if (dst) *dst = value;   // мест нет — просто записываем сразу
}

bool viz_fly_busy(void)
{
    for (int i = 0; i < FLY_MAX; i++)
        if (g_flies[i].active) return true;
    return false;
}

static void fly_finish(Fly* f)
{
    if (f->dst) *f->dst = f->value;
    f->active = false;
}

void viz_anim_update(float dt)
{
    for (int i = 0; i < FLY_MAX; i++) {
        Fly* f = &g_flies[i];
        if (!f->active) continue;
        f->t += dt / 0.7f;
        if (f->t >= 1.0f) fly_finish(f);
    }
}

void viz_anim_draw(void)
{
    for (int i = 0; i < FLY_MAX; i++) {
        const Fly* f = &g_flies[i];
        if (!f->active) continue;
        float e = viz_ease(f->t);
        Vector2 p = viz_lerp2(f->from, f->to, e);
        p.y -= sinf(e * PI) * 70.0f;   // летит дугой

        Vector2 m = measure_font(FAM_MONO, f->text, 28);
        Rectangle r = { p.x - m.x / 2 - 10, p.y - 20, m.x + 20, 40 };
        viz_fill(r, f->color);
        viz_code(f->text, r.x + 10, r.y + 6, 28, COL_BG);
    }
}

void viz_anim_reset(void)
{
    for (int i = 0; i < FLY_MAX; i++)
        if (g_flies[i].active) fly_finish(&g_flies[i]);
}

void viz_frame_end(void)
{
    if (g_cursor_want != g_cursor_now) { SetMouseCursor(g_cursor_want); g_cursor_now = g_cursor_want; }
    g_cursor_want = MOUSE_CURSOR_DEFAULT;
    smooth_collect();
}

// ---------------------------------------------------------------------
// Консоль кода
// ---------------------------------------------------------------------

#define LOG_MAX 40
typedef struct { char code[96]; char note[96]; float age; } LogEntry;
static LogEntry g_log[LOG_MAX];
static int g_log_count;   // сколько всего записей (кольцо по LOG_MAX)

void viz_log(const char* code, const char* note)
{
    LogEntry* e = &g_log[g_log_count % LOG_MAX];
    snprintf(e->code, sizeof e->code, "%s", code ? code : "");
    snprintf(e->note, sizeof e->note, "%s", note ? note : "");
    e->age = 0;
    g_log_count++;
}

void viz_log_clear(void) { g_log_count = 0; }

// Длинная строка не влезает в консоль — уменьшаем шрифт, но не мельче 12.
static float fit_size(const char* s, float size, float max_w)
{
    float w = measure_font(FAM_MONO, s, text_size(size)).x;
    if (w <= max_w) return size;
    float k = size * max_w / w;
    return k < 12 ? 12 : k;
}

void viz_log_draw(float dt)
{
    const float x = CODE_X, top = WORLD_TOP - 10, w = SCREEN_W - CODE_X;
    const float bottom = SCREEN_H - 34;
    if (g_theme.shadows) DrawRectangleGradientH((int)x - 8, (int)top, 8, (int)(bottom - top), (Color){ 0, 0, 0, 0 }, (Color){ 0, 0, 0, 100 });
    DrawRectangle((int)x, (int)top, (int)w, (int)(bottom - top), g_theme.shadows ? (Color){ 30, 30, 31, 255 } : (Color){ 22, 22, 22, 255 });
    viz_heading("Код твоих действий", x + 16, top + 2, 30, COL_TEXT);
    viz_text("каждое нажатие = строка на C", x + 18, top + 38, 15, COL_DIM);

    if (g_log_count == 0) {
        viz_text("Нажми что-нибудь в сцене —", x + 16, top + 90, 17, COL_DIM);
        viz_text("здесь появится код,", x + 16, top + 112, 17, COL_DIM);
        viz_text("который это делает.", x + 16, top + 134, 17, COL_DIM);
        return;
    }

    // снизу вверх: самая новая запись внизу
    float y = bottom - 12;
    int first = g_log_count > LOG_MAX ? g_log_count - LOG_MAX : 0;
    for (int i = g_log_count - 1; i >= first; i--) {
        LogEntry* e = &g_log[i % LOG_MAX];
        e->age += (i == g_log_count - 1) ? dt : 0;   // анимируется только последняя
        bool newest = i == g_log_count - 1;
        float h = e->note[0] ? 44.0f : 26.0f;
        y -= h;
        if (y < top + 64) break;

        float t = newest ? viz_ease(e->age / 0.3f) : 1.0f;
        float dx = (1 - t) * 40;
        unsigned char a = (unsigned char)(255 * t);
        if (newest) {
            viz_fill((Rectangle){ x + 8, y - 3, w - 16, h }, viz_tint(COL_ACCENT, (unsigned char)(60 * t)));
            if (g_theme.radius <= 0) DrawRectangle((int)(x + 8), (int)(y - 3), 4, (int)h, viz_tint(COL_ACCENT, a));
        }
        Color cc = newest ? COL_TEXT : viz_tint(COL_TEXT, 190);
        cc.a = a;
        viz_code(e->code, x + 18 + dx, y, fit_size(e->code, 18, w - 30), cc);
        if (e->note[0]) {
            Color nc = viz_tint(COL_GOOD, a);
            const char* note = TextFormat("// %s", e->note);
            viz_code(note, x + 18 + dx, y + 21, fit_size(note, 15, w - 30), nc);
        }
    }
}

// ---------------------------------------------------------------------
// Пошаговые демо
// ---------------------------------------------------------------------

void demo_start(Demo* d)
{
    *d = (Demo){ true, false, 0, 0.0f, false };
}

void demo_stop(Demo* d)
{
    d->active = false;
    d->autoplay = false;
}

bool demo_next(Demo* d, float dt, bool busy)
{
    if (!d->active) return false;
    if (IsKeyPressed(KEY_X)) { demo_stop(d); return false; }
    if (IsKeyPressed(KEY_A)) { d->autoplay = !d->autoplay; d->timer = 0; }
    if (IsKeyPressed(KEY_F)) { d->fast = !d->fast; d->autoplay = true; }
    if (busy || viz_fly_busy()) return false;

    if (IsKeyPressed(KEY_N)) { d->timer = 0; return true; }
    if (d->autoplay) {
        d->timer += dt;
        if (d->timer >= (d->fast ? 0.3f : 1.0f)) { d->timer = 0; return true; }
    }
    return false;
}

void demo_hint(const Demo* d)
{
    viz_hint(d->autoplay ? (d->fast ? "Быстрый автоплей     F — обычная скорость     A — пауза     X — выйти из демо"
                                    : "Автоплей включён     A — пауза     F — быстрее     N — шаг вперёд     X — выйти из демо")
                         : "N — следующий шаг     A — автоплей     F — быстрый автоплей     X — выйти из демо");
}
