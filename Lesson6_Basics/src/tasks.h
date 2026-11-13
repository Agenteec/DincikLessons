// tasks.h — функции из файла заданий tasks.c.
#ifndef TASKS_H
#define TASKS_H

// 1. Переменные и функции
int   sum(int a, int b);
float average(int a, int b);
int   is_even(int n);

// 2. Циклы
int sum_to(int n);
int count_char(const char* s, char c);

// 3. Указатели
void set_to_zero(int* p);
void swap(int* a, int* b);
int* bigger(int* a, int* b);

// 4. Массивы
int array_sum(const int* arr, int n);
int find_index(const int* arr, int n, int value);
int array_max(const int* arr, int n);

// 5. Строки
int my_strlen(const char* s);

// 6. Стек и куча
int* heap_int(int value);
int* zeros(int n);
int  sum_temp(int n);

// 7. Структуры
typedef struct {
    char name[16];
    int  age;
} Cat;

void cat_birthday(Cat* c);
Cat* cat_new(const char* name, int age);

// 8. void* и приведение типов
int    read_int(const void* p);
double read_double(const void* p);

// 9. Простые алгоритмы
void reverse(int* arr, int n);
void bubble_sort(int* arr, int n);

#endif
