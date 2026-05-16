// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <string.h>
#include "uberclock/uberclock_runtime.h"
#include "uberclock/uberclock_fifo.h"
#include "uberclock/uberclock_siggen.h"

#define SIGGEN_TONES 3u

static const int16_t sine_q64[64] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602,
    6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25831, 26318, 26789,
    27244, 27683, 28105, 28510, 28898, 29269, 29622, 29957,
    30274, 30572, 30852, 31113, 31356, 31579, 31783, 31968,
    32133, 32279, 32405, 32512, 32598, 32665, 32713, 32740
};

static uint32_t siggen_phase_increment(uint32_t frequency_hz, uint32_t sample_rate_hz) {
    return (uint32_t)(((uint64_t)frequency_hz << 32) / sample_rate_hz);
}

static int16_t sine_lookup(uint32_t phase) {
    uint8_t quadrant = (uint8_t)(phase >> 30);
    uint8_t index = (uint8_t)((phase >> 24) & 0x3fu);

    switch (quadrant) {
        case 0u: return sine_q64[index];
        case 1u: return sine_q64[63u - index];
        case 2u: return (int16_t)(-sine_q64[index]);
        default: return (int16_t)(-sine_q64[63u - index]);
    }
}

static int16_t clamp_to_s16(int32_t value) {
    if (value > 32767) {
        return 32767;
    }
    if (value < -32768) {
        return -32768;
    }
    return (int16_t)value;
}

static void siggen_update_increments(void) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();
    unsigned channel_index;
    unsigned tone_index;

    for (channel_index = 0u; channel_index < UBERCLOCK_CHANNEL_COUNT; ++channel_index) {
        for (tone_index = 0u; tone_index < SIGGEN_TONES; ++tone_index) {
            siggen->increment[channel_index][tone_index] =
                siggen_phase_increment(siggen->frequency_hz[channel_index][tone_index], 10000u);
        }
    }
}

void uberclock_siggen_start(void) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();

    memset(siggen->phase, 0, sizeof(siggen->phase));
    siggen_update_increments();
    siggen->channel_enabled[0] = 1u;
    siggen->channel_enabled[1] = 1u;
    siggen->channel_enabled[2] = 1u;
    siggen->channel_enabled[3] = 0u;
    siggen->channel_enabled[4] = 0u;
    siggen->enabled = 1;

    puts("sig3 enabled on ch1..ch3");
}

void uberclock_siggen_stop(void) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();
    struct uberclock_iq_frame frame = {0};
    unsigned channel_index;
    unsigned limit = 100000u;

    for (channel_index = 0u; channel_index < UBERCLOCK_CHANNEL_COUNT; ++channel_index) {
        siggen->channel_enabled[channel_index] = 0u;
    }

    while (limit-- > 0u) {
        if (uberclock_ups_fifo_push_frame(&frame)) {
            break;
        }
    }

    siggen->enabled = 0;
    puts("5-channel 3-tone software generator disabled");
}

int uberclock_siggen_step_frame(struct uberclock_iq_frame *frame) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();
    unsigned channel_index;
    unsigned tone_index;

    if (!siggen->enabled) {
        return 0;
    }

    for (channel_index = 0u; channel_index < UBERCLOCK_CHANNEL_COUNT; ++channel_index) {
        int32_t accumulator = 0;

        if (!siggen->channel_enabled[channel_index]) {
            frame->x[channel_index] = 0;
            frame->y[channel_index] = 0;
            continue;
        }

        for (tone_index = 0u; tone_index < SIGGEN_TONES; ++tone_index) {
            siggen->phase[channel_index][tone_index] += siggen->increment[channel_index][tone_index];
            accumulator += ((int32_t)siggen->amplitude[channel_index] *
                            (int32_t)sine_lookup(siggen->phase[channel_index][tone_index])) /
                           32767;
        }

        frame->x[channel_index] = clamp_to_s16(accumulator);
        frame->y[channel_index] = 0;
    }

    return 1;
}

