// checks.c — проверка заданий из tasks.c. К каждому заданию: коротко теория, пример, схема памяти, подсказка.
#include "checks.h"
#include "memtrack.h"
#include "tasks.h"
#include "ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifdef _MSC_VER
#include <excpt.h>
#endif

typedef bool (*CheckFn)(char* msg, size_t cap);

typedef struct {
    const char* topic;
    const char* title;
    const char* theory[4];
    const char* example;
    const char* diagram[6];
    const char* hint;
    CheckFn     check;
} Task;

#define FAIL(...) do { snprintf(msg, cap, __VA_ARGS__); return false; } while (0)

// ---------- проверки ----------

static bool c_sum(char* msg, size_t cap)
{
    int r = sum(2, 3);
    if (r != 5) FAIL("sum(2, 3): ожидали 5, получили %d", r);
    r = sum(-4, 10);
    if (r != 6) FAIL("sum(-4, 10): ожидали 6, получили %d", r);
    return true;
}

static bool c_average(char* msg, size_t cap)
{
    float r = average(2, 3);
    if (fabsf(r - 2.5f) > 0.001f) FAIL("average(2, 3): ожидали 2.5, получили %g", r);
    return true;
}

static bool c_is_even(char* msg, size_t cap)
{
    int nums[] = { 4, 7, 0, 13, 100 };
    int want[] = { 1, 0, 1, 0, 1 };
    for (int i = 0; i < 5; i++)
        if ((is_even(nums[i]) != 0) != want[i]) FAIL("is_even(%d): ожидали %d, получили %d", nums[i], want[i], is_even(nums[i]));
    return true;
}

static bool c_sum_to(char* msg, size_t cap)
{
    int r = sum_to(4);
    if (r != 10) FAIL("sum_to(4) = 1+2+3+4: ожидали 10, получили %d", r);
    return true;
}

static bool c_count_char(char* msg, size_t cap)
{
    int r = count_char("banana", 'a');
    if (r != 3) FAIL("count_char(\"banana\", 'a'): ожидали 3, получили %d", r);
    return true;
}

static bool c_set_to_zero(char* msg, size_t cap)
{
    int x = 42;
    set_to_zero(&x);
    if (x != 0) FAIL("int x = 42; set_to_zero(&x); — x остался %d, а должен стать 0", x);
    return true;
}

static bool c_swap(char* msg, size_t cap)
{
    int a = 1, b = 2;
    swap(&a, &b);
    if (a != 2 || b != 1) FAIL("a = 1, b = 2; swap(&a, &b): ожидали a = 2, b = 1, получили a = %d, b = %d", a, b);
    return true;
}

static bool c_bigger(char* msg, size_t cap)
{
    int a = 3, b = 8;
    int* r = bigger(&a, &b);
    if (r != &b) FAIL("a = 3, b = 8: ожидали адрес b, получили %s", r == &a ? "адрес a" : (r ? "чужой адрес" : "NULL"));
    r = bigger(&b, &a);
    if (r != &b) FAIL("bigger(&b, &a) при b = 8: ожидали адрес b");
    return true;
}

static int* tracked_array(const int* values, int n, int sentinel)
{
    // лишняя ячейка в конце — «чужая память» со значением sentinel: так видно выход за границу
    int* arr = mt_malloc((n + 1) * sizeof(int));
    for (int i = 0; i < n; i++) arr[i] = values[i];
    arr[n] = sentinel;
    return arr;
}

static bool c_array_sum(char* msg, size_t cap)
{
    int v[] = { 1, 2, 3, 4, 5 };
    int* arr = tracked_array(v, 5, 1000);
    int r = array_sum(arr, 5);
    mt_free(arr);
    if (r == 1015) FAIL("ожидали 15, получили 1015 — лишние 1000 лежали в ячейке arr[5], ЗА концом массива");
    if (r != 15) FAIL("array_sum({1,2,3,4,5}, 5): ожидали 15, получили %d", r);
    return true;
}

