// ui.h — красивый вывод в консоль: цвета, рамки, галочки, прогресс.
#ifndef UI_H
#define UI_H

#include <stdbool.h>

// Цвета ANSI. В Windows Terminal и новой консоли Windows 10/11 они работают.
#define C_RESET  "\x1b[0m"
#define C_BOLD   "\x1b[1m"
#define C_DIM    "\x1b[2m"
#define C_RED    "\x1b[91m"
#define C_GREEN  "\x1b[92m"
#define C_YELLOW "\x1b[93m"
#define C_BLUE   "\x1b[94m"
#define C_PURPLE "\x1b[95m"
#define C_CYAN   "\x1b[96m"
#define C_GRAY   "\x1b[90m"

void ui_init(void);                        // UTF-8 и цвета в консоли Windows
void ui_clear(void);
void ui_title(const char* text);           // большой заголовок в двойной рамке
void ui_section(const char* text);         // подзаголовок с линией
void ui_box(const char* color, const char* const* lines, int count);   // рамка вокруг строк
void ui_code(const char* code);            // код с номерами строк (строки через \n)
void ui_progress(int done, int total);
void ui_wait_enter(void);
int  ui_read_int(const char* prompt);      // -1, если ввели не число
int  ui_text_width(const char* s);         // ширина строки на экране (UTF-8 считается по символам)

#endif
