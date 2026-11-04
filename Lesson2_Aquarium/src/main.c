// Аквариум: кормим рыбок кликом, пускаем пузырьки пробелом,
// наводим мышку на рыбку — видим её настроение.
#include "aquarium.h"
#include "memstats.h"

#include <math.h>

static void aquarium_init(Aquarium* aq)
{
    const char* names[] = { "Nemo", "Dory", "Goldie", "Bubbles", "Captain Fin" };
    Color colors[] = { ORANGE, SKYBLUE, GOLD, PINK, LIME };

    aq->fish_count = sizeof names / sizeof names[0];
    aq->fish = MT_MALLOC(aq->fish_count * sizeof(Fish));

    for (int i = 0; i < aq->fish_count; i++) {
        Vector2 pos = { 120.0f + i * 170.0f, (float)GetRandomValue(120, SCREEN_H - 120) };
        fish_init(&aq->fish[i], names[i], colors[i], pos);
    }
}

static void aquarium_free(Aquarium* aq)
{
    MT_FREE(aq->fish);
    aq->fish = NULL;
    aq->fish_count = 0;
    food_free_all(&aq->food);
    bubbles_free_all(&aq->bubbles);
}

static void draw_background(void)
{
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, (Color){ 20, 90, 140, 255 },
                           (Color){ 5, 30, 60, 255 });
    DrawRectangle(0, SCREEN_H - 30, SCREEN_W, 30, (Color){ 194, 170, 120, 255 });

    // водоросли
    float t = (float)GetTime();
    for (int x = 60; x < SCREEN_W; x += 140) {
        for (int k = 0; k < 6; k++) {
            float sway = sinf(t * 1.5f + x * 0.1f + k * 0.5f) * k * 2.0f;
            DrawCircle(x + (int)sway, SCREEN_H - 40 - k * 18, 9.0f - k, (Color){ 40, 140, 70, 220 });
        }
    }
}

static void draw_tooltip(const Fish* f)
{
    const char* status = fish_status(f);
    Vector2 m = GetMousePosition();

    int w = MeasureText(status, 18);
    DrawRectangle((int)m.x + 12, (int)m.y - 30, w + 16, 26, (Color){ 0, 0, 0, 170 });
    DrawText(status, (int)m.x + 20, (int)m.y - 26, 18, RAYWHITE);
}

int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Aquarium");
    SetTargetFPS(60);

    Aquarium aq = { 0 };
    aquarium_init(&aq);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- ввод ---
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            food_drop(&aq.food, GetMousePosition());
        if (IsKeyPressed(KEY_SPACE))
            bubbles_add(&aq.bubbles, (Vector2){ (float)GetRandomValue(20, SCREEN_W - 20), SCREEN_H - 30 });

        // --- логика ---
        for (int i = 0; i < aq.fish_count; i++) {
            Food* target = food_nearest(&aq.food, aq.fish[i].pos, 250.0f);
            if (target) fish_swim_to(&aq.fish[i], target->pos, dt);
            fish_update(&aq.fish[i], dt);
        }
        food_update(&aq.food, aq.fish, aq.fish_count, dt);
        bubbles_update(&aq.bubbles, dt);

        // --- отрисовка ---
        BeginDrawing();
        draw_background();
        bubbles_draw(&aq.bubbles);
        food_draw(&aq.food);

        const Fish* hovered = NULL;
        for (int i = 0; i < aq.fish_count; i++) {
            fish_draw(&aq.fish[i]);
            if (CheckCollisionPointCircle(GetMousePosition(), aq.fish[i].pos, aq.fish[i].size))
                hovered = &aq.fish[i];
        }
        if (hovered) draw_tooltip(hovered);

        hud_draw(&aq);
        EndDrawing();
    }

    aquarium_free(&aq);
    CloseWindow();
    memstats_report();
    return 0;
}