static bool c_find_index(char* msg, size_t cap)
{
    int v[] = { 7, 3, 9 };
    if (find_index(v, 3, 9) != 2) FAIL("find_index({7,3,9}, 3, 9): ожидали 2, получили %d", find_index(v, 3, 9));
    int r = find_index(v, 3, 5);
    if (r != -1) FAIL("find_index({7,3,9}, 3, 5): числа 5 нет, ожидали -1, получили %d", r);
    return true;
}

static bool c_array_max(char* msg, size_t cap)
{
    int a[] = { 4, 11, 2 };
    if (array_max(a, 3) != 11) FAIL("array_max({4,11,2}): ожидали 11, получили %d", array_max(a, 3));
    int b[] = { -5, -2, -9 };
    int r = array_max(b, 3);
    if (r != -2) FAIL("array_max({-5,-2,-9}): ожидали -2, получили %d", r);
    return true;
}

static bool c_strlen(char* msg, size_t cap)
{
    int r = my_strlen("hello");
    if (r != 5) FAIL("my_strlen(\"hello\"): ожидали 5, получили %d", r);
    if (my_strlen("") != 0) FAIL("my_strlen(\"\"): ожидали 0");
    return true;
}

static bool c_heap_int(char* msg, size_t cap)
{
    int* p = heap_int(42);
    if (!p) FAIL("heap_int вернула NULL");
    int v = *p;
    mt_free(p);
    if (v != 42) FAIL("heap_int(42): по адресу лежит %d (0x%X), а должно быть 42", v, (unsigned)v);
    return true;
}

static bool c_zeros(char* msg, size_t cap)
{
    int* p = zeros(4);
    if (!p) FAIL("zeros вернула NULL");
    int bad = -1;
    for (int i = 0; i < 4; i++) if (p[i] != 0) { bad = i; break; }
    int v = bad >= 0 ? p[bad] : 0;
    mt_free(p);
    if (bad >= 0) FAIL("zeros(4): в p[%d] лежит 0x%X — это мусор, а не 0", bad, (unsigned)v);
    return true;
}

static bool c_sum_temp(char* msg, size_t cap)
{
    int before = mt_live_blocks();
    int r = sum_temp(5);
    int leaked = mt_live_blocks() - before;
    if (r != 15) FAIL("sum_temp(5): ожидали 15, получили %d", r);
    if (leaked > 0) FAIL("сумма верная, но в куче остался %d блок — утечка памяти", leaked);
    return true;
}

static bool c_cat_birthday(char* msg, size_t cap)
{
    Cat c = { "Мурка", 3 };
    cat_birthday(&c);
    if (c.age != 4) FAIL("кошке было 3, после cat_birthday стало %d, а должно 4", c.age);
    return true;
}

static bool c_cat_new(char* msg, size_t cap)
{
    Cat* c = cat_new("Барсик", 2);
    if (!c) FAIL("cat_new вернула NULL");
    int age = c->age;
    bool name_ok = strcmp(c->name, "Барсик") == 0;
    mt_free(c);
    if (!name_ok) FAIL("имя кота не совпало");
    if (age != 2) FAIL("cat_new(\"Барсик\", 2): age = %d (0x%X) — мусор, возраст не записан", age, (unsigned)age);
    return true;
}

static bool c_read_int(char* msg, size_t cap)
{
    int x = 77;
    int r = read_int(&x);
    if (r != 77) FAIL("int x = 77; read_int(&x): ожидали 77, получили %d", r);
    return true;
}

static bool c_read_double(char* msg, size_t cap)
{
    double d = 3.75;
    double r = read_double(&d);
    if (fabs(r - 3.75) > 1e-9) FAIL("double d = 3.75; read_double(&d): получили %g — байты double прочитали как int", r);
    return true;
}

static bool c_reverse(char* msg, size_t cap)
{
    int a[] = { 1, 2, 3, 4, 5 };
    reverse(a, 5);
    int want[] = { 5, 4, 3, 2, 1 };
    if (memcmp(a, want, sizeof a) != 0)
        FAIL("reverse({1,2,3,4,5}): получили {%d,%d,%d,%d,%d}, ожидали {5,4,3,2,1}", a[0], a[1], a[2], a[3], a[4]);
    return true;
}

