#include "uberclock/uberclock.h"
#include "uberclock/uberclock_internal.h"

void uberclock_register_cmds(void) {
    uberclock_cli_register_all();
}

void uberclock_init(void) {
    uberclock_runtime_init();
}

void uberclock_poll(void) {
    uberclock_runtime_poll();
}
