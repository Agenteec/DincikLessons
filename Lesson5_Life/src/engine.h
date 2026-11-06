// engine.h — общее для сцен: поле в памяти, рисование, узоры, проверка заданий.
#ifndef ENGINE_H
#define ENGINE_H

#include "life.h"
#include "memview.h"
#include "viz.h"

#define FIELD_X 30.0f
#define FIELD_Y 140.0f
#define FIELD_W 610.0f
#define FIELD_H 470.0f
#define PANEL_X 660.0f
#define PANEL_W 520.0f

void grid_init(Grid* g, int w, int h);   // выделить оба буфера (через счётчик памяти)
void grid_free(Grid* g);
void grid_clear(Grid* g);
void grid_random(Grid* g, int percent);
bool grid_get(const Grid* g, int x, int y);   // по правильной формуле — для рисования
void grid_set(Grid* g, int x, int y, bool alive);

// Эталон — чтобы сравнивать с тем, что получилось у её функций.
int  ref_neighbors(const Grid* g, int x, int y);
void ref_step(Grid* g);

// Рисование: area — куда вписать поле. Возвращает размер клетки.
float     grid_draw(const Grid* g, Rectangle area, Color alive);
Rectangle grid_cell_rect(const Grid* g, Rectangle area, int x, int y);
bool      grid_cell_at(const Grid* g, Rectangle area, Vector2 m, int* x, int* y);

// Узоры
typedef struct {
    const char* name;
    const char* note;
    int         w, h;
    const char* rows[12];
} Pattern;

extern const Pattern PATTERNS[];
extern const int     PATTERN_COUNT;
void pattern_stamp(Grid* g, const Pattern* p, int x, int y);

// Проверка заданий: вызывают её функции на маленьких тестовых полях.
typedef struct { bool index, neighbors, step, resize; } TaskStatus;
TaskStatus tasks_check(void);                   // считается один раз при старте
void       tasks_draw(float x, float y);        // плашки «1 2 3 4» зелёные/красные
bool       student_step_safe(const Grid* g);    // можно ли запускать её life_step на этом поле

#endif