static bool c_bubble(char* msg, size_t cap)
{
    int a[] = { 5, 1, 4, 2, 3 };
    bubble_sort(a, 5);
    int want[] = { 1, 2, 3, 4, 5 };
    if (memcmp(a, want, sizeof a) != 0)
        FAIL("bubble_sort({5,1,4,2,3}): получили {%d,%d,%d,%d,%d}", a[0], a[1], a[2], a[3], a[4]);
    return true;
}

// ---------- задания: теория, пример, схема ----------

static const Task TASKS[] = {
    { "Переменные и функции", "Сумма двух чисел",
      { "Функция получает значения через параметры и отдаёт результат через return.",
        "int sum(int a, int b) — «принимаю два int, возвращаю int»." },
      "int doubled(int x)\n{\n    return x * 2;\n}",
      { "doubled(5)  →  x = 5  →  return 10" },
      "Нужна одна строка: return a + b;", c_sum },

    { "Переменные и функции", "Среднее и деление",
      { "В C int / int — это деление НАЦЕЛО: 5 / 2 = 2.",
        "Чтобы получить 2.5, хотя бы одно число должно быть дробным:",
        "5 / 2.0f = 2.5   или   (float)5 / 2 = 2.5  — это приведение типа." },
      "int   a = 7 / 2;      // 3\nfloat b = 7 / 2.0f;   // 3.5",
      { "(2 + 3) / 2    = 5 / 2    = 2     (дробь отброшена)",
        "(2 + 3) / 2.0f = 5 / 2.0  = 2.5" },
      "Поменяй 2 на 2.0f.", c_average },

    { "Переменные и функции", "Чётное число",
      { "% — остаток от деления: 7 % 2 = 1, 8 % 2 = 0.",
        "Число чётное, если остаток от деления на 2 равен нулю.",
        "Сравнение == даёт 1 (правда) или 0 (ложь)." },
      "int r = 10 % 3;     // 1\nint z = (4 == 4);   // 1",
      { "is_even(8):  8 % 2 = 0  →  0 == 0  →  1" },
      "return n % 2 == 0;", c_is_even },

    { "Циклы", "Сумма от 1 до n",
      { "for (начало; условие; шаг) { тело } — тело повторяется, пока условие верно.",
        "i — счётчик: в задании он идёт 1, 2, 3, ..., n." },
      "for (int i = 1; i <= 3; i++)\n    printf(\"%d \", i);   // 1 2 3",
      { "i = 1: total = 0 + 1 = 1",
        "i = 2: total = 1 + 2 = 3",
        "i = 3: total = 3 + 3 = 6" },
      "На каждом шаге прибавляй сам счётчик: total += i;", c_sum_to },

    { "Циклы", "Сколько раз встречается буква",
      { "Строка — массив символов: s[0], s[1], ... В конце стоит '\\0' — конец строки.",
        "Цикл идёт по строке, пока не встретит '\\0'.",
        "Символы сравниваются через ==, символ пишется в одинарных кавычках: 'a'." },
      "const char* s = \"cat\";\n// s[0] = 'c', s[1] = 'a', s[2] = 't', s[3] = '\\0'",
      { "\"banana\":  b  a  n  a  n  a  \\0",
        "               ^     ^     ^     — три 'a'" },
      "Условие: if (s[i] == c)", c_count_char },

    { "Указатели", "Обнулить по адресу",
      { "Указатель хранит АДРЕС. *p — «то, что лежит по этому адресу».",
        "p = 0   — меняет сам указатель (локальную копию адреса), x не трогает.",
        "*p = 0  — открывает коробку по адресу и кладёт туда 0." },
      "int x = 5;\nint* p = &x;   // p хранит адрес x\n*p = 9;        // теперь x == 9",
      { "стек:  [ x = 42 ]  ←──  [ p = адрес x ]",
        "*p = 0  →  [ x = 0 ]",
        " p = 0  →  p больше никуда не указывает, x по-прежнему 42" },
      "Нужна звёздочка: *p = 0;", c_set_to_zero },

    { "Указатели", "Поменять местами",
      { "Чтобы функция изменила переменные снаружи, ей нужны их адреса.",
        "tmp запоминает старое *a, пока в *a кладём *b." },
      "int tmp = *a;   // сохранили\n*a = *b;        // a получило значение b\n*b = tmp;       // b получило старое a",
      { "a = 1, b = 2",
        "tmp = 1 → a = 2 → b = tmp = 1" },
      "В *b кладём то, что сохранили: *b = tmp;", c_swap },

    { "Указатели", "Адрес большего",
      { "Функция может вернуть указатель — адрес одной из переменных.",
        "NULL — «никуда не указывает». Если b больше, нужно вернуть адрес b, а это просто b." },
      "int* p = bigger(&a, &b);\n*p = 0;   // обнулит большее из a и b",
      { "a = 3 (адрес a)   b = 8 (адрес b)",
        "*a > *b? нет  →  return b" },
      "return b;", c_bigger },

    { "Массивы", "Сумма элементов",
      { "У массива из n элементов индексы от 0 до n-1. Элемента arr[n] НЕТ.",
        "Цикл: for (i = 0; i < n; i++). Знак <, а не <=.",
        "Выход за границу в C не останавливает программу — просто читается чужая память." },
      "int a[3] = {10, 20, 30};\n// a[0] = 10, a[1] = 20, a[2] = 30, a[3] — чужая память!",
      { "индекс:   0   1   2   3   4  │  5",
        "arr:     [1] [2] [3] [4] [5] │ [1000]  ← уже не наш массив" },
      "Условие цикла: i < n", c_array_sum },

    { "Массивы", "Найти индекс",
      { "Если нашли — сразу return i, функция заканчивается.",
        "Если цикл прошёл до конца — значит, числа нет.",
        "0 — это настоящий индекс (первый элемент), поэтому «не нашли» обозначают -1." },
      "find_index({7, 3, 9}, 3, 9)  →  2\nfind_index({7, 3, 9}, 3, 5)  →  -1",
      { "{7, 3, 9}, ищем 5:  7? нет  3? нет  9? нет  →  -1" },
      "return -1;", c_find_index },

    { "Массивы", "Максимум",
      { "Начальный «рекорд» должен быть одним из элементов массива.",
        "Если начать с 0, а все числа отрицательные, 0 так и останется «максимумом»." },
      "int best = arr[0];   // первый элемент — честный кандидат",
      { "{-5, -2, -9}:  best = 0  →  -5 > 0? нет ...  →  ответ 0 (неверно!)",
        "              best = -5 →  -2 > -5? да    →  ответ -2" },
      "int best = arr[0];", c_array_max },

    { "Строки", "Длина строки",
      { "Строка \"hello\" в памяти — 6 байт: h e l l o \\0.",
        "Длина — сколько символов ДО '\\0'.",
        "while (условие) — повторяет, пока условие верно." },
      "while (s[n] != '\\0')\n    n++;",
      { "  h   e   l   l   o   \\0",
        "  0   1   2   3   4   5   → длина 5" },
      "while (s[n] != '\\0')", c_strlen },

    { "Стек и куча", "Число в куче",
      { "Локальные переменные живут на СТЕКЕ и исчезают после return.",
        "malloc выделяет память в КУЧЕ — она живёт, пока не вызовешь free.",
        "malloc возвращает адрес. Значение туда кладём через *p." },
      "int* p = malloc(sizeof(int));\n*p = 42;\n...\nfree(p);",
      { "стек:  [ p = 0x..A0 ] ──┐",
        "куча:                   └──→ [ 42 ]   (пока не free)" },
      "*p = value;", c_heap_int },

    { "Стек и куча", "malloc или calloc",
      { "malloc(байты) — выделяет память, внутри мусор (в отладке 0xCD).",
        "calloc(сколько, размер) — выделяет и ЗАПОЛНЯЕТ НУЛЯМИ.",
        "Для массива из n int: calloc(n, sizeof(int))." },
      "int* a = malloc(4 * sizeof(int));   // CD CD CD CD ...\nint* b = calloc(4, sizeof(int));    // 0 0 0 0",
      { "malloc:  [CDCDCDCD] [CDCDCDCD] [CDCDCDCD] [CDCDCDCD]",
        "calloc:  [00000000] [00000000] [00000000] [00000000]" },
      "int* p = calloc(n, sizeof(int));", c_zeros },

    { "Стек и куча", "Не забыть free",
      { "Всё, что взяли malloc/calloc, нужно вернуть через free.",
        "Если адрес потерян, а free не было — это УТЕЧКА: память занята до конца программы.",
        "После free по этому адресу ходить нельзя." },
      "int* arr = malloc(n * sizeof(int));\n... работаем ...\nfree(arr);",
      { "без free:  функция закончилась, arr (на стеке) исчез,",
        "           а блок в куче остался — и на него больше никто не указывает" },
      "free(arr); перед return.", c_sum_temp },

    { "Структуры", "День рождения кота",
      { "Структура — несколько полей вместе: c.name, c.age.",
        "Если есть указатель на структуру — поле через стрелку: c->age.",
        "int age = c->age; — это КОПИЯ числа. Менять копию бесполезно." },
      "Cat* c = &murka;\nc->age = 5;    // то же, что (*c).age = 5",
      { "кот:  [ name = \"Мурка\" | age = 3 ]",
        "age (копия) = 3 → 4,   а в коте всё ещё 3" },
      "c->age++;", c_cat_birthday },

    { "Структуры", "Кот в куче",
      { "malloc(sizeof(Cat)) — место под целую структуру.",
        "Поля заполняем через ->: c->age = age;",
        "Слева поле структуры, справа параметр функции — они могут называться одинаково." },
      "Cat* c = malloc(sizeof(Cat));\nc->age = 2;",
      { "куча:  [ name = \"Барсик\" | age = ??? ]  ← без записи там мусор" },
      "c->age = age;", c_cat_new },

    { "void* и приведение типов", "Прочитать int через void*",
      { "void* — «адрес чего угодно». Компилятор не знает, что там лежит.",
        "Разыменовать void* нельзя. Сначала говорим тип: (const int*)p.",
        "*(const int*)p — «считай, что там int, и прочитай его»." },
      "int x = 77;\nvoid* v = &x;\nint y = *(int*)v;   // 77",
      { "v = адрес x  →  (int*)v  →  *(int*)v = 77" },
      "return *(const int*)p;", c_read_int },

    { "void* и приведение типов", "Прочитать double",
      { "Приведение типа указателя не превращает данные — меняет, как их ЧИТАТЬ.",
        "double занимает 8 байт, int — 4. Прочитать double как int — получить мусор.",
        "Тип при приведении должен совпадать с тем, что лежит на самом деле." },
      "double d = 3.75;\nint    bad  = *(int*)&d;     // мусор\ndouble good = *(double*)&d;  // 3.75",
      { "8 байт double:  [00 00 00 00 | 00 00 0E 40]",
        "(int*) читает первые 4 байта → 0" },
      "return *(const double*)p;", c_read_double },

    { "Простые алгоритмы", "Развернуть массив",
      { "Меняем местами первый и последний, второй и предпоследний...",
        "Хватит дойти до середины: i < n / 2.",
        "Каждый обмен — это swap через tmp." },
      "{1, 2, 3, 4, 5}\n i=0: 1 <-> 5\n i=1: 2 <-> 4",
      { "[1] [2] [3] [4] [5]",
        " ↑               ↑     обмен через tmp" },
      "arr[n - 1 - i] = tmp;", c_reverse },

    { "Простые алгоритмы", "Сортировка пузырьком",
      { "Идём по соседям: если левый больше правого — меняем.",
        "После прохода самое большое «всплывает» в конец.",
        "Для сортировки по возрастанию менять надо, когда arr[j] > arr[j + 1]." },
      "[5, 1, 4] → 5 > 1? меняем → [1, 5, 4] → 5 > 4? меняем → [1, 4, 5]",
      { "проход 1:  5 1 4 2 3  →  1 4 2 3 5   (5 всплыла)" },
      "if (arr[j] > arr[j + 1])", c_bubble },
};

