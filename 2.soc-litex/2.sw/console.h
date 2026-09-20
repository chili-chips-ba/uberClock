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
        /* Optional GUI hint, NULL if unspecified (GUI falls back to a
         * plain text-entry field). Grammar: "none" (no argument, plain
         * button), "int:min:max" (spinbox), or "enum:v=label,v=label,..."
         * (dropdown, sends the numeric value). Left NULL for commands
         * with unbounded, optional, or multiple arguments. */
        const char *arg_spec;
    };

    /* Shell */
    void console_init(const char *prompt);
    void console_register(const struct cmd_entry *tbl, unsigned n);
    void console_poll(void);

    /* Read-only access to registered tables (for introspection, e.g. cmd_list.c) */
    unsigned console_table_count(void);
    unsigned console_table_len(unsigned t);
    const struct cmd_entry *console_table(unsigned t);

    /* Small helpers for modules */
    char *get_token(char **str);
    void puts_help_header(const char *title);
    void console_print_prompt(void);

    #ifdef __cplusplus
}
#endif
