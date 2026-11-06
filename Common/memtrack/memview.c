// memview.c — панель «Куча» и запись выделений памяти в консоль кода.
#include "memview.h"

#include <stdio.h>

const char* fmt_bytes(size_t n)
{
    static char buf[4][32];
    static int k = 0;
    k = (k + 1) % 4;
    if (n >= 1000000)   snprintf(buf[k], sizeof buf[k], "%zu %03zu %03zu байт", n / 1000000, n / 1000 % 1000, n % 1000);
    else if (n >= 1000) snprintf(buf[k], sizeof buf[k], "%zu %03zu байт", n / 1000, n % 1000);
    else                snprintf(buf[k], sizeof buf[k], "%zu байт", n);
    return buf[k];
}

void mem_panel(float x, float y, float w)
{
    static int    last_blocks = -1;
    static float  flash;
    static int    delta;
    int blocks = mt_live_blocks();
    if (last_blocks >= 0 && blocks != last_blocks) { delta = blocks - last_blocks; flash = 1; }
    last_blocks = blocks;
    flash = viz_approach(flash, 0, 1.2f, GetFrameTime());

    Rectangle r = { x, y, w, 64 };
    viz_fill(r, COL_PANEL);
    viz_glow(r, flash, delta > 0 ? COL_PTR : COL_GOOD);
    viz_text("Куча (через calloc/malloc)", x + 14, y + 8, 16, COL_DIM);
    viz_code(TextFormat("%d блоков", blocks), x + 14, y + 30, 22, COL_TEXT);
    viz_code(fmt_bytes(mt_live_bytes()), x + 170, y + 30, 22, COL_ADDR);
    if (flash > 0.02f)
        viz_code(TextFormat("%+d", delta), x + w - 60, y + 30, 22, viz_tint(delta > 0 ? COL_PTR : COL_GOOD, (unsigned char)(255 * flash)));
}

void mem_log_since(long long seq)
{
    MtEvent ev[32];
    int n = mt_events_since(seq, ev, 32);
    for (int i = 0; i < n; i++) {
        if (ev[i].kind > 0)      viz_log(TextFormat("// + calloc %zu байт", ev[i].size), TextFormat("блок %s", viz_addr(ev[i].ptr)));
        else if (ev[i].kind < 0) viz_log(TextFormat("// - free %zu байт", ev[i].size), TextFormat("блок %s", viz_addr(ev[i].ptr)));
        else                     viz_log(TextFormat("// ~ realloc → %zu байт", ev[i].size), "внутренний массив Chipmunk вырос");
    }
}
