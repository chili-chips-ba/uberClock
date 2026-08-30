// SPDX-FileCopyrightText: 2026 Ahmed Imamovic
// SPDX-License-Identifier: CC-BY-SA-4.0

// sig3.c
// 5-channel x 3-tone software DDS test-tone generator: the excitation signal
// used to probe the crystal, injected via ups_fifo into the TX chain.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <generated/csr.h>
#include "uberclock.h"
#include "sig3.h"

#define SIG3_ENABLE_DEFAULT 1
#define SIG3_CHANNELS 5
#define SIG3_TONES    3

static volatile int sig3_enable = 0;
static uint8_t sig3_channel_enable[SIG3_CHANNELS] = {1u, 1u, 1u, 0u, 0u};

/* 32-bit DDS phase accumulators/increments for 5 channels x 3 tones. */
static uint32_t sig3_phase[SIG3_CHANNELS][SIG3_TONES];
static uint32_t sig3_inc[SIG3_CHANNELS][SIG3_TONES];
static uint32_t sig3_freq_hz[SIG3_CHANNELS][SIG3_TONES] = {
    { 990u, 1000u, 1010u},
    { 970u, 1000u, 1030u},
    { 970u, 1000u, 1030u},
    { 990u, 1000u, 1010u},
    { 990u, 1000u, 1010u},
};

/* per-tone amplitude in output counts */
static int16_t sig3_amp[SIG3_CHANNELS] = {3000, 10000, 10000, 3000, 3000};

/* 256-entry sine LUT, one full cycle, Q15-ish signed values */
static const int16_t sine_q64[64] = {
      0,   804,  1608,  2410,  3212,  4011,  4808,  5602,
   6393,  7179,  7962,  8739,  9512, 10278, 11039, 11793,
  12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
  18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
  23170, 23731, 24279, 24811, 25329, 25831, 26318, 26789,
  27244, 27683, 28105, 28510, 28898, 29269, 29622, 29957,
  30274, 30572, 30852, 31113, 31356, 31579, 31783, 31968,
  32133, 32279, 32405, 32512, 32598, 32665, 32713, 32740
};

uint32_t sig3_phase_inc(uint32_t f_hz, uint32_t fs_hz) {
    return (uint32_t)(((uint64_t)f_hz << 32) / fs_hz);
}

uint32_t sig3_phase_inc_from_mhz(int64_t f_hz_milli, uint32_t fs_hz) {
    if (f_hz_milli <= 0)
        return 0u;
    return (uint32_t)((((uint64_t)f_hz_milli) << 32) / ((uint64_t)fs_hz * 1000ull));
}

void sig3_update_increments(void) {
    unsigned ch, tone;

    for (ch = 0; ch < SIG3_CHANNELS; ch++) {
        for (tone = 0; tone < SIG3_TONES; tone++) {
            sig3_inc[ch][tone] = sig3_phase_inc(sig3_freq_hz[ch][tone], 10000u);
        }
    }
}

static void sig3_set_channel_freqs(unsigned channel, uint32_t f1_hz, uint32_t f2_hz, uint32_t f3_hz) {
    if (channel >= SIG3_CHANNELS)
        return;

    sig3_freq_hz[channel][0] = f1_hz;
    sig3_freq_hz[channel][1] = f2_hz;
    sig3_freq_hz[channel][2] = f3_hz;
}

void sig3_set_channel_symmetric(unsigned channel, uint32_t center_hz, uint32_t delta_hz) {
    if (center_hz <= delta_hz)
        return;

    sig3_set_channel_freqs(channel, center_hz - delta_hz, center_hz, center_hz + delta_hz);
}

void sig3_enable_channel(unsigned channel) {
    if (channel >= SIG3_CHANNELS)
        return;

    sig3_enable = 1;
    sig3_channel_enable[channel] = 1u;
}

