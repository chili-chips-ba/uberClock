#include "uberclock/uberclock_internal.h"

void uberclock_cli_register_all(void) {
    uberclock_basic_register_cmds();
    uberclock_fft_register_cmds();
    uberclock_siggen_register_cmds();
    uberclock_dma_register_cmds();
}
