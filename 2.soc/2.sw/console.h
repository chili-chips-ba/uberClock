#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
    #endif

    typedef void (*cmd_fn)(char *args);

    struct cmd_entry {
        const char *name;
        cmd_fn      fn;
        const char *help;
    };

    /* Shell */
    void console_init(const char *prompt);
    void console_register(const struct cmd_entry *tbl, unsigned n);
    void console_poll(void);

    /* Small helpers for modules */
    char *get_token(char **str);
    void puts_help_header(const char *title);
    void console_print_prompt(void);

    #ifdef __cplusplus
}
#endif
