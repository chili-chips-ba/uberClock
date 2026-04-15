#include <stdio.h>
#include <string.h>
#include <libbase/console.h>
#include "console.h"

/* ===== Simple shared console ===== */

static const char *g_prompt = ">";
static const struct cmd_entry *g_tbls[8];
static unsigned g_tblcnts[8], g_ntbls;

static char *readstr(void) {
    char c[2];
    static char s[64];
    static int ptr = 0;

    if (!readchar_nonblock()) return NULL;
    c[0] = getchar(); c[1] = 0;
    switch (c[0]) {
        case 0x7f: case 0x08:
            if (ptr) { ptr--; fputs("\x08 \x08", stdout); }
            break;
        case 0x07:
            break;
        case '\r': case '\n':
            s[ptr] = 0;
            fputs("\n", stdout);
            ptr = 0;
            return s;
        default:
            if (ptr < (int)sizeof(s)-1) { fputs(c, stdout); s[ptr++] = c[0]; }
            break;
    }
    return NULL;
}

char *get_token(char **str) {
    char *p = strchr(*str, ' ');
    if (!p) { char *t = *str; *str += strlen(*str); return t; }
    *p = 0;  { char *t = *str; *str = p+1; return t; }
}

void console_init(const char *prompt) {
    g_prompt = prompt ? prompt : g_prompt;
}

void console_register(const struct cmd_entry *tbl, unsigned n) {
    if (g_ntbls < 8) {
        g_tbls[g_ntbls]   = tbl;
        g_tblcnts[g_ntbls]= n;
        g_ntbls++;
    }
}

void console_print_prompt(void) {
    printf("\e[92;1m%s\e[0m ", g_prompt);
}

void puts_help_header(const char *title) {
    puts("");
    puts(title);
    puts("-------------------------------------");
}

void console_poll(void) {
    char *line = readstr();
    if (!line) return;

    char *args = line;
    char *cmd  = get_token(&args);

    for (unsigned t = 0; t < g_ntbls; ++t) {
        for (unsigned i = 0; i < g_tblcnts[t]; ++i) {
            if (!strcmp(cmd, g_tbls[t][i].name)) {
                g_tbls[t][i].fn(args);
                console_print_prompt();
                return;
            }
        }
    }
    printf("Unknown command: %s\n", cmd);
    console_print_prompt();
}
