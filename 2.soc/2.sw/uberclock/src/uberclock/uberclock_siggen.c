#include <stdio.h>
#include "uberclock/uberclock_runtime.h"
#include "uberclock/uberclock_fifo.h"
#include "uberclock/uberclock_siggen.h"

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

void uberclock_siggen_start(void) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();

    siggen->phase_left = 0u;
    siggen->phase_1000 = 0u;
    siggen->phase_right = 0u;
    siggen->increment_left = siggen_phase_increment(970u, 10000u);
    siggen->increment_1000 = siggen_phase_increment(1000u, 10000u);
    siggen->increment_right = siggen_phase_increment(1030u, 10000u);
    siggen->enabled = 1;

    puts("3-tone software generator enabled");
}

void uberclock_siggen_stop(void) {
    uberclock_siggen_state()->enabled = 0;
    puts("3-tone software generator disabled");
}

int uberclock_siggen_step(int16_t *sample_x, int16_t *sample_y) {
    struct uberclock_siggen_state *siggen = uberclock_siggen_state();
    int32_t tone0;
    int32_t tone1;
    int32_t tone2;

    if (!siggen->enabled) {
        return 0;
    }

    siggen->phase_left += siggen->increment_left;
    siggen->phase_1000 += siggen->increment_1000;
    siggen->phase_right += siggen->increment_right;

    tone0 = ((int32_t)siggen->amplitude * (int32_t)sine_lookup(siggen->phase_left)) / 32767;
    tone1 = ((int32_t)siggen->amplitude * (int32_t)sine_lookup(siggen->phase_1000)) / 32767;
    tone2 = ((int32_t)siggen->amplitude * (int32_t)sine_lookup(siggen->phase_right)) / 32767;

    *sample_x = clamp_to_s16(tone0 + tone1 + tone2);
    *sample_y = 0;
    return 1;
}

void uberclock_siggen_service_push(void) {
    int16_t sample_x;
    int16_t sample_y;

    if (!uberclock_siggen_step(&sample_x, &sample_y)) {
        return;
    }

    (void)uberclock_ups_fifo_push(sample_x, sample_y);
}

void uberclock_siggen_set_amplitude(int16_t amplitude) {
    uberclock_siggen_state()->amplitude = amplitude;
}

int16_t uberclock_siggen_amplitude(void) {
    return uberclock_siggen_state()->amplitude;
}
