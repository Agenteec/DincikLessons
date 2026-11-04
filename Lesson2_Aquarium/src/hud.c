#include "aquarium.h"
#include "memstats.h"

#include <stdio.h>

static char* hud_counters(const Aquarium* aq)
{
    char* text = MT_MALLOC(128);
    snprintf(text, 128, "Fish: %d   Food: %d   Bubbles: %d",
             aq->fish_count, aq->food.count, aq->bubbles.count);
    return text;
}

void hud_draw(const Aquarium* aq)
{
    DrawRectangle(0, 0, SCREEN_W, 34, (Color){ 0, 0, 0, 120 });
    DrawText(hud_counters(aq), 12, 8, 20, RAYWHITE);

    char mem[64];
    snprintf(mem, sizeof mem, "Memory: %d blocks / %zu bytes",
             memstats_blocks(), memstats_bytes());
    int w = MeasureText(mem, 20);
    DrawText(mem, SCREEN_W - w - 12, 8, 20, YELLOW);

    DrawText("LMB - feed   SPACE - bubble   hover - fish mood", 12, SCREEN_H - 22, 16,
             (Color){ 255, 255, 255, 150 });
}
