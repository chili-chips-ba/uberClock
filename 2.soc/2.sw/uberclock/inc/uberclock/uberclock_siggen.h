#ifndef UBERCLOCK_SIGGEN_H
#define UBERCLOCK_SIGGEN_H

#include <stdint.h>

void uberclock_siggen_start(void);
void uberclock_siggen_stop(void);
int uberclock_siggen_step(int16_t *sample_x, int16_t *sample_y);
void uberclock_siggen_service_push(void);
void uberclock_siggen_set_amplitude(int16_t amplitude);
int16_t uberclock_siggen_amplitude(void);

#endif

