#ifndef UBERCLOCK_RUNTIME_H
#define UBERCLOCK_RUNTIME_H

#include <stdint.h>

void uberclock_runtime_init(void);
void uberclock_runtime_poll(void);

int16_t uberclock_runtime_get_magnitude(void);
int32_t uberclock_runtime_get_phase(void);
uint32_t uberclock_runtime_get_ce_ticks(void);

#endif
