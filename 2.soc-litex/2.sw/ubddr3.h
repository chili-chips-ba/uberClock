#pragma once
#ifdef __cplusplus
extern "C" {
    #endif

    /* Command handlers, called by the master table in cmd_list.c - the
     * registry itself no longer lives in this file. */
    void cmd_ddrbyte(char *args);
    void cmd_ddrinfo(char *args);
    void cmd_ddrmap(char *args);
    void cmd_ddrpat(char *args);
    void cmd_ddrprobe(char *args);
    void cmd_ddrtest(char *args);
    void cmd_ddrtestb(char *args);
    void cmd_ddrwait(char *args);
    void cmd_help_ddr(char *args);
    void cmd_timeinfo(char *args);
    void cmd_timertest(char *args);

    #ifdef __cplusplus
}
#endif
