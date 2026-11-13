// quiz.c — «Угадай, что напечатает»: читаем код и выбираем ответ. Писать ничего не нужно.
#include "quiz.h"
#include "ui.h"

#include <stdio.h>

typedef struct {
    const char* topic;
    const char* code;
    const char* options[4];
    int         answer;        // 1..4
    const char* explain[3];
} Question;

static const Question QUESTIONS[] = {
    { "Деление",
      "int x = 7 / 2;\nprintf(\"%d\", x);",
      { "3.5", "3", "4", "ошибка компиляции" }, 2,
      { "int / int — деление нацело, дробная часть просто отбрасывается.",
        "Не округляется, а отбрасывается: 7 / 2 = 3, 9 / 10 = 0." } },

    { "Массивы и указатели",
      "int a[4] = {10, 20, 30, 40};\nprintf(\"%d\", *(a + 2));",
      { "12", "20", "30", "адрес a[2]" }, 3,
      { "a + 2 — адрес элемента с индексом 2, * — «открыть коробку».",
        "*(a + 2) — это ровно то же самое, что a[2] = 30." } },

    { "Указатели",
      "int x = 5;\nint* p = &x;\n*p = 9;\nprintf(\"%d\", x);",
      { "5", "9", "адрес x", "ошибка" }, 2,
      { "p хранит адрес x. *p = 9 кладёт 9 прямо в x.", "Поэтому x стал 9." } },

    { "Функции и копии",
      "void f(int n) { n = 100; }\n\nint x = 1;\nf(x);\nprintf(\"%d\", x);",
      { "1", "100", "101", "ошибка" }, 1,
      { "В функцию попадает КОПИЯ x. f меняет копию n, а x остаётся 1.",
        "Чтобы f изменила x, ей нужен адрес: void f(int* n) { *n = 100; } и f(&x)." } },

    { "sizeof",
      "printf(\"%zu\", sizeof(int*));   // 64-битная программа",
      { "1", "4", "8", "зависит от числа внутри" }, 3,
      { "Любой указатель — это адрес. В 64-битной программе адрес занимает 8 байт.",
        "Неважно, на что указывает: int*, char*, Cat* — всё по 8 байт." } },

    { "Строки",
      "char s[] = \"cat\";\nprintf(\"%zu\", sizeof s);",
      { "3", "4", "8", "1" }, 2,
      { "\"cat\" в памяти — 4 байта: c a t \\0.", "Невидимый '\\0' в конце показывает, где строка кончается." } },

    { "Куча",
      "int* p = malloc(sizeof(int));\nfree(p);\n*p = 5;",
      { "всё хорошо, *p = 5", "ошибка компиляции", "use-after-free: пишем в чужую память", "p сам станет NULL" }, 3,
      { "После free память больше не наша, но адрес в p остался прежним.",
        "Компилятор не заметит. Программа может упасть сразу, позже — или «работать» с испорченными данными.",
        "Хорошая привычка: после free писать p = NULL;" } },

    { "calloc",
      "int* p = calloc(3, sizeof(int));\nprintf(\"%d\", p[1]);",
      { "мусор", "0", "3", "1" }, 2,
      { "calloc выделяет память и заполняет её нулями.", "malloc — без обнуления: там был бы мусор." } },

    { "Циклы",
      "for (int i = 0; i < 3; i++)\n    printf(\"%d \", i);",
      { "1 2 3", "0 1 2 3", "0 1 2", "0 1" }, 3,
      { "Старт с 0, условие i < 3: i = 0, 1, 2. На i = 3 условие ложно — выход." } },

    { "Структуры",
      "typedef struct { int age; } Cat;\n\nCat a = {3};\nCat b = a;\nb.age = 10;\nprintf(\"%d\", a.age);",
      { "3", "10", "13", "ошибка" }, 1,
      { "Cat b = a; копирует всю структуру. b — отдельный кот.",
        "Меняем b — a остаётся 3. С указателем Cat* b = &a; было бы 10." } },

    { "Приведение типов",
      "double d = 2.9;\nint i = (int)d;\nprintf(\"%d\", i);",
      { "3", "2.9", "2", "ошибка" }, 3,
      { "(int) отбрасывает дробную часть, не округляет: 2.9 → 2." } },

    { "void*",
      "int x = 65;\nvoid* v = &x;\nprintf(\"%d\", *(int*)v);",
      { "65", "'A'", "адрес x", "ошибка: void* нельзя использовать" }, 1,
      { "void* хранит адрес x. Перед чтением приводим к int* — и читаем 65.",
        "Разыменовать сам void* (*v) нельзя: неизвестно, сколько байт читать." } },

    { "Ссылки",
      "int x = 5;\nint& r = x;   // это C, не C++\nr = 7;",
      { "x станет 7", "x останется 5", "ошибка компиляции", "r станет адресом x" }, 3,
      { "В C нет ссылок (int&) — это возможность C++.",
        "В C для этого есть только указатели: int* p = &x; *p = 7;",
        "А & в C — это «взять адрес»: &x." } },

    { "Стек",
      "int* make(void) {\n    int x = 5;\n    return &x;\n}\n\nint* p = make();   // что такое p?",
      { "адрес числа 5 в куче", "NULL", "адрес мёртвой переменной на стеке", "копия числа 5" }, 3,
      { "x живёт на стеке только пока работает make. После return её кадр стёрт.",
        "p указывает в память, которая уже не наша (баг №3 из аквариума).",
        "Чтобы число пережило функцию, его кладут в кучу: malloc." } },
};

#define QUESTION_COUNT (int)(sizeof QUESTIONS / sizeof QUESTIONS[0])

void quiz_run(void)
{
    int score = 0;
    for (int i = 0; i < QUESTION_COUNT; i++) {
        const Question* q = &QUESTIONS[i];
        ui_clear();
        char head[96];
        snprintf(head, sizeof head, "Вопрос %d из %d · %s", i + 1, QUESTION_COUNT, q->topic);
        ui_title(head);
        printf("\n");
        ui_code(q->code);
        printf(C_BOLD "\n  Что будет?\n" C_RESET);
        for (int k = 0; k < 4; k++) printf("   %d) %s\n", k + 1, q->options[k]);

        int a;
        do {
            a = ui_read_int("\n  Твой ответ (1–4): ");
            if (a == -2) return;   // ввод закончился
        } while (a < 1 || a > 4);

        if (a == q->answer) {
            score++;
            printf(C_GREEN C_BOLD "\n  ✔ Верно!\n" C_RESET);
        } else {
            printf(C_RED C_BOLD "\n  ✘ Не совсем. Правильно: %d) %s\n" C_RESET, q->answer, q->options[q->answer - 1]);
        }
        int n = 0;
        while (n < 3 && q->explain[n]) n++;
        ui_box(C_CYAN, q->explain, n);
        ui_wait_enter();
    }
    ui_clear();
    ui_title("Итог викторины");
    printf("\n  Правильных ответов: " C_BOLD "%d из %d\n\n  " C_RESET, score, QUESTION_COUNT);
    ui_progress(score, QUESTION_COUNT);
    if (score == QUESTION_COUNT) printf(C_GREEN "\n  Идеально! 🎉\n" C_RESET);
    else if (score * 2 >= QUESTION_COUNT) printf(C_GREEN "\n  Отлично! Ошибки — это ровно то, что стоит повторить.\n" C_RESET);
    else printf(C_YELLOW "\n  Хорошее начало. Загляни в шпаргалку (пункт 3) и попробуй ещё раз.\n" C_RESET);
    ui_wait_enter();
}