int16_t sig3_sin_u32(uint32_t ph) {
    uint8_t q = (uint8_t)(ph >> 30);          /* quadrant 0..3 */
    uint8_t idx = (uint8_t)((ph >> 24) & 0x3f); /* 0..63 within quadrant */

    switch (q) {
        case 0: return sine_q64[idx];
        case 1: return sine_q64[63 - idx];
        case 2: return (int16_t)(-sine_q64[idx]);
        default: return (int16_t)(-sine_q64[63 - idx]);
    }
}

void sig3_start(void) {
    memset(sig3_phase, 0, sizeof(sig3_phase));
    sig3_update_increments();
    sig3_channel_enable[0] = 1u;
    sig3_channel_enable[1] = 1u;
    sig3_channel_enable[2] = 1u;
    sig3_channel_enable[3] = 0u;
    sig3_channel_enable[4] = 0u;

    sig3_enable = 1;
    puts("sig3 enabled on ch1..ch3");
}

static void sig3_stop(void) {
    unsigned ch;

    for (ch = 0; ch < SIG3_CHANNELS; ch++) {
        sig3_channel_enable[ch] = 0u;
    }
    {
        iq5_frame_t frame = {0};
        unsigned limit = 100000u;
        while (limit--) {
            unsigned flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);
            if (((flags >> 1) & 1u) != 0u) {
                ups_fifo_write_frame(&frame);
                break;
            }
        }
    }
    sig3_enable = 0;
    puts("5-channel 3-tone software generator disabled");
}

static inline int16_t sig3_clamp_s16(int32_t x) {
    if (x >  32767) return  32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

static int sig3_step(iq5_frame_t *frame) {
    unsigned ch, tone;

    if (!sig3_enable) return 0;

    for (ch = 0; ch < SIG3_CHANNELS; ch++) {
        int32_t acc = 0;

        if (!sig3_channel_enable[ch]) {
            frame->x[ch] = 0;
            frame->y[ch] = 0;
            continue;
        }

        for (tone = 0; tone < SIG3_TONES; tone++) {
            sig3_phase[ch][tone] += sig3_inc[ch][tone];
            acc += ((int32_t)sig3_amp[ch] * (int32_t)sig3_sin_u32(sig3_phase[ch][tone])) / 32767;
        }

        frame->x[ch] = sig3_clamp_s16(acc);
        frame->y[ch] = 0;
    }
    return 1;
}

static int sig3_push_update_now(void) {
    iq5_frame_t frame;
    unsigned limit;

    if (!sig3_step(&frame))
        return 0;

    limit = 100000u;
    while (limit--) {
        unsigned flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);
        if (((flags >> 1) & 1u) != 0u) {
            ups_fifo_write_frame(&frame);
            return 1;
        }
    }
    return 0;
}

static void sig3_push_zero_now(void) {
    iq5_frame_t frame = {0};
    unsigned limit = 100000u;

    while (limit--) {
        unsigned flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);
        if (((flags >> 1) & 1u) != 0u) {
            ups_fifo_write_frame(&frame);
            return;
        }
    }
}

void sig3_push_one(void) {
    unsigned flags;
    iq5_frame_t frame;

    flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);
    if (((flags >> 1) & 1u) == 0u)
        return;

    if (!sig3_step(&frame))
        return;

    ups_fifo_write_frame(&frame);
}
void cmd_sig3_amp(char *a) {
    char *tok1 = strtok(a, " \t");
    char *tok2 = strtok(NULL, " \t");
    char *tok3 = strtok(NULL, " \t");
    int v;
    unsigned ch;

    if (!tok1 || tok3) {
        puts("Usage: sig3_amp <val> | sig3_amp <ch:1..5> <val>");
        return;
    }

    if (!tok2) {
        v = parse_s(tok1, 1, 10000, "sig3_amp");
        if (v < 1 || v > 10000) return;
        for (ch = 0; ch < SIG3_CHANNELS; ch++)
            sig3_amp[ch] = (int16_t)v;
        printf("sig3 amplitude per tone = %d for all channels\n", v);
        return;
    }

    ch = (unsigned)strtoul(tok1, NULL, 0);
    if (ch < 1u || ch > SIG3_CHANNELS) {
        puts("sig3_amp channel must be 1..5");
        return;
    }

    v = parse_s(tok2, 1, 10000, "sig3_amp");
    if (v < 1 || v > 10000) return;

    sig3_amp[ch - 1u] = (int16_t)v;
    printf("sig3 ch%u amplitude per tone = %d\n", ch, v);
}