int uberclock_siggen_step(int16_t *sample_x, int16_t *sample_y) {
    struct uberclock_iq_frame frame;

    if (!uberclock_siggen_step_frame(&frame)) {
        return 0;
    }

    *sample_x = frame.x[0];
    *sample_y = frame.y[0];
    return 1;
}

void uberclock_siggen_service_push(void) {
    struct uberclock_iq_frame frame;

    if (!uberclock_siggen_step_frame(&frame)) {
        return;
    }

    (void)uberclock_ups_fifo_push_frame(&frame);
}

void uberclock_siggen_set_amplitude_all(int16_t amplitude) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();
    unsigned channel_index;

    for (channel_index = 0u; channel_index < UBERCLOCK_CHANNEL_COUNT; ++channel_index) {
        siggen->amplitude[channel_index] = amplitude;
    }
}

void uberclock_siggen_set_channel_amplitude(unsigned channel_index, int16_t amplitude) {
    if (channel_index < UBERCLOCK_CHANNEL_COUNT) {
        uberclock_siggen_state()->amplitude[channel_index] = amplitude;
    }
}

int16_t uberclock_siggen_channel_amplitude(unsigned channel_index) {
    if (channel_index >= UBERCLOCK_CHANNEL_COUNT) {
        return 0;
    }
    return uberclock_siggen_state()->amplitude[channel_index];
}

void uberclock_siggen_set_channel_frequencies(unsigned channel_index,
                                              uint32_t f1_hz,
                                              uint32_t f2_hz,
                                              uint32_t f3_hz) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();

    if (channel_index >= UBERCLOCK_CHANNEL_COUNT) {
        return;
    }

    siggen->frequency_hz[channel_index][0] = f1_hz;
    siggen->frequency_hz[channel_index][1] = f2_hz;
    siggen->frequency_hz[channel_index][2] = f3_hz;
    siggen_update_increments();
}

void uberclock_siggen_set_channel_symmetric(unsigned channel_index, uint32_t center_hz, uint32_t delta_hz) {
    if (center_hz <= delta_hz) {
        return;
    }

    uberclock_siggen_set_channel_frequencies(channel_index, center_hz - delta_hz, center_hz, center_hz + delta_hz);
}

void uberclock_siggen_enable_channel(unsigned channel_index) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();

    if (channel_index >= UBERCLOCK_CHANNEL_COUNT) {
        return;
    }

    siggen->enabled = 1;
    siggen->channel_enabled[channel_index] = 1u;
}

void uberclock_siggen_disable_channel(unsigned channel_index) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();

    if (channel_index >= UBERCLOCK_CHANNEL_COUNT) {
        return;
    }

    siggen->channel_enabled[channel_index] = 0u;
}

int uberclock_siggen_channel_enabled(unsigned channel_index) {
    if (channel_index >= UBERCLOCK_CHANNEL_COUNT) {
        return 0;
    }
    return uberclock_siggen_state()->channel_enabled[channel_index] != 0u;
}

uint32_t uberclock_siggen_channel_frequency(unsigned channel_index, unsigned tone_index) {
    if (channel_index >= UBERCLOCK_CHANNEL_COUNT || tone_index >= SIGGEN_TONES) {
        return 0u;
    }
    return uberclock_siggen_state()->frequency_hz[channel_index][tone_index];
}

uint32_t uberclock_siggen_channel_increment(unsigned channel_index, unsigned tone_index) {
    if (channel_index >= UBERCLOCK_CHANNEL_COUNT || tone_index >= SIGGEN_TONES) {
        return 0u;
    }
    return uberclock_siggen_state()->increment[channel_index][tone_index];
}

int64_t uberclock_siggen_increment_to_mhz(uint32_t phase_increment, uint32_t sample_rate_hz) {
    return (int64_t)((((uint64_t)phase_increment * (uint64_t)sample_rate_hz * 1000ull) +
                      (1ull << 31)) >>
                     32);
}
