#ifndef UBERCLOCK_RUNTIME_H
#define UBERCLOCK_RUNTIME_H

#include <stdint.h>
#include "uberclock/uberclock_types.h"

struct uberclock_app_context *uberclock_app_context(void);
struct uberclock_runtime *uberclock_runtime_state(void);
struct uberclock_fft_context *uberclock_fft_context(void);
struct uberclock_track_state *uberclock_track_state(void);
struct uberclock_siggen_state *uberclock_siggen_state(void);

void uberclock_runtime_record_ce_event(void);
void uberclock_runtime_service_ce_event(void);
void uberclock_runtime_service_ce_events(unsigned budget);
void uberclock_runtime_wait_ticks(uint32_t wait_ticks);
void uberclock_cache_sync(void);

#endif

