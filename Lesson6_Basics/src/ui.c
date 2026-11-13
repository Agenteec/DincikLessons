// ui.c — рамки рисуются символами Unicode (┌─┐│└┘), цвета — escape-последовательностями ANSI.
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

void ui_init(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void ui_clear(void) { printf("\x1b[2J\x1b[H"); }

// Ширина на экране: считаем символы, а не байты, и пропускаем escape-коды цветов.
int ui_text_width(const char* s)
{
    int w = 0;
    for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
        if (*p == 0x1b) {                       // \x1b[...m — цвет, на экране не виден
            while (*p && *p != 'm') p++;
            if (!*p) break;
            continue;
        }
        if ((*p & 0xC0) != 0x80) w++;           // байты-продолжения UTF-8 не считаем
    }
    return w;
}

static void repeat(const char* s, int n)
{
    for (int i = 0; i < n; i++) fputs(s, stdout);
}

void ui_title(const char* text)
{
    int w = ui_text_width(text) + 6;
    printf(C_CYAN "╔"); repeat("═", w); printf("╗\n");
    printf("║   " C_BOLD "%s" C_RESET C_CYAN "   ║\n", text);
    printf("╚"); repeat("═", w); printf("╝" C_RESET "\n");
}

void ui_section(const char* text)
{
    int w = ui_text_width(text);
    printf("\n" C_BOLD C_PURPLE "── %s " C_RESET C_GRAY, text);
    repeat("─", w < 60 ? 60 - w : 2);
    printf(C_RESET "\n");
}

void ui_box(const char* color, const char* const* lines, int count)
{
    int w = 0;
    for (int i = 0; i < count; i++) {
        int lw = ui_text_width(lines[i]);
        if (lw > w) w = lw;
    }
    printf("%s┌", color); repeat("─", w + 2); printf("┐" C_RESET "\n");
    for (int i = 0; i < count; i++) {
        printf("%s│" C_RESET " %s", color, lines[i]);
        repeat(" ", w - ui_text_width(lines[i]));
        printf(" %s│" C_RESET "\n", color);
    }
    printf("%s└", color); repeat("─", w + 2); printf("┘" C_RESET "\n");
}

void ui_code(const char* code)
{
    int n = 1;
    const char* line = code;
    while (*line) {
        const char* end = strchr(line, '\n');
        int len = end ? (int)(end - line) : (int)strlen(line);
        printf(C_GRAY " %2d │ " C_RESET C_YELLOW "%.*s" C_RESET "\n", n++, len, line);
        if (!end) break;
        line = end + 1;
    }
}

void ui_progress(int done, int total)
{
    const int width = 30;
    int filled = total ? done * width / total : 0;
    printf(C_GREEN);
    repeat("█", filled);
    printf(C_GRAY);
    repeat("░", width - filled);
    printf(C_RESET "  %d из %d\n", done, total);
}

void ui_wait_enter(void)
{
    printf(C_GRAY "\n(Enter — дальше)" C_RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int ui_read_int(const char* prompt)
{
    char buf[64];
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, sizeof buf, stdin)) return -2;   // ввод закончился
    char* end;
    long v = strtol(buf, &end, 10);
    if (end == buf) return -1;
    return (int)v;
}
