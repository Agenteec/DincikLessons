#include "aquarium.h"
#include "raymath.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void fish_init(Fish* f, const char* name, Color color, Vector2 pos)
{
    f->color = color;
    strcpy(f->name, name);
    f->pos = pos;

    float dir = GetRandomValue(0, 1) ? 1.0f : -1.0f;
    f->vel = (Vector2){ dir * (float)GetRandomValue(40, 90), 0.0f };
    f->size = 18.0f;
    f->hunger = 0.5f;
}

void fish_update(Fish* f, float dt)
{
    f->pos.x += f->vel.x * dt;
    f->pos.y += f->vel.y * dt;

    // отскок от стенок
    if (f->pos.x < f->size || f->pos.x > SCREEN_W - f->size) {
        f->vel.x = -f->vel.x;
        f->pos.x = Clamp(f->pos.x, f->size, SCREEN_W - f->size);
    }
    if (f->pos.y < 60 || f->pos.y > SCREEN_H - 60) {
        f->vel.y = -f->vel.y;
        f->pos.y = Clamp(f->pos.y, 60, SCREEN_H - 60);
    }

    // плавно гасим вертикальную скорость
    f->vel.y *= 0.98f;

    f->hunger += dt * 0.03f;
    if (f->hunger > 1.0f) f->hunger = 1.0f;
}

void fish_swim_to(Fish* f, Vector2 target, float dt)
{
    float dx = target.x - f->pos.x;
    float dy = target.y - f->pos.y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;

    float speed = fabsf(f->vel.x) + 20.0f;
    f->vel.x += (dx / len * speed - f->vel.x) * 2.0f * dt;
    f->vel.y += (dy / len * speed - f->vel.y) * 2.0f * dt;
}

void fish_feed(Fish* f)
{
    f->hunger = 0.0f;
    if (f->size < 30.0f) f->size += 1.5f;
}

void fish_draw(const Fish* f)
{
    float dir = f->vel.x >= 0 ? 1.0f : -1.0f;
    float s = f->size;

    // хвост
    Vector2 t1 = { f->pos.x - dir * s * 0.8f, f->pos.y };
    Vector2 t2 = { f->pos.x - dir * s * 1.6f, f->pos.y - s * 0.6f };
    Vector2 t3 = { f->pos.x - dir * s * 1.6f, f->pos.y + s * 0.6f };
    if (dir > 0) DrawTriangle(t1, t2, t3, f->color);
    else         DrawTriangle(t1, t3, t2, f->color);

    // тело и глаз
    DrawEllipse((int)f->pos.x, (int)f->pos.y, s, s * 0.6f, f->color);
    DrawCircle((int)(f->pos.x + dir * s * 0.5f), (int)(f->pos.y - s * 0.15f), s * 0.12f, BLACK);

    // имя над рыбкой
    int w = MeasureText(f->name, 14);
    DrawText(f->name, (int)f->pos.x - w / 2, (int)(f->pos.y - s - 16), 14, RAYWHITE);
}

const char* fish_status(const Fish* f)
{
    char buf[64];
    snprintf(buf, sizeof buf, "%s: %s", f->name, f->hunger > 0.7f ? "hungry!" : "happy");
    return buf;
}
