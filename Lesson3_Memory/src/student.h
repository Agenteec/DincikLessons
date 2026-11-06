// student.h — функции, которые нужно написать. Сам код — в student.c.
#ifndef STUDENT_H
#define STUDENT_H

#include "types.h"

// Сцена 1. Указатели
void swap(int* a, int* b);

// Сцена 2. Массивы
int* find_max(int* arr, int n);

// Сцена 3. Структуры
void ball_move(Ball* b, float dt);

// Сцена 4. Динамический массив
void vec_push(BallVec* v, Ball b);

// Сцена 5. Связный список
void list_push_front(Node** head, int value, Vector2 pos);
void list_remove(Node** head, Node* target);

// Сцена 6. static
int next_ticket(void);

#endif