void cmd_sig3_freqs(char *args) {
    char *tok_ch = strtok(args, " \t");
    char *tok_f1 = strtok(NULL, " \t");
    char *tok_f2 = strtok(NULL, " \t");
    char *tok_f3 = strtok(NULL, " \t");
    unsigned ch;
    unsigned f1, f2, f3;

    if (!tok_ch || !tok_f1 || !tok_f2 || !tok_f3) {
        puts("Usage: sig3_freqs <ch:1..5> <f1_hz> <f2_hz> <f3_hz>");
        return;
    }

    ch = (unsigned)strtoul(tok_ch, NULL, 0);
    if (ch < 1u || ch > SIG3_CHANNELS) {
        puts("sig3_freqs channel must be 1..5");
        return;
    }

    f1 = (unsigned)strtoul(tok_f1, NULL, 0);
    f2 = (unsigned)strtoul(tok_f2, NULL, 0);
    f3 = (unsigned)strtoul(tok_f3, NULL, 0);
    if (f1 == 0u || f2 == 0u || f3 == 0u) {
        puts("sig3_freqs frequencies must be > 0 Hz");
        return;
    }

    sig3_set_channel_freqs(ch - 1u, f1, f2, f3);
    sig3_update_increments();

    printf("sig3 ch%u freqs = %u, %u, %u Hz\n", ch, f1, f2, f3);
}

void cmd_sig3_enable_ch(char *a) {
    unsigned ch = (unsigned)strtoul(a ? a : "0", NULL, 0);

    if (ch < 1u || ch > SIG3_CHANNELS) {
        puts("sig3_enable_ch channel must be 1..5");
        return;
    }

    sig3_enable = 1;
    sig3_channel_enable[ch - 1u] = 1u;
    printf("sig3 channel %u enabled\n", ch);
}

void cmd_sig3_disable_ch(char *a) {
    unsigned ch = (unsigned)strtoul(a ? a : "0", NULL, 0);
    unsigned any_enabled = 0u;
    unsigned i;

    if (ch < 1u || ch > SIG3_CHANNELS) {
        puts("sig3_disable_ch channel must be 1..5");
        return;
    }

    sig3_channel_enable[ch - 1u] = 0u;
    for (i = 0; i < SIG3_CHANNELS; i++) {
        if (sig3_channel_enable[i]) {
            any_enabled = 1u;
            break;
        }
    }
    if (any_enabled) {
        sig3_enable = 1;
        (void)sig3_push_update_now();
    } else {
        sig3_push_zero_now();
        sig3_enable = 0;
    }
    printf("sig3 channel %u disabled\n", ch);
}

void cmd_sig3_start(char *a) {
    (void)a;
    sig3_start();
}

void cmd_sig3_stop(char *a) {
    (void)a;
    sig3_stop();
}

static int64_t sig3_phase_inc_to_mhz(uint32_t phase_inc, uint32_t fs_hz) {
    return (int64_t)((((uint64_t)phase_inc * (uint64_t)fs_hz * 1000ull) + (1ull << 31)) >> 32);
}

/* Accessor for trackq.c: what tone frequency is this sig3 channel currently
 * centered on, if it's actively driving fallback_center_hz's target tone?
 * Falls back to the caller's own center_hz (as milli-Hz) otherwise. */
int64_t sig3_channel_center_hz_milli(unsigned channel, uint32_t fallback_center_hz) {
    if (channel < SIG3_CHANNELS &&
        sig3_enable &&
        sig3_channel_enable[channel] &&
        fallback_center_hz == sig3_freq_hz[channel][1]) {
        return sig3_phase_inc_to_mhz(sig3_inc[channel][1], 10000u);
    }

    return (int64_t)fallback_center_hz * 1000ll;
}
