// Игра «Жизнь»: двумерное поле в одномерной памяти, соседи, два буфера, изменение размера.
// Переключение — клавиши 1–5 или клик по вкладке, F2 — тема оформления.
#include "scenes.h"
#include "viz.h"

static const Scene* g_scenes[] = {
    &scene_cells, &scene_neighbors, &scene_step, &scene_resize, &scene_gallery,
};
#define SCENE_COUNT (int)(sizeof g_scenes / sizeof g_scenes[0])

// Вкладки. Metro — pivot: строчные заголовки тонким шрифтом.
// Material — панель вкладок: ПРОПИСНЫЕ подписи, индикатор под текущей, тень под панелью.
static int draw_tabs(int current)
{
    int clicked = -1;
    bool metro = g_theme.metro_text;
    DrawRectangle(0, 0, SCREEN_W, WORLD_TOP - 10, metro ? COL_BG : (Color){ 33, 33, 34, 255 });
    if (!metro) DrawRectangleGradientV(0, WORLD_TOP - 10, SCREEN_W, 8, (Color){ 0, 0, 0, 110 }, (Color){ 0, 0, 0, 0 });

    float x = 24;
    float sel_x = 0, sel_w = 0;
    for (int i = 0; i < SCENE_COUNT; i++) {
        const char* label = metro ? g_scenes[i]->title : viz_upper(g_scenes[i]->title);
        float size = metro ? 30.0f : 22.0f;
        Vector2 m = viz_measure_heading(label, size);
        float pad = metro ? 14.0f : 22.0f;
        Rectangle r = { x - 6, 4, m.x + pad + 16, (float)(WORLD_TOP - 18) };
        bool hover = CheckCollisionPointRec(GetMousePosition(), r);
        if (!metro && hover) DrawRectangleRec((Rectangle){ x - 8, 0, m.x + pad + 20, WORLD_TOP - 10 }, (Color){ 255, 255, 255, 12 });
        Color c = i == current ? (metro ? COL_TEXT : COL_ACCENT)
                               : (hover ? viz_tint(COL_TEXT, 200) : viz_tint(COL_TEXT, metro ? 110 : 150));

        viz_text(TextFormat("%d", i + 1), x, 12, 14, i == current ? COL_ACCENT : viz_tint(COL_TEXT, 90));
        viz_heading(label, x + pad, metro ? 6.0f : 14.0f, size, c);
        if (i == current) { sel_x = x + pad; sel_w = m.x; }
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) clicked = i;
        x += m.x + (metro ? 40 : 52);
    }
    // индикатор под текущей вкладкой плавно переезжает
    Vector2 bar = viz_smooth(g_scenes, 0, (Vector2){ sel_x, sel_w });
    if (metro) DrawRectangle((int)bar.x, WORLD_TOP - 16, (int)bar.y, 3, COL_ACCENT);
    else       DrawRectangle((int)bar.x - 10, WORLD_TOP - 13, (int)bar.y + 20, 3, COL_ACCENT);

    const char* hint = TextFormat("F2 \xE2\x80\x94 тема: %s", g_theme.name);
    viz_text(hint, SCREEN_W - viz_measure(hint, 16).x - 20, 22, 16, viz_tint(COL_TEXT, 120));
    return clicked;
}

int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "Игра «Жизнь»");
    SetTargetFPS(60);
    viz_load_fonts();

    for (int i = 0; i < SCENE_COUNT; i++) g_scenes[i]->init();
    int current = 0;

    while (!WindowShouldClose()) {
        int requested = -1;
        for (int i = 0; i < SCENE_COUNT; i++)
            if (IsKeyPressed(KEY_ONE + i)) requested = i;
        if (requested >= 0 && requested != current) {
            viz_anim_reset();
            viz_log_clear();
            current = requested;
        }

        if (IsKeyPressed(KEY_F2)) viz_theme_next();

        float dt = GetFrameTime();
        viz_anim_update(dt);
        g_scenes[current]->update(dt);

        BeginDrawing();
        ClearBackground(COL_BG);
        g_scenes[current]->draw();
        viz_anim_draw();
        viz_log_draw(dt);
        int clicked = draw_tabs(current);
        EndDrawing();
        viz_frame_end();

        if (clicked >= 0 && clicked != current) {
            viz_anim_reset();
            viz_log_clear();
            current = clicked;
        }
    }

    for (int i = 0; i < SCENE_COUNT; i++) g_scenes[i]->unload();
    viz_unload_fonts();
    CloseWindow();
    return 0;
}
