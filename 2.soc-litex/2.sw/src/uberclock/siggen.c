#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uberclock/uberclock_internal.h"

#define SIG3_ENABLE_DEFAULT 1

static volatile int sig3_enable = 0;
static uint8_t sig3_channel_enable[UBERCLOCK_CHANNELS] = {1u, 0u, 0u, 0u, 0u};
static uint32_t sig3_phase[UBERCLOCK_CHANNELS][UBERCLOCK_SIG3_TONES];
static uint32_t sig3_inc[UBERCLOCK_CHANNELS][UBERCLOCK_SIG3_TONES];
static uint32_t sig3_freq_hz[UBERCLOCK_CHANNELS][UBERCLOCK_SIG3_TONES] = {
    { 980u, 1000u, 1020u},
    { 940u,  960u,  980u},
    { 980u, 1000u, 1020u},
    {1020u, 1040u, 1060u},
    {1060u, 1080u, 1100u},
};
static int16_t sig3_amp = 3000;

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

static uint32_t sig3_phase_inc(uint32_t f_hz, uint32_t fs_hz) {
    return (uint32_t)(((uint64_t)f_hz << 32) / fs_hz);
}

static void sig3_update_increments(void) {
    unsigned ch;
    unsigned tone;

    for (ch = 0; ch < UBERCLOCK_CHANNELS; ch++) {
        for (tone = 0; tone < UBERCLOCK_SIG3_TONES; tone++) {
            sig3_inc[ch][tone] = sig3_phase_inc(sig3_freq_hz[ch][tone], 10000u);
        }
    }
}