#define TASK_COUNT (int)(sizeof TASKS / sizeof TASKS[0])

int checks_count(void) { return TASK_COUNT; }

// Если код задания упадёт (например, пишет по NULL), не роняем всю программу.
static int run_check(const Task* t, char* msg, size_t cap)
{
    msg[0] = '\0';
#ifdef _MSC_VER
    __try {
        return t->check(msg, cap) ? 1 : 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        snprintf(msg, cap, "программа упала: обращение к чужой памяти (например, *p, когда p = NULL)");
        return -1;
    }
#else
    return t->check(msg, cap) ? 1 : 0;
#endif
}

static void show_card(int i, int result, const char* msg)
{
    const Task* t = &TASKS[i];
    char head[160];
    snprintf(head, sizeof head, "Задание %d. %s  " C_GRAY "(%s)" C_RESET C_BOLD C_PURPLE, i + 1, t->title, t->topic);
    ui_section(head);

    int n = 0;
    while (n < 4 && t->theory[n]) n++;
    ui_box(C_CYAN, t->theory, n);

    printf(C_GRAY "  Пример:\n" C_RESET);
    ui_code(t->example);

    int d = 0;
    while (d < 6 && t->diagram[d]) d++;
    printf(C_GRAY "  В памяти:\n" C_RESET);
    ui_box(C_GRAY, t->diagram, d);

    if (result == 1) {
        printf(C_GREEN "  ✔ Готово!\n" C_RESET);
    } else {
        printf(C_RED "  ✘ %s\n" C_RESET, msg);
        printf(C_YELLOW "  Найди в tasks.c строку с ??? в задании %d.\n" C_RESET, i + 1);
    }
}

