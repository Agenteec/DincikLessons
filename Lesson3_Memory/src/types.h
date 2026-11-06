// types.h — структуры, с которыми работают сцены и задания.
#ifndef TYPES_H
#define TYPES_H

#include "screen.h"   // размеры окна (Common/viz)

// Мячик: позиция, скорость, радиус и цвет.
typedef struct {
    Vector2 pos;
    Vector2 vel;
    float   radius;
    Color   color;
} Ball;

// Динамический массив мячиков: data — адрес первого, count — сколько лежит, cap — на сколько хватит места.
typedef struct {
    Ball* data;
    int   count;
    int   cap;
} BallVec;

// Узел связного списка: значение, где рисовать и стрелка на следующий узел.
typedef struct Node {
    int          value;
    Vector2      pos;
    struct Node* next;
} Node;

#endif