static int16_t sig3_clamp_s16(int32_t x) {
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

static int16_t sig3_sin_u32(uint32_t ph) {
    uint8_t q = (uint8_t)(ph >> 30);
    uint8_t idx = (uint8_t)((ph >> 24) & 0x3f);

    switch (q) {
    case 0: return sine_q64[idx];
    case 1: return sine_q64[63 - idx];
    case 2: return (int16_t)(-sine_q64[idx]);
    default: return (int16_t)(-sine_q64[63 - idx]);
    }
}

static int sig3_step(iq5_frame_t *frame) {
    unsigned ch;
    unsigned tone;

    if (!sig3_enable) {
        return 0;
    }

    for (ch = 0; ch < UBERCLOCK_CHANNELS; ch++) {
        int32_t acc = 0;

        if (!sig3_channel_enable[ch]) {
            frame->x[ch] = 0;
            frame->y[ch] = 0;
            continue;
        }

        for (tone = 0; tone < UBERCLOCK_SIG3_TONES; tone++) {
            sig3_phase[ch][tone] += sig3_inc[ch][tone];
            acc += ((int32_t)sig3_amp * (int32_t)sig3_sin_u32(sig3_phase[ch][tone])) / 32767;
        }

        frame->x[ch] = sig3_clamp_s16(acc);
        frame->y[ch] = 0;
    }

    return 1;
}

static int sig3_push_update_now(void) {
    iq5_frame_t frame;
    unsigned limit = 100000u;

    if (!sig3_step(&frame)) {
        return 0;
    }

    while (limit--) {
        if (((uberclock_int_read_ups_flags() >> 1) & 1u) != 0u) {
            uberclock_int_ups_fifo_write_frame(&frame);
            return 1;
        }
    }
    return 0;
}

static void sig3_push_zero_now(void) {
    iq5_frame_t frame = {0};
    unsigned limit = 100000u;

    while (limit--) {
        if (((uberclock_int_read_ups_flags() >> 1) & 1u) != 0u) {
            uberclock_int_ups_fifo_write_frame(&frame);
            return;
        }
    }
}

void uberclock_siggen_start(void) {
    memset(sig3_phase, 0, sizeof(sig3_phase));
    sig3_update_increments();
    sig3_channel_enable[0] = 1u;
    sig3_channel_enable[1] = 0u;
    sig3_channel_enable[2] = 0u;
    sig3_channel_enable[3] = 0u;
    sig3_channel_enable[4] = 0u;
    sig3_enable = SIG3_ENABLE_DEFAULT;
    puts("sig3 enabled on ch1 only");
}

void uberclock_siggen_stop(void) {
    unsigned ch;

    for (ch = 0; ch < UBERCLOCK_CHANNELS; ch++) {
        sig3_channel_enable[ch] = 0u;
    }

    sig3_push_zero_now();
    sig3_enable = 0;
    puts("5-channel 3-tone software generator disabled");
}

void uberclock_siggen_push_one(void) {
    iq5_frame_t frame;

    if (((uberclock_int_read_ups_flags() >> 1) & 1u) == 0u) {
        return;
    }
    if (!sig3_step(&frame)) {
        return;
    }
    uberclock_int_ups_fifo_write_frame(&frame);
}

static void prv_cmd_sig3_amp(char *a) {
    int v = uberclock_int_parse_s(a, 1, 10000, "sig3_amp");
    if (v < 1 || v > 10000) {
        return;
    }
    sig3_amp = (int16_t)v;
    printf("sig3 amplitude per tone = %d\n", sig3_amp);
}

static void prv_cmd_sig3_freqs(char *args) {
    char *tok_ch = strtok(args, " \t");
    char *tok_f1 = strtok(NULL, " \t");
    char *tok_f2 = strtok(NULL, " \t");
    char *tok_f3 = strtok(NULL, " \t");
    unsigned ch;
    unsigned f1;
    unsigned f2;
    unsigned f3;

    if (!tok_ch || !tok_f1 || !tok_f2 || !tok_f3) {
        puts("Usage: sig3_freqs <ch:1..5> <f1_hz> <f2_hz> <f3_hz>");
        return;
    }

    ch = (unsigned)strtoul(tok_ch, NULL, 0);
    if (ch < 1u || ch > UBERCLOCK_CHANNELS) {
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

    sig3_freq_hz[ch - 1u][0] = f1;
    sig3_freq_hz[ch - 1u][1] = f2;
    sig3_freq_hz[ch - 1u][2] = f3;
    sig3_update_increments();

    printf("sig3 ch%u freqs = %u, %u, %u Hz\n", ch, f1, f2, f3);
}

static void prv_cmd_sig3_enable_ch(char *a) {
    unsigned ch = (unsigned)strtoul(a ? a : "0", NULL, 0);

    if (ch < 1u || ch > UBERCLOCK_CHANNELS) {
        puts("sig3_enable_ch channel must be 1..5");
        return;
    }

    sig3_enable = 1;
    sig3_channel_enable[ch - 1u] = 1u;
    printf("sig3 channel %u enabled\n", ch);
}

static void prv_cmd_sig3_disable_ch(char *a) {
    unsigned ch = (unsigned)strtoul(a ? a : "0", NULL, 0);
    unsigned any_enabled = 0u;
    unsigned i;

    if (ch < 1u || ch > UBERCLOCK_CHANNELS) {
        puts("sig3_disable_ch channel must be 1..5");
        return;
    }

    sig3_channel_enable[ch - 1u] = 0u;
    for (i = 0; i < UBERCLOCK_CHANNELS; i++) {
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

static void prv_cmd_sig3_start(char *a) {
    (void)a;
    uberclock_siggen_start();
}

static void prv_cmd_sig3_stop(char *a) {
    (void)a;
    uberclock_siggen_stop();
}

static const struct cmd_entry g_siggen_cmds[] = {
    {"sig3_start", prv_cmd_sig3_start, "Start 5 independent 3-tone software generators"},
    {"sig3_stop",  prv_cmd_sig3_stop,  "Stop 5 independent 3-tone software generators"},
    {"sig3_amp",   prv_cmd_sig3_amp,   "Set 3-tone per-tone amplitude shared by all channels"},
    {"sig3_freqs", prv_cmd_sig3_freqs, "Set channel 3-tone frequencies: sig3_freqs <ch> <f1> <f2> <f3>"},
    {"sig3_enable_ch",  prv_cmd_sig3_enable_ch,  "Enable one sig3 channel: sig3_enable_ch <ch>"},
    {"sig3_disable_ch", prv_cmd_sig3_disable_ch, "Disable one sig3 channel: sig3_disable_ch <ch>"},
};

void uberclock_siggen_register_cmds(void) {
    console_register(g_siggen_cmds, sizeof(g_siggen_cmds) / sizeof(g_siggen_cmds[0]));
}
