// teacher.c — готовые решения всех заданий из student.c.
// Собирается в отдельную цель MemoryLab_Teacher: можно показать, как сцена должна работать,
// или сравнить со своим решением. Дине — только после того, как попробует сама 🙂
#include "student.h"

#include <stdlib.h>

// Задание 1: меняем не адреса, а то, что лежит ПО адресам.
void swap(int* a, int* b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// Задание 2: храним адрес лучшего элемента, а не его значение.
int* find_max(int* arr, int n)
{
    int* best = &arr[0];
    for (int i = 1; i < n; i++)
        if (arr[i] > *best) best = &arr[i];
    return best;
}

// Задание 3: b — указатель, поэтому поля через ->.
void ball_move(Ball* b, float dt)
{
    b->pos.x += b->vel.x * dt;
    b->pos.y += b->vel.y * dt;
}

// Задание 4: при нехватке места удваиваем cap. Размер в realloc — в БАЙТАХ.
void vec_push(BallVec* v, Ball b)
{
    if (v->count == v->cap) {
        v->cap = v->cap ? v->cap * 2 : 4;
        v->data = realloc(v->data, v->cap * sizeof(Ball));
    }
    v->data[v->count] = b;
    v->count++;
}

// Задание 5а: Node** нужен, чтобы изменить саму переменную head снаружи.
void list_push_front(Node** head, int value, Vector2 pos)
{
    Node* n = malloc(sizeof(Node));
    n->value = value;
    n->pos = pos;
    n->next = *head;
    *head = n;
}

// Задание 5б: pp указывает на «стрелку», которая ведёт к текущему узлу.
void list_remove(Node** head, Node* target)
{
    Node** pp = head;
    while (*pp && *pp != target)
        pp = &(*pp)->next;

    if (*pp) {
        *pp = target->next;   // сначала перецепляем стрелку
        free(target);         // и только потом освобождаем
    }
}

// Задание 6: static — переменная создаётся один раз и помнит значение.
int next_ticket(void)
{
    static int counter = 0;
    counter++;
    return counter;
}
