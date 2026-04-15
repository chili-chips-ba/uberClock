#ifndef UBERCLOCK_TYPES_H
#define UBERCLOCK_TYPES_H

#include <stdint.h>
#include "kiss_fft.h"
#include "uberclock/uberclock_config.h"

struct uberclock_runtime {
    volatile uint32_t ce_ticks;
    volatile uint32_t ce_event;
    int16_t magnitude;
    int32_t phase;
};

struct uberclock_fft_context {
    kiss_fft_cpx fft_in[UBERCLOCK_FFT_MAX_N];
    kiss_fft_cpx fft_out[UBERCLOCK_FFT_MAX_N];
    uint8_t cfg_mem[UBERCLOCK_FFT_CFG_MAX_BYTES];
    uint32_t sample_rate_hz;
};

struct uberclock_track_state {
    int enabled;
    unsigned n;
    unsigned settle;
    uint32_t center_hz;
    uint32_t delta_hz;
    uint32_t next_tick;
    int32_t filtered_error_mhz;
    int32_t step_accumulator_mhz;
};

struct uberclock_siggen_state {
    int enabled;
    uint32_t phase_940;
    uint32_t phase_1000;
    uint32_t phase_1060;
    uint32_t increment_940;
    uint32_t increment_1000;
    uint32_t increment_1060;
    int16_t amplitude;
};

struct uberclock_app_context {
    struct uberclock_runtime runtime;
    struct uberclock_fft_context fft;
    struct uberclock_track_state track;
    struct uberclock_siggen_state siggen;
};

#endif

