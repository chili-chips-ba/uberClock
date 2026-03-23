#ifndef UBERCLOCK_H
#define UBERCLOCK_H

#ifdef __cplusplus
extern "C" {
    #endif

    void uberclock_register_cmds(void);
    void uberclock_init(void);
    void uberclock_poll(void);

    #ifdef __cplusplus
}
#endif

#endif
