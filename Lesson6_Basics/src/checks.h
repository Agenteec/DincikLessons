// checks.h — проверка заданий из tasks.c.
#ifndef CHECKS_H
#define CHECKS_H

#include <stdbool.h>

int  checks_count(void);
int  checks_run_all(bool interactive);   // возвращает, сколько заданий выполнено
void checks_show(int number);            // карточка одного задания: теория, пример, схема, результат

#endif
