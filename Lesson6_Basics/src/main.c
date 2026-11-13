// «Основы C шаг за шагом»: консольный тренажёр.
// 1 — проверить задания из tasks.c, 2 — викторина «угадай, что напечатает», 3 — шпаргалка.
// Запуск с параметром --check просто проверяет задания и выходит.
#include "checks.h"
#include "quiz.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>

static void cheatsheet(void)
{
    ui_clear();
    ui_title("Шпаргалка по основам C");

    ui_section("Переменные и типы");
    const char* types[] = {
        "char    1 байт    'a'           символ",
        "int     4 байта   42            целое",
        "float   4 байта   2.5f          дробное",
        "double  8 байт    2.5           дробное поточнее",
        "любой указатель   8 байт        это просто адрес",
        "7 / 2 = 3 (нацело)      7 / 2.0 = 3.5      (int)2.9 = 2",
    };
    ui_box(C_CYAN, types, 6);

    ui_section("& и *");
    const char* ptr[] = {
        "&x       адрес x                  int* p = &x;",
        "*p       то, что лежит по адресу  *p = 9;   // x стал 9",
        "int* p   в объявлении * значит «p — указатель»",
        "NULL     указатель «в никуда». *NULL — падение программы",
        "Ссылок (int&) в C нет — это C++. В C только указатели.",
    };
    ui_box(C_CYAN, ptr, 5);

    ui_section("Массивы и строки");
    const char* arr[] = {
        "int a[5];       индексы 0..4,  a[5] — уже чужая память",
        "a[i]  ==  *(a + i)",
        "for (int i = 0; i < n; i++)    — знак <, а не <=",
        "\"cat\" = 'c' 'a' 't' '\\0'      — 4 байта, '\\0' — конец строки",
    };
    ui_box(C_CYAN, arr, 4);

    ui_section("Стек и куча");
    const char* mem[] = {
        "СТЕК   локальные переменные. Исчезают сами после return.",
        "КУЧА   malloc / calloc. Живёт, пока не вызовешь free.",
        "malloc(n * sizeof(int))   память с мусором",
        "calloc(n, sizeof(int))    память с нулями",
        "free(p); p = NULL;        вернуть память и забыть адрес",
        "Адрес локальной переменной возвращать нельзя: return &x;",
    };
    ui_box(C_CYAN, mem, 6);

    ui_section("Структуры и void*");
    const char* st[] = {
        "Cat c;    c.age = 3;     точка — у самой структуры",
        "Cat* p;   p->age = 3;    стрелка — у указателя",
        "Cat b = a;               копия: менять b — a не меняется",
        "void* v = &x;            адрес чего угодно",
        "*(int*)v                 сначала скажи тип, потом читай",
    };
    ui_box(C_CYAN, st, 5);
    ui_wait_enter();
}

int main(int argc, char** argv)
{
    ui_init();
    if (argc > 1 && strcmp(argv[1], "--check") == 0) {
        int done = checks_run_all(false);
        return done == checks_count() ? 0 : 1;
    }

    for (;;) {
        ui_clear();
        ui_title("Основы C шаг за шагом");
        printf("\n");
        printf("   " C_BOLD "1" C_RESET "  Проверить задания из tasks.c\n");
        printf("   " C_BOLD "2" C_RESET "  Викторина «Угадай, что напечатает»  " C_GRAY "(писать ничего не надо)" C_RESET "\n");
        printf("   " C_BOLD "3" C_RESET "  Шпаргалка\n");
        printf("   " C_BOLD "4" C_RESET "  Открыть карточку задания по номеру\n");
        printf("   " C_BOLD "0" C_RESET "  Выход\n");

        int c = ui_read_int("\n  Выбери пункт: ");
        if (c == -2 || c == 0) break;
        switch (c) {
        case 1: ui_clear(); checks_run_all(true); break;
        case 2: quiz_run(); break;
        case 3: cheatsheet(); break;
        case 4: {
            int n = ui_read_int("  Номер задания (1–21): ");
            if (n == -2) return 0;
            ui_clear();
            checks_show(n);
            break;
        }
        default: break;
        }
    }
    return 0;
}
