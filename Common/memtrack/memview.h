// memview.h — как показать счётчик памяти на экране.
#ifndef MEMVIEW_H
#define MEMVIEW_H

#include "memtrack.h"
#include "viz.h"

void        mem_panel(float x, float y, float w);   // панель «Куча»: блоки, байты, вспышка при изменении
void        mem_log_since(long long seq);           // в консоль кода: что выделилось/освободилось после seq
const char* fmt_bytes(size_t n);                    // «12 480 байт»

#endif