int checks_run_all(bool interactive)
{
    char msg[256];
    int done = 0, first_fail = -1, first_result = 0;
    char first_msg[256] = "";
    const char* topic = NULL;

    ui_section("Проверка заданий из tasks.c");
    for (int i = 0; i < TASK_COUNT; i++) {
        if (!topic || strcmp(topic, TASKS[i].topic) != 0) {
            topic = TASKS[i].topic;
            printf(C_BOLD "\n  %s\n" C_RESET, topic);
        }
        int r = run_check(&TASKS[i], msg, sizeof msg);
        if (r == 1) {
            done++;
            printf(C_GREEN "   ✔ " C_RESET "%2d. %s\n", i + 1, TASKS[i].title);
        } else {
            printf(C_RED "   ✘ " C_RESET "%2d. %s" C_GRAY "  — %s" C_RESET "\n", i + 1, TASKS[i].title, msg);
            if (first_fail < 0) {
                first_fail = i;
                first_result = r;
                snprintf(first_msg, sizeof first_msg, "%s", msg);
            }
        }
    }
    printf("\n  ");
    ui_progress(done, TASK_COUNT);

    if (first_fail >= 0) {
        printf(C_BOLD "\n  Следующее задание — №%d. Вот всё, что нужно для него:\n" C_RESET, first_fail + 1);
        show_card(first_fail, first_result, first_msg);
    } else {
        printf(C_GREEN C_BOLD "\n  Все задания выполнены! Ты молодец 🎉\n" C_RESET);
    }
    if (interactive) ui_wait_enter();
    return done;
}

void checks_show(int number)
{
    if (number < 1 || number > TASK_COUNT) {
        printf(C_RED "Такого задания нет: номера от 1 до %d.\n" C_RESET, TASK_COUNT);
        return;
    }
    char msg[256];
    int r = run_check(&TASKS[number - 1], msg, sizeof msg);
    show_card(number - 1, r, msg);
    ui_wait_enter();
}
