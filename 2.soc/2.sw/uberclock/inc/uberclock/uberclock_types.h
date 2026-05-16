// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

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

struct uberclock_iq_frame {
    int16_t x[UBERCLOCK_CHANNEL_COUNT];
    int16_t y[UBERCLOCK_CHANNEL_COUNT];
};

struct uberclock_track_state {
    int enabled;
    unsigned channel;
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
    uint8_t channel_enabled[UBERCLOCK_CHANNEL_COUNT];
    uint32_t phase[UBERCLOCK_CHANNEL_COUNT][3];
    uint32_t increment[UBERCLOCK_CHANNEL_COUNT][3];
    uint32_t frequency_hz[UBERCLOCK_CHANNEL_COUNT][3];
    int16_t amplitude[UBERCLOCK_CHANNEL_COUNT];
};

struct uberclock_app_context {
    struct uberclock_runtime runtime;
    struct uberclock_fft_context fft;
    struct uberclock_track_state track[UBERCLOCK_TRACK_CHANNEL_COUNT];
    struct uberclock_siggen_state siggen;
};

#endif
