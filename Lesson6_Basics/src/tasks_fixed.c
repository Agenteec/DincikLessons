// tasks_fixed.c — ответы к tasks.c (собирается в BasicsLab_Teacher). Дине не показывать.
#include "tasks.h"

#include <stdlib.h>
#include <string.h>

int   sum(int a, int b) { return a + b; }
float average(int a, int b) { return (a + b) / 2.0f; }
int   is_even(int n) { return n % 2 == 0; }

int sum_to(int n)
{
    int total = 0;
    for (int i = 1; i <= n; i++) total += i;
    return total;
}

int count_char(const char* s, char c)
{
    int count = 0;
    for (int i = 0; s[i] != '\0'; i++)
        if (s[i] == c) count++;
    return count;
}

void set_to_zero(int* p) { *p = 0; }

void swap(int* a, int* b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int* bigger(int* a, int* b)
{
    if (*a > *b) return a;
    return b;
}

int array_sum(const int* arr, int n)
{
    int total = 0;
    for (int i = 0; i < n; i++) total += arr[i];
    return total;
}

int find_index(const int* arr, int n, int value)
{
    for (int i = 0; i < n; i++)
        if (arr[i] == value) return i;
    return -1;
}

int array_max(const int* arr, int n)
{
    int best = arr[0];
    for (int i = 0; i < n; i++)
        if (arr[i] > best) best = arr[i];
    return best;
}

int my_strlen(const char* s)
{
    int n = 0;
    while (s[n] != '\0') n++;
    return n;
}

int* heap_int(int value)
{
    int* p = malloc(sizeof(int));
    *p = value;
    return p;
}

int* zeros(int n) { return calloc(n, sizeof(int)); }

int sum_temp(int n)
{
    int* arr = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = i + 1;
    int total = 0;
    for (int i = 0; i < n; i++) total += arr[i];
    free(arr);
    return total;
}

void cat_birthday(Cat* c) { c->age++; }

Cat* cat_new(const char* name, int age)
{
    Cat* c = malloc(sizeof(Cat));
    strncpy(c->name, name, sizeof c->name - 1);
    c->name[sizeof c->name - 1] = '\0';
    c->age = age;
    return c;
}

int    read_int(const void* p) { return *(const int*)p; }
double read_double(const void* p) { return *(const double*)p; }

void reverse(int* arr, int n)
{
    for (int i = 0; i < n / 2; i++) {
        int tmp = arr[i];
        arr[i] = arr[n - 1 - i];
        arr[n - 1 - i] = tmp;
    }
}

void bubble_sort(int* arr, int n)
{
    for (int pass = 0; pass < n - 1; pass++)
        for (int j = 0; j < n - 1 - pass; j++)
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
}
