// life.h — поле игры «Жизнь» и функции из файла заданий tickets.c.
#ifndef LIFE_H
#define LIFE_H

// Поле w × h клеток. Двумерного массива в памяти нет: клетки лежат одной длинной строкой,
// ряд за рядом. В клетке 1 — живая, 0 — мёртвая.
// cur — текущее поколение (его рисуем), next — сюда считаем следующее.
typedef struct {
    int            w, h;
    unsigned char* cur;
    unsigned char* next;
} Grid;

int  cell_index(const Grid* g, int x, int y);        // задание 1: где в строке лежит клетка (x, y)
int  count_neighbors(const Grid* g, int x, int y);   // задание 2: сколько живых соседей из 8 (поле замкнуто)
void life_step(Grid* g);                             // задание 3: одно поколение
void grid_resize(Grid* g, int w, int h);             // задание 4: новый размер поля

#endif
