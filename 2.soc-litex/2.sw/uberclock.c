// SPDX-FileCopyrightText: 2026 Ahmed Imamovic
// SPDX-License-Identifier: CC-BY-SA-4.0

// uberclock.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define REF_FFT_PI 3.14159265358979323846

#include <irq.h>
#include <libbase/uart.h>
#include <generated/csr.h>
#include <generated/soc.h>
#include "uberclock.h"
#include "console.h"
#include "ubddr3.h"
#include "kissfft/kiss_fft.h"
#include "libliteeth/udp.h"   // LiteEth UDP stack header
static inline unsigned parse_u(const char *s, unsigned max, const char *what);
static inline int parse_s(const char *s, int minv, int maxv, const char *what);

typedef struct {
    int16_t x[5];
    int16_t y[5];
} iq5_frame_t;

typedef struct {
    int16_t x[6];
    int16_t y[6];
} iq6_frame_t;

static void write_upsampler_inputs_all_x(int16_t v) {
    uint32_t w = (uint32_t)((int32_t)v & 0xffff);
    main_upsampler_input_x1_write(w);
    main_upsampler_input_x2_write(w);
    main_upsampler_input_x3_write(w);
    main_upsampler_input_x4_write(w);
    main_upsampler_input_x5_write(w);
}

static void write_upsampler_inputs_all_y(int16_t v) {
    uint32_t w = (uint32_t)((int32_t)v & 0xffff);
    main_upsampler_input_y1_write(w);
    main_upsampler_input_y2_write(w);
    main_upsampler_input_y3_write(w);
    main_upsampler_input_y4_write(w);
    main_upsampler_input_y5_write(w);
}

static void ups_fifo_write_frame(const iq5_frame_t *frame) {
    main_ups_fifo_x1_write((uint32_t)((int32_t)frame->x[0] & 0xffff));
    main_ups_fifo_y1_write((uint32_t)((int32_t)frame->y[0] & 0xffff));
    main_ups_fifo_x2_write((uint32_t)((int32_t)frame->x[1] & 0xffff));
    main_ups_fifo_y2_write((uint32_t)((int32_t)frame->y[1] & 0xffff));
    main_ups_fifo_x3_write((uint32_t)((int32_t)frame->x[2] & 0xffff));
    main_ups_fifo_y3_write((uint32_t)((int32_t)frame->y[2] & 0xffff));
    main_ups_fifo_x4_write((uint32_t)((int32_t)frame->x[3] & 0xffff));
    main_ups_fifo_y4_write((uint32_t)((int32_t)frame->y[3] & 0xffff));
    main_ups_fifo_x5_write((uint32_t)((int32_t)frame->x[4] & 0xffff));
    main_ups_fifo_y5_write((uint32_t)((int32_t)frame->y[4] & 0xffff));
    main_ups_fifo_push_write(1);
}

static void ups_fifo_write_replicated(int16_t x, int16_t y) {
    iq5_frame_t frame = {
        .x = {x, x, x, x, x},
        .y = {y, y, y, y, y},
    };
    ups_fifo_write_frame(&frame);
}

static void ds_fifo_read_frame(iq6_frame_t *frame) {
    main_ds_fifo_pop_write(1);
    frame->x[0] = (int16_t)(main_ds_fifo_x1_read() & 0xffffu);
    frame->y[0] = (int16_t)(main_ds_fifo_y1_read() & 0xffffu);
    frame->x[1] = (int16_t)(main_ds_fifo_x2_read() & 0xffffu);
    frame->y[1] = (int16_t)(main_ds_fifo_y2_read() & 0xffffu);
    frame->x[2] = (int16_t)(main_ds_fifo_x3_read() & 0xffffu);
    frame->y[2] = (int16_t)(main_ds_fifo_y3_read() & 0xffffu);
    frame->x[3] = (int16_t)(main_ds_fifo_x4_read() & 0xffffu);
    frame->y[3] = (int16_t)(main_ds_fifo_y4_read() & 0xffffu);
    frame->x[4] = (int16_t)(main_ds_fifo_x5_read() & 0xffffu);
    frame->y[4] = (int16_t)(main_ds_fifo_y5_read() & 0xffffu);
    frame->x[5] = (int16_t)(main_ds_fifo_xref_read() & 0xffffu);
    frame->y[5] = (int16_t)(main_ds_fifo_yref_read() & 0xffffu);
}

static void cmd_fft32_ds_y(char *args) {
    (void)args;

    const unsigned N = 32u;
    const uint32_t fs_hz = 10000u; /* ce_down rate */

    kiss_fft_cpx in[32];
    kiss_fft_cpx out[32];
    uint8_t cfg_mem[768];

    size_t cfg_need = 0;
    (void)kiss_fft_alloc((int)N, 0, NULL, &cfg_need);
    if (cfg_need > sizeof(cfg_mem)) {
        printf("fft32 cfg too big: need %lu bytes\n", (unsigned long)cfg_need);
        return;
    }

    size_t cfg_len = sizeof(cfg_mem);
    kiss_fft_cfg cfg = kiss_fft_alloc((int)N, 0, cfg_mem, &cfg_len);
    if (!cfg) {
        puts("kiss_fft_alloc failed");
        return;
    }

    /* Pop last 32 real Y samples from DS FIFO */
    for (unsigned i = 0; i < N; i++) {
        if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
            printf("Not enough DS FIFO samples: got %u/%u\n", i, N);
            return;
        }

        iq6_frame_t frame;
        ds_fifo_read_frame(&frame);
        int16_t sy = frame.y[0];

        in[i].r = (kiss_fft_scalar)sy;
        in[i].i = (kiss_fft_scalar)0;
    }

    kiss_fft(cfg, in, out);

    puts("bin,freq_hz,re,im,pwr");

    uint64_t peak_pwr = 0;
    unsigned peak_k = 0;

    for (unsigned k = 0; k < (N / 2u); k++) {
        int32_t re = (int32_t)out[k].r;
        int32_t im = (int32_t)out[k].i;
        uint64_t pwr = (uint64_t)((int64_t)re * re) + (uint64_t)((int64_t)im * im);
        uint64_t f_hz = ((uint64_t)k * (uint64_t)fs_hz) / (uint64_t)N;

        printf("%2u,%5llu,%8ld,%8ld,%12llu\n",
               k,
               (unsigned long long)f_hz,
               (long)re,
               (long)im,
               (unsigned long long)pwr);

        if (k > 0u && pwr > peak_pwr) {
            peak_pwr = pwr;
            peak_k = k;
        }
    }

    {
        uint64_t peak_f_hz = ((uint64_t)peak_k * (uint64_t)fs_hz) / (uint64_t)N;
        printf("fft32_ds_y peak: bin=%u f=%llu Hz pwr=%llu  (Fs=%lu, N=%u, df=%lu Hz)\n",
               peak_k,
               (unsigned long long)peak_f_hz,
               (unsigned long long)peak_pwr,
               (unsigned long)fs_hz,
               N,
               (unsigned long)(fs_hz / N));
    }
}



#define SIG3_ENABLE_DEFAULT 1
#define SIG3_CHANNELS 5
#define SIG3_TONES    3
#define TRACKQ_CHANNELS 3
/* Keep the tracking and temperature-estimator channel order explicit. */
#define TRACKQ_C300_CHANNEL 0u
#define TRACKQ_A100_CHANNEL 1u
#define TRACKQ_C100_CHANNEL 2u

#define TRACKQ_C300_DELTA_HZ 10u
#define TRACKQ_A100_DELTA_HZ 30u
#define TRACKQ_C100_DELTA_HZ 30u

/* Mixer starts at the 25 C nominal resonance minus the 1 kHz BB center. */
#define TRACKQ_C300_START_MIXER_HZ 10003005u
#define TRACKQ_A100_START_MIXER_HZ  6269093u
#define TRACKQ_C100_START_MIXER_HZ  3387617u

static volatile int sig3_enable = 0;
/* Quiet single-tone test override; normal settings remain intact. */
static int sig3_single_channel = -1;
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
static int16_t sig3_amp[SIG3_CHANNELS] = {3000, 10900, 10000, 3000, 3000};

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
static uint32_t sig3_phase_inc(uint32_t f_hz, uint32_t fs_hz) {
    return (uint32_t)(((uint64_t)f_hz << 32) / fs_hz);
}

static uint32_t sig3_phase_inc_from_mhz(int64_t f_hz_milli, uint32_t fs_hz) {
    if (f_hz_milli <= 0)
        return 0u;
    return (uint32_t)((((uint64_t)f_hz_milli) << 32) / ((uint64_t)fs_hz * 1000ull));
}

static void sig3_update_increments(void) {
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

static void sig3_set_channel_symmetric(unsigned channel, uint32_t center_hz, uint32_t delta_hz) {
    if (center_hz <= delta_hz)
        return;

    sig3_set_channel_freqs(channel, center_hz - delta_hz, center_hz, center_hz + delta_hz);
}

static void sig3_enable_channel(unsigned channel) {
    if (channel >= SIG3_CHANNELS)
        return;

    sig3_enable = 1;
    sig3_channel_enable[channel] = 1u;
}

static inline int16_t sig3_sin_u32(uint32_t ph) {
    uint8_t q = (uint8_t)(ph >> 30);          /* quadrant 0..3 */
    uint8_t idx = (uint8_t)((ph >> 24) & 0x3f); /* 0..63 within quadrant */

    switch (q) {
        case 0: return sine_q64[idx];
        case 1: return sine_q64[63 - idx];
        case 2: return (int16_t)(-sine_q64[idx]);
        default: return (int16_t)(-sine_q64[63 - idx]);
    }
}

static void sig3_start(void) {
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

        if (!sig3_channel_enable[ch] ||
            (sig3_single_channel >= 0 && ch != (unsigned)sig3_single_channel)) {
            frame->x[ch] = 0;
            frame->y[ch] = 0;
            continue;
        }

        for (tone = 0; tone < SIG3_TONES; tone++) {
            sig3_phase[ch][tone] += sig3_inc[ch][tone];
            if (sig3_single_channel >= 0 && tone != 1u) continue;
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

static void sig3_push_one(void) {
    unsigned flags;
    iq5_frame_t frame;

    flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);
    if (((flags >> 1) & 1u) == 0u)
        return;

    if (!sig3_step(&frame))
        return;

    ups_fifo_write_frame(&frame);
}
static void cmd_sig3_amp(char *a) {
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
        v = parse_s(tok1, 1, 10900, "sig3_amp");
        if (v < 1 || v > 10900) return;
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

    v = parse_s(tok2, 1, 10900, "sig3_amp");
    if (v < 1 || v > 10900) return;

    sig3_amp[ch - 1u] = (int16_t)v;
    printf("sig3 ch%u amplitude per tone = %d\n", ch, v);
}

static void cmd_sig3_freqs(char *args) {
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

static void cmd_sig3_enable_ch(char *a) {
    unsigned ch = (unsigned)strtoul(a ? a : "0", NULL, 0);

    if (ch < 1u || ch > SIG3_CHANNELS) {
        puts("sig3_enable_ch channel must be 1..5");
        return;
    }

    sig3_enable = 1;
    sig3_channel_enable[ch - 1u] = 1u;
    printf("sig3 channel %u enabled\n", ch);
}

static void cmd_sig3_disable_ch(char *a) {
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

static void cmd_sig3_start(char *a) {
    (void)a;
    sig3_start();
}

static void cmd_sig3_stop(char *a) {
    (void)a;
    sig3_stop();
}
static inline void ub_cache_sync(void) { flush_cpu_dcache(); flush_l2_cache(); }



/* ========================================================================= */
/*                             UberClock                                     */
/* ========================================================================= */

static volatile uint32_t ce_event = 0;
static int16_t  mag;
static int32_t  phase;
static volatile int dsp_pump_enable = 0;
static volatile uint32_t dsp_work_tokens = 0;

#define DSP_SWQ_LEN 256u
static int16_t dsp_swq_x[DSP_SWQ_LEN];
static int16_t dsp_swq_y[DSP_SWQ_LEN];
static unsigned dsp_swq_r = 0;
static unsigned dsp_swq_w = 0;
static unsigned dsp_swq_count = 0;

/* Fixed-cadence DSP pumping is driven from ce_down ISR. */
static unsigned dsp_pump_step(unsigned max_in, unsigned max_out);
static inline void uc_commit(void);

#define FFT_MAX_N 2048u
#define FFT_CFG_MAX_BYTES 12288u
static kiss_fft_cpx fft_in[FFT_MAX_N];
static kiss_fft_cpx fft_out[FFT_MAX_N];
static uint8_t fft_cfg_mem[FFT_CFG_MAX_BYTES];
static uint32_t fft_fs_hz = 10000u;
static volatile uint32_t ce_ticks = 0;
static int16_t track_samples[TRACKQ_CHANNELS][FFT_MAX_N];
static int16_t track_samples_ref[FFT_MAX_N];

#define TRACK3_RF_FS_HZ            65000000u
#define TRACK3_DEFAULT_STEP_HZ     5u
#define TRACK3_DEFAULT_MAX_STEPS   400u
#define TRACK3_DEFAULT_N           2048u
#define TRACK3_DEFAULT_SETTLE      256u
#define TRACK3_DEFAULT_CENTER_HZ   1000u
#define TRACK3_DEFAULT_DELTA_HZ    10u
#define TRACKQ_REF_INPUT_HZ        10000000u
#define TRACKQ_NCO_TARGET_HZ       10000000u
#define TRACKQ_REF_FFT_N                64u
#define TRACKQ_TEMP_NOM_C300_MHZ    10004000000ll
#define TRACKQ_TEMP_NOM_A100_MHZ     6269781000ll
#define TRACKQ_TEMP_NOM_C100_MHZ     3388594000ll
#define TRACKQ_TEMP_W_C300_NC_PER_PPM -327179055ll
#define TRACKQ_TEMP_W_A100_NC_PER_PPM  -25307908ll
#define TRACKQ_TEMP_W_C100_NC_PER_PPM  -11944104ll
/* Measured 25 C table anchors (2026-09-17), with previous Hz/C slopes.
 * H=[alpha, 1], order C300/A100/C100. Historical free-fit residual
 * covariance in Hz is retained as a provisional noise model, then rescaled
 * to ppm using these anchors; no new slope fit or noise measurement.
 * This re-anchors the model: it is no longer the Variant-B free-intercept fit.
 * Temperature row: degrees C/ppm scaled by 1e9; sum is exactly zero.
 * Common row: dimensionless scaled by 1e9; sum is exactly 1e9.
 * These diagnose residual common error AFTER reference correction.
 */
#define TRACKQ_TEMP_JOINT_C300_Q9       38471985ll
#define TRACKQ_TEMP_JOINT_A100_Q9      -29220475ll
#define TRACKQ_TEMP_JOINT_C100_Q9       -9251510ll
#define TRACKQ_TEMP_COMMON_C300_Q9   1003347610ll
#define TRACKQ_TEMP_COMMON_A100_Q9    -10736098ll
#define TRACKQ_TEMP_COMMON_C100_Q9      7388488ll
/* alpha in ppm/degree C scaled by 1e6, for exact linear-model ratio. */
#define TRACKQ_TEMP_ALPHA_C300_Q6       -358644ll
#define TRACKQ_TEMP_ALPHA_A100_Q6     -34323755ll
#define TRACKQ_TEMP_ALPHA_C100_Q6      -1171846ll
#define TRACK3_DEFAULT_BAND_BINS   1u
#define TRACKQ_INTERVAL_TICKS      10000u
#define TRACKQ_CORR_SHIFT          10u
#define TRACKQ_REF_INPUT_HZ        10000000u
#define TRACKQ_MAX_STEP_HZ         2
#define TRACKQ_ERR_ALPHA_NUM       1
#define TRACKQ_ERR_ALPHA_DEN       4
#define TRACKQ_KP_NUM              1
#define TRACKQ_KP_DEN              4
#define TRACKQ_MIN_CONF_PCT        5u
/* Side-power deadband percentages, channel order C300/A100/C100. */
static unsigned trackq_deadband_pct[TRACKQ_CHANNELS] = {10u, 1u, 10u};
static int trackq_control_diag = 0;
static int64_t trackq_inc_remainder[TRACKQ_CHANNELS];
#define TRACKQ_WEAK_GAIN_DEN       4
#define TRACKQ_WEAK_MAX_ERR_MHZ    750
#define TRACKQ_SERVICE_STRIDE      4u
#define TRACK3_FIFO_WAIT_POLLS     1000000u
#define TRACK3_SIDE_MIN_PCT        2u
#define TRACK3_SIDE_MAX_PCT        95u
#define TRACK3_SIDE_BALANCE_PCT    40u

struct trackq_state {
    int enabled;
    unsigned channel;
    unsigned n;
    unsigned settle;
    uint32_t center_hz;
    uint32_t delta_hz;
    uint32_t next_tick;
    int32_t filt_error_mhz;
    int32_t step_accum_mhz;
};

static struct trackq_state trackq[TRACKQ_CHANNELS] = {
    {0, TRACKQ_C300_CHANNEL, TRACK3_DEFAULT_N, TRACK3_DEFAULT_SETTLE, TRACK3_DEFAULT_CENTER_HZ, TRACKQ_C300_DELTA_HZ,  0u, 0, 0},
    {0, TRACKQ_A100_CHANNEL, TRACK3_DEFAULT_N, TRACK3_DEFAULT_SETTLE, TRACK3_DEFAULT_CENTER_HZ, TRACKQ_A100_DELTA_HZ, 0u, 0, 0},
    {0, TRACKQ_C100_CHANNEL, TRACK3_DEFAULT_N, TRACK3_DEFAULT_SETTLE, TRACK3_DEFAULT_CENTER_HZ, TRACKQ_C100_DELTA_HZ, 0u, 0, 0},
};
static uint32_t trackq_log_iteration = 0u;
static int trackq_hybrid_enabled;
static int ref_fft_enabled = 0;
static uint64_t ref_fft_origin = 0;
static uint64_t ref_fft_next = 0;
static uint32_t ref_fft_sequence = 0u;

static uint32_t trackq_default_delta_hz(unsigned channel) {
    switch (channel) {
        case TRACKQ_C300_CHANNEL: return TRACKQ_C300_DELTA_HZ;
        case TRACKQ_A100_CHANNEL: return TRACKQ_A100_DELTA_HZ;
        case TRACKQ_C100_CHANNEL: return TRACKQ_C100_DELTA_HZ;
        default: return TRACKQ_C300_DELTA_HZ;
    }
}

static inline int is_pow2_u(unsigned x) {
    return (x != 0u) && ((x & (x - 1u)) == 0u);
}

static uint32_t uc_phase_inc_from_hz(uint32_t f_hz, uint32_t fs_hz) {
    return (uint32_t)(((uint64_t)f_hz << 26) / (uint64_t)fs_hz);
}

static uint32_t uc_phase_inc_to_hz(uint32_t phase_inc, uint32_t fs_hz) {
    return (uint32_t)((((uint64_t)phase_inc * (uint64_t)fs_hz) + (1u << 25)) >> 26);
}

static int64_t uc_phase_inc_to_mhz(uint32_t phase_inc, uint32_t fs_hz) {
    return (int64_t)((((uint64_t)phase_inc * (uint64_t)fs_hz * 1000ull) + (1u << 25)) >> 26);
}

static int64_t sig3_phase_inc_to_mhz(uint32_t phase_inc, uint32_t fs_hz) {
    return (int64_t)((((uint64_t)phase_inc * (uint64_t)fs_hz * 1000ull) + (1ull << 31)) >> 32);
}

static int64_t trackq_center_tone_hz_milli(unsigned channel) {
    if (channel < SIG3_CHANNELS &&
        sig3_enable &&
        sig3_channel_enable[channel] &&
        trackq[channel].center_hz == sig3_freq_hz[channel][1]) {
        return sig3_phase_inc_to_mhz(sig3_inc[channel][1], 10000u);
    }

    return (int64_t)trackq[channel].center_hz * 1000ll;
}

static uint32_t phase_down_read(unsigned channel) {
    switch (channel) {
        case 0: return main_phase_inc_down_1_read();
        case 1: return main_phase_inc_down_2_read();
        case 2: return main_phase_inc_down_3_read();
        case 3: return main_phase_inc_down_4_read();
        case 4: return main_phase_inc_down_5_read();
        default: return 0u;
    }
}

static void phase_down_write(unsigned channel, uint32_t phase_inc) {
    switch (channel) {
        case 0: main_phase_inc_down_1_write(phase_inc); break;
        case 1: main_phase_inc_down_2_write(phase_inc); break;
        case 2: main_phase_inc_down_3_write(phase_inc); break;
        case 3: main_phase_inc_down_4_write(phase_inc); break;
        case 4: main_phase_inc_down_5_write(phase_inc); break;
        default: break;
    }
}

static uint64_t fft_bin_power(unsigned k) {
    int32_t re = (int32_t)fft_out[k].r;
    int32_t im = (int32_t)fft_out[k].i;
    return (uint64_t)((int64_t)re * re) + (uint64_t)((int64_t)im * im);
}

static uint64_t fft_band_power(unsigned k, unsigned bins) {
    uint64_t pwr = 0;
    unsigned start = (k > bins) ? (k - bins) : 0u;
    unsigned stop = k + bins;
    unsigned i;

    if (stop >= (FFT_MAX_N / 2u))
        stop = (FFT_MAX_N / 2u) - 1u;

    for (i = start; i <= stop; i++)
        pwr += fft_bin_power(i);

    return pwr;
}

static void service_one_ce_event(void) {
    sig3_push_one();
    mag = (int16_t)(main_magnitude_read() & 0xffff);
    evm_pending_write(1);
    evm_enable_write(1);
}

static void track3_service_background(void) {
    if (ce_event == 0u) {
        return;
    }

    ce_event--;
    service_one_ce_event();
}

static void track3_service_background_budget(unsigned budget) {
    while (budget-- && ce_event) {
        ce_event--;
        service_one_ce_event();
    }
}

static void track3_wait_ticks(uint32_t wait_ticks) {
    uint32_t start_tick = ce_ticks;

    while ((uint32_t)(ce_ticks - start_tick) < wait_ticks) {
        track3_service_background();
    }
}

static unsigned ds_fifo_flush_all(unsigned max_samples) {
    unsigned flushed = 0;

    while (flushed < max_samples && (main_ds_fifo_flags_read() & 0x1u)) {
        /* Track/FFT are for channel 1 on this SoC, so consume the ch1 sample. */
        main_ds_fifo_pop_write(1);
        (void)main_ds_fifo_x1_read();
        (void)main_ds_fifo_y1_read();
        flushed++;
        track3_service_background_budget(4);
    }

    return flushed;
}

static int track3_wait_ds_fifo(const char *phase, unsigned sample_idx, unsigned total) {
    unsigned stall = 0;

    while ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
        track3_service_background();
        if (++stall >= TRACK3_FIFO_WAIT_POLLS) {
            printf("track3 %s timeout at sample %u/%u after %u polls\n",
                   phase, sample_idx, total, stall);
            return 0;
        }
    }

    return 1;
}

static int capture_ds_fft_channel(unsigned channel, unsigned n, unsigned settle) {
    for (unsigned i = 0; i < settle; i++) {
        iq6_frame_t frame;

        if (!track3_wait_ds_fifo("settle", i, settle)) {
            return 0;
        }
        ds_fifo_read_frame(&frame);
        track3_service_background_budget(4);
    }

    for (unsigned i = 0; i < n; i++) {
        iq6_frame_t frame;
        int16_t sx;

        if (!track3_wait_ds_fifo("capture", i, n)) {
            return 0;
        }
        ds_fifo_read_frame(&frame);
        sx = frame.x[channel];

        fft_in[i].r = (kiss_fft_scalar)sx;
        fft_in[i].i = (kiss_fft_scalar)0;
        track3_service_background_budget(4);
    }

    return 1;
}

/* Hybrid capture diagnostics: old backlog is not the new record. */
static uint32_t hybrid_fifo_pre, hybrid_fifo_capture, hybrid_fifo_process;
static uint64_t ref_fft_cycles(void);
static unsigned hybrid_fifo_flags(void) {
    return (main_ds_fifo_overflow_read() ? 1u : 0u) |
           (main_ds_fifo_underflow_read() ? 2u : 0u) |
           (main_ups_fifo_overflow_read() ? 4u : 0u) |
           (main_ups_fifo_underflow_read() ? 8u : 0u);
}
static void hybrid_fifo_boundary(void) {
    main_ds_fifo_clear_write(1);
    main_ups_fifo_clear_write(1);
    uint64_t until = ref_fft_cycles() + CONFIG_CLOCK_FREQUENCY/100000u;
    while (ref_fft_cycles() < until) track3_service_background_budget(4u);
}
static int hybrid_fresh_capture_prepare(void) {
    hybrid_fifo_pre = hybrid_fifo_flags();
    hybrid_fifo_capture = hybrid_fifo_process = 0u;
    /* Recover excitation after any preceding UPS fault, servicing and draining.
     * Retain the history; it does not silently disappear from the log. */
    if (hybrid_fifo_pre & 12u) {
        uint64_t until = ref_fft_cycles() + 2ull*CONFIG_CLOCK_FREQUENCY;
        while (ref_fft_cycles() < until) {
            track3_service_background_budget(4u);
            if (main_ds_fifo_flags_read() & 1u) {
                iq6_frame_t frame;
                ds_fifo_read_frame(&frame);
            }
        }
        hybrid_fifo_pre |= hybrid_fifo_flags();
    }
    /* Drain to empty, not merely N+settle samples from an old full FIFO. */
    for (unsigned i = 0; i < 65536u; ++i) {
        if (!(main_ds_fifo_flags_read() & 1u)) {
            hybrid_fifo_pre |= hybrid_fifo_flags();
            hybrid_fifo_boundary();
            return 1;
        }
        iq6_frame_t frame;
        ds_fifo_read_frame(&frame);
        track3_service_background_budget(4u);
    }
    hybrid_fifo_capture = 32u;
    return 0;
}

static int capture_ds_track_multi(unsigned n, unsigned settle) {
    unsigned i;

    for (i = 0; i < settle; i++) {
        iq6_frame_t frame;

        if (!track3_wait_ds_fifo("settle", i, settle))
            return 0;
        ds_fifo_read_frame(&frame);
        track3_service_background_budget(4);
    }

    for (i = 0; i < n; i++) {
        iq6_frame_t frame;

        if (!track3_wait_ds_fifo("capture", i, n))
            return 0;
        ds_fifo_read_frame(&frame);
        track_samples[TRACKQ_C300_CHANNEL][i] = frame.x[TRACKQ_C300_CHANNEL];
        track_samples[TRACKQ_A100_CHANNEL][i] = frame.x[TRACKQ_A100_CHANNEL];
        track_samples[TRACKQ_C100_CHANNEL][i] = frame.x[TRACKQ_C100_CHANNEL];
        track_samples_ref[i] = frame.y[5];
        track3_service_background_budget(4);
    }

    return 1;
}

static uint64_t fft_bin_power_at(unsigned k) {
    int32_t re = (int32_t)fft_out[k].r;
    int32_t im = (int32_t)fft_out[k].i;
    return (uint64_t)((int64_t)re * re) + (uint64_t)((int64_t)im * im);
}

static uint64_t fft_band_power_at(unsigned k, unsigned half_bins, unsigned n) {
    uint64_t pwr = 0;
    unsigned start = (k > half_bins) ? (k - half_bins) : 0u;
    unsigned stop = k + half_bins;
    unsigned bins = n / 2u;
    unsigned i;

    if (stop >= bins)
        stop = bins - 1u;

    for (i = start; i <= stop; i++)
        pwr += fft_bin_power_at(i);

    return pwr;
}

static inline void trackq_service_background_sample(unsigned sample_idx) {
    /* Keep the software-fed upsampler moving during long correlation sweeps. */
    if ((sample_idx & (TRACKQ_SERVICE_STRIDE - 1u)) == (TRACKQ_SERVICE_STRIDE - 1u))
        track3_service_background_budget(4u);
}

static uint64_t track_power_at_hz(uint32_t f_hz, unsigned n) {
    uint32_t phase_acc = 0u;
    uint32_t phase_inc = sig3_phase_inc(f_hz, fft_fs_hz);
    int64_t acc_i = 0;
    int64_t acc_q = 0;
    unsigned i;

    for (i = 0; i < n; i++) {
        int32_t sample = (int32_t)fft_in[i].r;
        int32_t cos_q15 = (int32_t)sig3_sin_u32(phase_acc + 0x40000000u);
        int32_t sin_q15 = (int32_t)sig3_sin_u32(phase_acc);

        acc_i += ((int64_t)sample * (int64_t)cos_q15) >> TRACKQ_CORR_SHIFT;
        acc_q -= ((int64_t)sample * (int64_t)sin_q15) >> TRACKQ_CORR_SHIFT;
        phase_acc += phase_inc;
        trackq_service_background_sample(i);
    }

    return (uint64_t)(acc_i * acc_i) + (uint64_t)(acc_q * acc_q);
}

static uint64_t track_power_at_hz_samples(const int16_t *samples, uint32_t f_hz, unsigned n) {
    uint32_t phase_acc = 0u;
    uint32_t phase_inc = sig3_phase_inc(f_hz, fft_fs_hz);
    int64_t acc_i = 0;
    int64_t acc_q = 0;
    unsigned i;

    for (i = 0; i < n; i++) {
        int32_t sample = (int32_t)samples[i];
        int32_t cos_q15 = (int32_t)sig3_sin_u32(phase_acc + 0x40000000u);
        int32_t sin_q15 = (int32_t)sig3_sin_u32(phase_acc);

        acc_i += ((int64_t)sample * (int64_t)cos_q15) >> TRACKQ_CORR_SHIFT;
        acc_q -= ((int64_t)sample * (int64_t)sin_q15) >> TRACKQ_CORR_SHIFT;
        phase_acc += phase_inc;
        trackq_service_background_sample(i);
    }

    return (uint64_t)(acc_i * acc_i) + (uint64_t)(acc_q * acc_q);
}

static uint64_t track_power_at_mhz_samples(const int16_t *samples, int64_t f_hz_milli, unsigned n) {
    uint32_t phase_acc = 0u;
    uint32_t phase_inc = sig3_phase_inc_from_mhz(f_hz_milli, fft_fs_hz);
    int64_t acc_i = 0;
    int64_t acc_q = 0;
    unsigned i;

    for (i = 0; i < n; i++) {
        int32_t sample = (int32_t)samples[i];
        int32_t cos_q15 = (int32_t)sig3_sin_u32(phase_acc + 0x40000000u);
        int32_t sin_q15 = (int32_t)sig3_sin_u32(phase_acc);

        acc_i += ((int64_t)sample * (int64_t)cos_q15) >> TRACKQ_CORR_SHIFT;
        acc_q -= ((int64_t)sample * (int64_t)sin_q15) >> TRACKQ_CORR_SHIFT;
        phase_acc += phase_inc;
        trackq_service_background_sample(i);
    }

    return (uint64_t)(acc_i * acc_i) + (uint64_t)(acc_q * acc_q);
}

static uint64_t track_band_power_at_hz(uint32_t f_hz, unsigned n) {
    uint32_t df_hz;
    uint64_t center_pwr;
    uint64_t lower_pwr;
    uint64_t upper_pwr;

    center_pwr = track_power_at_hz(f_hz, n);
    if (fft_fs_hz == 0u || n == 0u) {
        return center_pwr;
    }

    df_hz = (uint32_t)((((uint64_t)fft_fs_hz) + (n / 2u)) / (uint64_t)n);
    if (df_hz == 0u) {
        return center_pwr;
    }

    lower_pwr = track_power_at_hz((f_hz > df_hz) ? (f_hz - df_hz) : 0u, n);
    upper_pwr = track_power_at_hz(f_hz + df_hz, n);

    /* Narrow 3-bin band: center + 0.5*(lower + upper). */
    return center_pwr + ((lower_pwr + upper_pwr) >> 1);
}

static uint64_t track_band_power_at_hz_samples(const int16_t *samples, uint32_t f_hz, unsigned n) {
    uint32_t df_hz;
    uint64_t center_pwr;
    uint64_t lower_pwr;
    uint64_t upper_pwr;

    center_pwr = track_power_at_hz_samples(samples, f_hz, n);
    if (fft_fs_hz == 0u || n == 0u) {
        return center_pwr;
    }

    df_hz = (uint32_t)((((uint64_t)fft_fs_hz) + (n / 2u)) / (uint64_t)n);
    if (df_hz == 0u) {
        return center_pwr;
    }

    lower_pwr = track_power_at_hz_samples(samples, (f_hz > df_hz) ? (f_hz - df_hz) : 0u, n);
    upper_pwr = track_power_at_hz_samples(samples, f_hz + df_hz, n);

    return center_pwr + ((lower_pwr + upper_pwr) >> 1);
}

static int track3_triplet_match(uint64_t left_pwr, uint64_t center_pwr, uint64_t right_pwr) {
    uint64_t min_side;
    uint64_t max_side;

    if (center_pwr <= left_pwr || center_pwr <= right_pwr)
        return 0;
    if (left_pwr == 0u || right_pwr == 0u)
        return 0;

    min_side = (left_pwr < right_pwr) ? left_pwr : right_pwr;
    max_side = (left_pwr > right_pwr) ? left_pwr : right_pwr;

    if ((min_side * 100u) < (center_pwr * TRACK3_SIDE_MIN_PCT))
        return 0;
    if ((max_side * 100u) > (center_pwr * TRACK3_SIDE_MAX_PCT))
        return 0;
    if ((min_side * 100u) < (max_side * TRACK3_SIDE_BALANCE_PCT))
        return 0;

    return 1;
}

static int32_t trackq_clamp_step_hz(int32_t correction_hz) {
    if (correction_hz > TRACKQ_MAX_STEP_HZ)
        return TRACKQ_MAX_STEP_HZ;
    if (correction_hz < -TRACKQ_MAX_STEP_HZ)
        return -TRACKQ_MAX_STEP_HZ;
    return correction_hz;
}

static int32_t trackq_clamp_weak_step_hz(int32_t correction_hz) {
    if (correction_hz > 1)
        return 1;
    if (correction_hz < -1)
        return -1;
    return correction_hz;
}

static int trackq_vertex_confident(uint64_t left_pwr, uint64_t center_pwr, uint64_t right_pwr) {
    uint64_t side_span;

    if (!track3_triplet_match(left_pwr, center_pwr, right_pwr))
        return 0;

    side_span = (left_pwr > right_pwr) ? (left_pwr - right_pwr) : (right_pwr - left_pwr);
    return ((side_span * 100u) >= (center_pwr * TRACKQ_MIN_CONF_PCT));
}

static int32_t trackq_side_error_mhz(uint64_t left_pwr, uint64_t right_pwr, uint32_t delta_hz, unsigned deadband_pct) {
    int64_t diff;
    uint64_t sum;
    uint64_t mag;

    sum = left_pwr + right_pwr;
    if (sum == 0u)
        return 0;

    diff = (int64_t)right_pwr - (int64_t)left_pwr;
    mag = (diff < 0ll) ? (uint64_t)(-diff) : (uint64_t)diff;
    if ((mag * 100u) < (sum * deadband_pct))
        return 0;

    diff = diff / TRACKQ_WEAK_GAIN_DEN;
    diff = (diff * (int64_t)delta_hz * 1000ll) / (int64_t)sum;
    if (diff > TRACKQ_WEAK_MAX_ERR_MHZ)
        return TRACKQ_WEAK_MAX_ERR_MHZ;
    if (diff < -TRACKQ_WEAK_MAX_ERR_MHZ)
        return -TRACKQ_WEAK_MAX_ERR_MHZ;
    return (int32_t)diff;
}

static int trackq_bin_vertex_estimate_mhz(const int16_t *samples,
                                          uint32_t center_hz,
                                          unsigned n,
                                          int64_t *vertex_hz_milli_out) {
    uint32_t k_center;
    int64_t x1, x2, x3;
    uint64_t p_left;
    uint64_t p_center;
    uint64_t p_right;
    int64_t y1, y2, y3;
    int64_t den;
    int64_t num;

    if (!samples || !vertex_hz_milli_out || n < 8u || fft_fs_hz == 0u)
        return 0;

    k_center = (uint32_t)((((uint64_t)center_hz * (uint64_t)n) + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);
    if (k_center == 0u || (k_center + 1u) >= (n / 2u))
        return 0;

    x1 = (int64_t)((((uint64_t)(k_center - 1u) * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);
    x2 = (int64_t)((((uint64_t)k_center * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);
    x3 = (int64_t)((((uint64_t)(k_center + 1u) * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);

    p_left = track_power_at_mhz_samples(samples, x1, n);
    p_center = track_power_at_mhz_samples(samples, x2, n);
    p_right = track_power_at_mhz_samples(samples, x3, n);

    if (p_center <= p_left || p_center <= p_right)
        return 0;

    y1 = (int64_t)p_left;
    y2 = (int64_t)p_center;
    y3 = (int64_t)p_right;

    den = (y1 - (2ll * y2) + y3);
    if (den >= 0ll)
        return 0;

    num = (x3 - x1) * (y1 - y3);
    *vertex_hz_milli_out = x2 + (num / (4ll * den));
    return 1;
}

/* Reference estimator shared by trackq and the standalone comparison.
 * Hann/log-power implementation adapted from dev-gui 9b07015. */
/* Queue a bounded reserve before non-yielding FFT work. 512 samples are
 * 51.2 ms at nominal 10 kHz before accounting for fill time. sig3_push_one
 * checks writable BEFORE advancing phase, so a full FIFO loses no samples.
 * Normal servicing continues between smaller arithmetic blocks. */
static void qs_prime_excitation(void) {
    if (!sig3_enable) return;
    for (unsigned i = 0; i < 512u; ++i) {
        if (!(main_ups_fifo_flags_read() & 2u)) break;
        sig3_push_one();
    }
    track3_service_background_budget(4u);
}

static int ref_fft_hann_log_estimate_mhz(const int16_t *samples,
                                               unsigned n,
                                               int64_t center_hz_milli_hint,
                                               unsigned half_width_hz,
                                               int64_t *vertex_hz_milli_out) {
    size_t cfg_need = 0;
    size_t cfg_len;
    kiss_fft_cfg cfg;
    unsigned bins;
    unsigned k_peak = 0u;
    uint64_t p_peak = 0u;
    unsigned k;
    unsigned k_lo, k_hi;
    int64_t x1, x2, x3;
    int64_t y1, y2, y3;
    int64_t den;

    if (vertex_hz_milli_out) *vertex_hz_milli_out = 0ll;

    if (!samples || !vertex_hz_milli_out || !is_pow2_u(n) || n < 8u || n > FFT_MAX_N || fft_fs_hz == 0u)
        return 0;

    qs_prime_excitation();

    /* Hann window, computed on the fly for whatever N is in use (rather than
     * a fixed-size precomputed table) so N=64/256/1024 get the same coverage
     * as N=2048. Bench-verified (LoopbackRetest.md dumps, offline analysis):
     * combined with the log-power parabola below, this cuts interpolation
     * bias from ~0.28-0.31 bin down to ~0.01-0.02 bin, consistently across
     * N=64..2048 and at both mid-bin and bin-boundary operating points. */
    for (k = 0u; k < n; k++) {
        double w = 0.5 - 0.5 * cos((2.0 * REF_FFT_PI * (double)k) / (double)(n - 1));
        double windowed = (double)samples[k] * w;
        fft_in[k].r = (kiss_fft_scalar)windowed;
        fft_in[k].i = (kiss_fft_scalar)0;
        track3_service_background_budget(4u);
    }

    for (unsigned reserve = 0; reserve < (n >= 2048u ? 16u : 1u); ++reserve) qs_prime_excitation();
    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES)
        return 0;

    cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    cfg = kiss_fft_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg)
        return 0;

    for (unsigned reserve = 0; reserve < (n >= 2048u ? 16u : 1u); ++reserve) qs_prime_excitation();
    kiss_fft(cfg, fft_in, fft_out);
    track3_service_background_budget(16u);

    bins = n / 2u;

    /* Bound the search to a window around the expected frequency when a hint
     * is available (steady-state tracking, where the last estimate is a
     * strong prior); fall back to a full-spectrum search only when there is
     * no prior (first call after (re)acquisition). An unbounded search during
     * tracking lets any unrelated, momentarily-stronger spectral component
     * (image, harmonic, filter leakage) hijack the peak pick even though the
     * real tone has barely moved since the previous update. */
    k_lo = 1u;
    k_hi = bins - 2u;
    if (center_hz_milli_hint > 0) {
        int64_t lo_hz_milli = center_hz_milli_hint - ((int64_t)half_width_hz * 1000ll);
        int64_t hi_hz_milli = center_hz_milli_hint + ((int64_t)half_width_hz * 1000ll);
        int64_t k_lo_s = (lo_hz_milli * (int64_t)n) / ((int64_t)fft_fs_hz * 1000ll);
        /* Include both bins bracketing the upper edge, also for N=64. */
        int64_t k_hi_s = (hi_hz_milli * (int64_t)n + (int64_t)fft_fs_hz * 1000ll - 1ll) /
                         ((int64_t)fft_fs_hz * 1000ll);
        if (k_lo_s < 1) k_lo_s = 1;
        if (k_hi_s > (int64_t)(bins - 2u)) k_hi_s = (int64_t)(bins - 2u);
        if (k_lo_s <= k_hi_s) {
            k_lo = (unsigned)k_lo_s;
            k_hi = (unsigned)k_hi_s;
        }
    }

    for (k = k_lo; k <= k_hi; k++) {
        trackq_service_background_sample(k);
        uint64_t pwr = fft_bin_power_at(k);
        if (pwr > p_peak) {
            p_peak = pwr;
            k_peak = k;
        }
    }

    if (k_peak == 0u || (k_peak + 1u) >= bins)
        return 0;

    y1 = (int64_t)fft_bin_power_at(k_peak - 1u);
    y2 = (int64_t)fft_bin_power_at(k_peak);
    y3 = (int64_t)fft_bin_power_at(k_peak + 1u);
    den = y1 - (2ll * y2) + y3;
    /* Equal adjacent maxima at a half-bin position are valid; a flat triplet
     * is not. Requiring strict dominance rejects a legitimate half-bin tone.
     * y1,y3 > 0 is required below for log(); a real FFT bin power is only
     * ever exactly zero for a pathological all-zero capture. */
    if (y2 < y1 || y2 < y3 || den >= 0ll || y1 <= 0ll || y3 <= 0ll)
        return 0;

    x1 = (int64_t)((((uint64_t)(k_peak - 1u) * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);
    x2 = (int64_t)((((uint64_t)k_peak * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);
    x3 = (int64_t)((((uint64_t)(k_peak + 1u) * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);

    {
        /* Fit the parabola to log(power), not raw power: a linear-power fit
         * assumes a mainlobe shape that neither a rectangular nor a Hann
         * window actually has, and is the confirmed source of the ~0.3-bin
         * bias measured before this change. log-power matches a windowed
         * mainlobe's real shape far better (bench-verified across N=64..2048
         * via LoopbackRetest.md's raw-dump analysis). */
        double ly1 = log((double)y1);
        double ly2 = log((double)y2);
        double ly3 = log((double)y3);
        double log_den = ly1 - 2.0 * ly2 + ly3;
        double log_num = (double)(x3 - x1) * (ly1 - ly3);
        /* Local-maximum geometry bounds the correction to half a bin. */
        int64_t half_bin_hz_milli = ((int64_t)fft_fs_hz * 1000ll) / (2ll * (int64_t)n);
        int64_t correction;
        if (!(log_den < 0.0))
            return 0;
        correction = (int64_t)(log_num / (4.0 * log_den));
        if (correction > half_bin_hz_milli)
            correction = half_bin_hz_milli;
        if (correction < -half_bin_hz_milli)
            correction = -half_bin_hz_milli;
        *vertex_hz_milli_out = x2 + correction;
    }
    return 1;
}

/* Convert mHz to milli-ppm. The 10% guard is an arithmetic bound, not a
 * resonance-lock or calibrated-temperature-range test. */
static int trackq_temp_ppm(int64_t measured_mhz, int64_t nominal_mhz,
                          int64_t *mppm) {
    int64_t delta;
    if (measured_mhz <= 0ll)
        return 0;
    delta = measured_mhz - nominal_mhz;
    if (delta < -(nominal_mhz / 10ll) || delta > nominal_mhz / 10ll)
        return 0;
    *mppm = (delta * 1000000000ll) / nominal_mhz;
    return 1;
}

/* Independent of reference validity. A100/C300 normalized raw-frequency
 * ratio, R=(raw_A/nom_A)/(raw_C/nom_C). With a=alpha*1e-6,
 * d=(R-1)/(a_A-R*a_C). No linearization of this ratio equation.
 * Q9 normalization introduces small fixed-point quantization only. */
static int trackq_temp_ratio_mc(int64_t raw_a100_mhz, int64_t raw_c300_mhz,
                               int64_t *delta_mc) {
    int64_t pa, pc, ratio_q9, den_q6;
    if (!trackq_temp_ppm(raw_a100_mhz, TRACKQ_TEMP_NOM_A100_MHZ, &pa) ||
        !trackq_temp_ppm(raw_c300_mhz, TRACKQ_TEMP_NOM_C300_MHZ, &pc))
        return 0;
    ratio_q9 = ((1000000000ll + pa) * 1000000000ll) / (1000000000ll + pc);
    den_q6 = TRACKQ_TEMP_ALPHA_A100_Q6 -
             (ratio_q9 * TRACKQ_TEMP_ALPHA_C300_Q6) / 1000000000ll;
    if (den_q6 > -1000000ll && den_q6 < 1000000ll)
        return 0;
    *delta_mc = ((ratio_q9 - 1000000000ll) * 1000000ll) / den_q6;
    return 1;
}

struct trackq_crystal_estimate {
    int64_t temp_abs_mc;
    int64_t clock_scale_q9;
    int64_t clock_offset_mppm;
    int64_t fs_mhz;
    uint32_t nco_inc;
    int64_t residual_mppm[TRACKQ_CHANNELS];
};

/* Evaluate x * scale / denominator without overflowing x * scale.
 * All inputs are positive. The result is rounded to nearest. */
static int64_t trackq_mul_div_pos_round(int64_t x, int64_t scale,
                                        int64_t denominator) {
    int64_t quotient = x / denominator;
    int64_t remainder = x % denominator;
    return quotient * scale +
           ((remainder * scale + denominator / 2ll) / denominator);
}

/* Joint raw-frequency model:
 *   z_i = raw_i / nominal_i = u + alpha_i * v,
 *   u = nominal_Fs / actual_Fs, v = u * (T - 25 C).
 * Existing calibrated joint rows yield v (mC) and u-1 (milli-ppm)
 * when applied to raw normalized deviations. No reference measurement is
 * used here. The returned NCO increment is diagnostic and is not written. */
static int trackq_crystal_estimate(const int64_t raw_mhz[TRACKQ_CHANNELS],
                                   struct trackq_crystal_estimate *result) {
    static const int64_t nominal_mhz[TRACKQ_CHANNELS] = {
        TRACKQ_TEMP_NOM_C300_MHZ, TRACKQ_TEMP_NOM_A100_MHZ,
        TRACKQ_TEMP_NOM_C100_MHZ
    };
    static const int64_t temp_q9[TRACKQ_CHANNELS] = {
        TRACKQ_TEMP_JOINT_C300_Q9, TRACKQ_TEMP_JOINT_A100_Q9,
        TRACKQ_TEMP_JOINT_C100_Q9
    };
    static const int64_t common_q9[TRACKQ_CHANNELS] = {
        TRACKQ_TEMP_COMMON_C300_Q9, TRACKQ_TEMP_COMMON_A100_Q9,
        TRACKQ_TEMP_COMMON_C100_Q9
    };
    static const int64_t alpha_q6[TRACKQ_CHANNELS] = {
        TRACKQ_TEMP_ALPHA_C300_Q6, TRACKQ_TEMP_ALPHA_A100_Q6,
        TRACKQ_TEMP_ALPHA_C100_Q6
    };
    int64_t p_mppm[TRACKQ_CHANNELS];
    int64_t v_mc;
    int64_t v_numerator = 0ll;
    int64_t common_numerator = 0ll;
    int64_t u_q9;
    int64_t tuning_numerator, tuning_quotient, tuning_remainder, tuning_q9;
    unsigned i;

    if (!result)
        return 0;
    for (i = 0u; i < TRACKQ_CHANNELS; ++i) {
        if (!trackq_temp_ppm(raw_mhz[i], nominal_mhz[i], &p_mppm[i]))
            return 0;
        v_numerator += temp_q9[i] * p_mppm[i];
        common_numerator += common_q9[i] * p_mppm[i];
    }
    v_mc = v_numerator / 1000000000ll;
    result->clock_offset_mppm = common_numerator / 1000000000ll;

    u_q9 = 1000000000ll + result->clock_offset_mppm;
    if (u_q9 <= 0ll)
        return 0;
    result->clock_scale_q9 = u_q9;
    result->temp_abs_mc = 25000ll + (v_mc * 1000000000ll) / u_q9;
    result->fs_mhz = trackq_mul_div_pos_round(
        (int64_t)TRACK3_RF_FS_HZ * 1000ll, 1000000000ll, u_q9);

    tuning_numerator = (int64_t)TRACKQ_NCO_TARGET_HZ * (1ll << 26);
    tuning_quotient = tuning_numerator / (int64_t)TRACK3_RF_FS_HZ;
    tuning_remainder = tuning_numerator % (int64_t)TRACK3_RF_FS_HZ;
    tuning_q9 = tuning_quotient * u_q9 +
        (tuning_remainder * u_q9 + (TRACK3_RF_FS_HZ / 2u)) /
        (int64_t)TRACK3_RF_FS_HZ;
    tuning_q9 = (tuning_q9 + 500000000ll) / 1000000000ll;
    if (tuning_q9 < 0ll || tuning_q9 >= (1ll << 26))
        return 0;
    result->nco_inc = (uint32_t)tuning_q9;

    for (i = 0u; i < TRACKQ_CHANNELS; ++i) {
        int64_t predicted_mppm = result->clock_offset_mppm +
            (alpha_q6[i] * v_mc) / 1000000ll;
        result->residual_mppm[i] = p_mppm[i] - predicted_mppm;
    }
    return 1;
}

/* Explicit sign preserves negative values between -1 and 0. Invalid
 * diagnostics have no numeric placeholder that might look like a reading. */
static void trackq_log_milli(const char *name, int valid, int64_t value,
                             const char *unit) {
    if (!valid) {
        printf(" %s=(na)", name);
        return;
    }
    printf(" %s=%s%ld.%03ld%s", name, value < 0ll ? "-" : "",
           (long)(llabs(value) / 1000ll), (long)(llabs(value) % 1000ll), unit);
}

/* TIMER0 advances independently of CPU performance-counter support.
 * Owned exclusively while ref_fft is active; restore previous configuration
 * on stop. Do not run another command that reprograms TIMER0 in this mode.
 */
static uint32_t ref_fft_timer_last;
static uint64_t ref_fft_timer_elapsed;
#if defined(CSR_TIMER0_BASE)
static uint32_t ref_fft_timer_saved_en, ref_fft_timer_saved_reload;
static uint32_t ref_fft_timer_saved_value, ref_fft_timer_saved_events;

static void ref_fft_timer_release(void) {
    timer0_en_write(0);
    timer0_reload_write(ref_fft_timer_saved_reload);
    timer0_load_write(ref_fft_timer_saved_value);
    timer0_en_write(ref_fft_timer_saved_en);
    timer0_ev_enable_write(ref_fft_timer_saved_events);
}
#endif

static uint64_t ref_fft_cycles(void) {
#if defined(CSR_TIMER0_BASE)
    uint32_t now;
    timer0_update_value_write(1);
    now = timer0_value_read();
    /* Unsigned subtraction handles the 32-bit down-counter wrap. Called
     * each poll, so no entire wrap (about 43 s at 100 MHz) is missed. */
    ref_fft_timer_elapsed += (uint32_t)(ref_fft_timer_last - now);
    ref_fft_timer_last = now;
    return ref_fft_timer_elapsed;
#else
    return 0ull;
#endif
}

static int ref_fft_timer_start(void) {
#if defined(CSR_TIMER0_BASE)
    ref_fft_timer_saved_en = timer0_en_read();
    ref_fft_timer_saved_reload = timer0_reload_read();
    ref_fft_timer_saved_events = timer0_ev_enable_read();
    timer0_update_value_write(1);
    ref_fft_timer_saved_value = timer0_value_read();
    timer0_ev_enable_write(0);
    timer0_en_write(0);
    timer0_reload_write(0xffffffffu);
    timer0_load_write(0xffffffffu);
    timer0_en_write(1);
    timer0_update_value_write(1);
    ref_fft_timer_last = timer0_value_read();
    ref_fft_timer_elapsed = 0ull;
    for (unsigned i = 0u; i < 1024u; ++i) {
        if (ref_fft_cycles() != 0ull)
            return 1;
    }
    ref_fft_timer_release();
#endif
    puts("ref_fft: TIMER0 unavailable or not advancing; measurement not started");
    return 0;
}

static int capture_ref_fft_samples(void) {
    unsigned discarded = 0u;
    /* Discard backlog from the preceding idle/FFT period. Read only Y_ref;
     * popping still consumes an entire frame. Bounded to avoid hanging. */
    while (main_ds_fifo_flags_read() & 0x1u) {
        main_ds_fifo_pop_write(1);
        (void)main_ds_fifo_yref_read();
        track3_service_background_budget(4u);
        if (++discarded >= 65536u) {
            puts("ref_fft: could not drain FIFO; measurement invalid");
            return 0;
        }
    }
    main_ds_fifo_clear_write(1);
    for (unsigned i = 0u; i < 2048u; ++i) {
        if (!track3_wait_ds_fifo("ref_fft capture", i, 2048u))
            return 0;
        main_ds_fifo_pop_write(1);
        track_samples_ref[i] = (int16_t)(main_ds_fifo_yref_read() & 0xffffu);
        track3_service_background_budget(4u);
    }
    /* A gap/overflow invalidates the record; do not FFT discontinuous data. */
    if (main_ds_fifo_overflow_read() || main_ds_fifo_underflow_read()) {
        puts("ref_fft: FIFO overflow/underflow during capture; measurement invalid");
        return 0;
    }
    return 1;
}

/* Time-domain comparator: fit rising zero-crossing sample positions against
 * crossing ordinal. No FFT/interpolation of spectral bins and no extra buffer.
 * Mean subtraction only; periods are checked, not selectively discarded. */
struct ref_zc_diag {
    uint32_t crossings, flags;
    int32_t min_sample, max_sample, min_period_msamples, max_period_msamples;
};
static int ref_zc_estimate_mhz(const int16_t *samples, unsigned n,
                              int64_t *frequency_mhz, struct ref_zc_diag *d) {
    memset(d, 0, sizeof(*d));
    *frequency_mhz = 0ll;
    if (!samples || n < 32u || n > FFT_MAX_N || fft_fs_hz == 0u) { d->flags=1u; return 0; }
    int32_t sum=0;
    d->min_sample=32767; d->max_sample=-32768;
    for (unsigned i=0; i<n; ++i) {
        int32_t x=samples[i];
        sum+=x;
        if (x<d->min_sample) d->min_sample=x;
        if (x>d->max_sample) d->max_sample=x;
        trackq_service_background_sample(i);
    }
    if (d->max_sample-d->min_sample < 8) { d->flags=2u; return 0; }
    double mean=(double)sum/(double)n;
    double sum_k=0.0, sum_t=0.0, sum_kk=0.0, sum_kt=0.0;
    double previous_crossing=0.0, min_period=1e9, max_period=0.0;
    double previous=(double)samples[0]-mean;
    for (unsigned i=1; i<n; ++i) {
        double current=(double)samples[i]-mean;
        if (previous<0.0 && current>=0.0) {
            double t=(double)(i-1u)-previous/(current-previous);
            double k=(double)d->crossings;
            sum_k+=k; sum_t+=t; sum_kk+=k*k; sum_kt+=k*t;
            if (d->crossings) {
                double period=t-previous_crossing;
                if (period<min_period) min_period=period;
                if (period>max_period) max_period=period;
            }
            previous_crossing=t;
            ++d->crossings;
        }
        previous=current;
        trackq_service_background_sample(i);
    }
    if (d->crossings<8u) { d->flags|=1u; return 0; }
    d->min_period_msamples=(int32_t)(min_period*1000.0+0.5);
    d->max_period_msamples=(int32_t)(max_period*1000.0+0.5);
    if (!(min_period>0.0) || max_period>1.25*min_period) d->flags|=4u;
    double count=(double)d->crossings;
    double den=count*sum_kk-sum_k*sum_k;
    double period=(count*sum_kt-sum_k*sum_t)/den;
    if (!(den>0.0 && period>2.0 && period<(double)n)) { d->flags|=8u; return 0; }
    double f=(double)fft_fs_hz/period;
    *frequency_mhz=(int64_t)(f*1000.0+0.5);
    /* Period-spread flag retains the estimate for diagnosis, not servo use. */
    return 1;
}

static int ref_reconstruct_fs(uint32_t ref_lo, int64_t ref_mhz, double *fs) {
    if (ref_mhz<=0ll || ref_mhz>=5000000ll || ref_lo==0u) return 0;
    double nominal=(double)TRACK3_RF_FS_HZ;
    double denominator=(double)ref_lo*nominal/(double)(1ul<<26)+(double)ref_mhz/1000.0;
    *fs=nominal*(double)TRACKQ_REF_INPUT_HZ/denominator;
    return *fs>64000000.0 && *fs<66000000.0;
}

static void ref_fft_step(void) {
    uint64_t started, finished, elapsed;
    int64_t h64_mhz = 0ll, h2048_mhz = 0ll;
    int hvalid64 = 0, hvalid2048 = 0;
    int64_t zc_mhz=0ll;
    int zvalid=0;
    struct ref_zc_diag zdiag={0};
    zdiag.flags=1u;
    if (!ref_fft_enabled)
        return;
    started = ref_fft_cycles();
    if (started < ref_fft_next)
        return;
    ref_fft_next += (uint64_t)CONFIG_CLOCK_FREQUENCY;
    if (capture_ref_fft_samples()) {
        /* Symmetric Hann windows share sample center 1023.5:
         * FFT64 uses indices 992..1055; FFT2048 uses 0..2047. */
        hvalid64 = ref_fft_hann_log_estimate_mhz(track_samples_ref + (2048u - 64u) / 2u, 64u, 0ll, 0u, &h64_mhz);
        track3_service_background_budget(16u);
        hvalid2048 = ref_fft_hann_log_estimate_mhz(track_samples_ref, 2048u, 0ll, 0u, &h2048_mhz);
        track3_service_background_budget(16u);
        zvalid=ref_zc_estimate_mhz(track_samples_ref,2048u,&zc_mhz,&zdiag);
        track3_service_background_budget(16u);
    }
    finished = ref_fft_cycles();
    elapsed = finished - started;
    printf("ref_fft: seq=%lu t_ms=%lu", (unsigned long)++ref_fft_sequence,
           (unsigned long)((started - ref_fft_origin) / (CONFIG_CLOCK_FREQUENCY / 1000u)));
    trackq_log_milli("ref64_center_hann_log", hvalid64, h64_mhz, "Hz");
    trackq_log_milli("ref2048_hann_log", hvalid2048, h2048_mhz, "Hz");
    trackq_log_milli("diff_center64_2048", hvalid64 && hvalid2048, h64_mhz - h2048_mhz, "Hz");
    trackq_log_milli("ref_zc2048", zvalid, zc_mhz, "Hz");
    trackq_log_milli("diff_zc_fft2048", zvalid && hvalid2048, zc_mhz-h2048_mhz, "Hz");
    uint32_t ref_lo=main_phase_inc_down_ref_read();
    double fs64=0.0, fs2048=0.0, fszc=0.0;
    int fv64=hvalid64 && fft_fs_hz==10000u && ref_reconstruct_fs(ref_lo,h64_mhz,&fs64);
    int fv2048=hvalid2048 && fft_fs_hz==10000u && ref_reconstruct_fs(ref_lo,h2048_mhz,&fs2048);
    int fvzc=zvalid && !zdiag.flags && fft_fs_hz==10000u && ref_reconstruct_fs(ref_lo,zc_mhz,&fszc);
    trackq_log_milli("fs64",fv64,(int64_t)(fs64*1000.0+0.5),"Hz");
    trackq_log_milli("fs2048",fv2048,(int64_t)(fs2048*1000.0+0.5),"Hz");
    trackq_log_milli("fs_zc",fvzc,(int64_t)(fszc*1000.0+0.5),"Hz");
    printf(" zc_crossings=%lu zc_flags=%lu zc_min=%ld zc_max=%ld period_min_msamples=%ld period_max_msamples=%ld reflo=%lu",
        (unsigned long)zdiag.crossings,(unsigned long)zdiag.flags,(long)zdiag.min_sample,
        (long)zdiag.max_sample,(long)zdiag.min_period_msamples,(long)zdiag.max_period_msamples,
        (unsigned long)ref_lo);
    printf(" work_ms=%lu fs_nom=%luHz\n",
           (unsigned long)(elapsed / (CONFIG_CLOCK_FREQUENCY / 1000u)),
           (unsigned long)fft_fs_hz);
    /* Keep a 1-second start grid; skip missed slots rather than print bursts.
     * t_ms/work_ms expose any processing or console-induced overruns. */
    finished = ref_fft_cycles();
    if (finished >= ref_fft_next) {
        uint64_t missed = (finished - ref_fft_next) / CONFIG_CLOCK_FREQUENCY + 1ull;
        ref_fft_next += missed * CONFIG_CLOCK_FREQUENCY;
        printf("ref_fft: skipped_slots=%lu (processing exceeded schedule)\n", (unsigned long)missed);
    }
}

static void cmd_ref_fft_start(char *args) {
    if (trackq_hybrid_enabled) { puts("Run trackq_hybrid 0 before ref_fft_start"); return; }
    (void)args;
    if (ref_fft_enabled) {
        puts("ref_fft already running; use ref_fft_stop before restarting");
        return;
    }
    if (fft_fs_hz == 0u) {
        puts("ref_fft: fft_fs must be > 0");
        return;
    }
    if (!ref_fft_timer_start())
        return;
    /* Reuse existing static sample/FFT buffers; tracking must be stopped. */
    for (unsigned i = 0u; i < TRACKQ_CHANNELS; ++i)
        trackq[i].enabled = 0;
    ref_fft_sequence = 0u;
    ref_fft_origin = ref_fft_cycles();
    ref_fft_next = ref_fft_origin;
    ref_fft_enabled = 1;
    puts("ref_fft_start: trackq stopped; Y_ref Hann/log FFT64(center)/FFT2048 and time-domain rising crossings ZC2048 from one fresh record; fs reconstructed separately; zc_flags 1=few/invalid record 2=small range 4=period spread >25% 8=invalid slope; 1 s target period via TIMER0; LO/NCO unchanged");
}

static void cmd_ref_fft_stop(char *args) {
    (void)args;
#if defined(CSR_TIMER0_BASE)
    if (ref_fft_enabled)
        ref_fft_timer_release();
#endif
    ref_fft_enabled = 0;
    puts("ref_fft_stop: reference measurement disabled");
}

/* Add a signed frequency change directly to the existing 26-bit increment.
 * remainder is in Hz * 2^26 units, with |remainder| < nominal Fs.
 * Keep the unrepresentable fraction for the next nonzero correction.
 * A zero request preserves the exact previous increment (no Hz round trip).
 */
static uint32_t trackq_adjust_increment(unsigned channel, uint32_t previous,
                                        int32_t correction_hz, int *limited) {
    int64_t numerator, step, next;
    *limited = 0;
    if (correction_hz == 0)
        return previous;
    numerator = (int64_t)correction_hz * (1ll << 26) + trackq_inc_remainder[channel];
    step = numerator / (int64_t)TRACK3_RF_FS_HZ;
    next = (int64_t)previous + step;
    if (next < 0ll || next > ((1ll << 26) - 1ll)) {
        *limited = 1;
        trackq_inc_remainder[channel] = 0ll;
        return next < 0ll ? 0u : ((1u << 26) - 1u);
    }
    trackq_inc_remainder[channel] = numerator - step * (int64_t)TRACK3_RF_FS_HZ;
    return (uint32_t)next;
}

/* Observer v1: read-only measurements, emitted after the control commit.
 * Tick stamps describe CPU capture boundaries (including settle), NOT source
 * sample timestamps. FIFO backlog/transport delay is not measured here. */
static int trackq_observer_enabled;
static uint32_t trackq_observer_session, trackq_observer_capture_id;
static uint32_t trackq_observer_last_tick;
static uint64_t trackq_observer_tick_total;

struct trackq_observer_mode {
    uint32_t lo_inc, tone_inc[3], center_hz, delta_hz;
    int amplitude, tone_enabled, due, have_power, fit_ok, limited;
    uint64_t power[3];
    int64_t drive_mhz, requested_mhz, demand_mhz;
    int64_t applied_increment;
    const char *controller_mode;
    const char *fit_status;
};
static struct {
    uint32_t id, ref_lo_inc, nco_before, n, fs_hz;
    uint64_t begin_tick, end_tick;
    unsigned fifo_before, fifo_after, underflow_after;
    struct trackq_observer_mode mode[TRACKQ_CHANNELS];
} trackq_observer;

static uint64_t trackq_observer_ticks(void) {
    uint32_t now = ce_ticks;
    trackq_observer_tick_total += (uint32_t)(now - trackq_observer_last_tick);
    trackq_observer_last_tick = now;
    return trackq_observer_tick_total;
}

static void trackq_observer_begin(unsigned n) {
    memset(&trackq_observer, 0, sizeof(trackq_observer));
    trackq_observer.id = ++trackq_observer_capture_id;
    trackq_observer.n = n;
    trackq_observer.fs_hz = fft_fs_hz;
    trackq_observer.ref_lo_inc = main_phase_inc_down_ref_read();
    trackq_observer.nco_before = main_phase_inc_nco_read();
    trackq_observer.fifo_before = main_ds_fifo_overflow_read();
    for (unsigned i = 0; i < TRACKQ_CHANNELS; ++i) {
        struct trackq_observer_mode *m = &trackq_observer.mode[i];
        m->lo_inc = phase_down_read(i);
        m->center_hz = trackq[i].center_hz;
        m->delta_hz = trackq[i].delta_hz;
        m->amplitude = sig3_amp[i];
        m->tone_enabled = sig3_enable && sig3_channel_enable[i];
        for (unsigned j = 0; j < 3; ++j) m->tone_inc[j] = sig3_inc[i][j];
        m->drive_mhz = uc_phase_inc_to_mhz(m->lo_inc, TRACK3_RF_FS_HZ) +
                      sig3_phase_inc_to_mhz(m->tone_inc[1], 10000u);
        m->controller_mode = "not_due";
        m->fit_status = "not_due";
    }
    trackq_observer.begin_tick = trackq_observer_ticks();
}

/* Split before conversion: the shifted half always fits int64_t.
 * This avoids a __floatundidf dependency absent from the LiteX runtime. */
static double trackq_observer_power_double(uint64_t power) {
    return (double)(int64_t)(power >> 1) * 2.0 +
           (double)(int64_t)(power & 1u);
}

static void trackq_observer_emit(unsigned iteration,
    const int64_t *raw_mhz, const uint8_t *raw_valid,
    int ref_valid, int64_t ref_bb_mhz, int fs_valid, int64_t fs_ref_mhz,
    uint32_t nco_ref, int crystal_valid, const struct trackq_crystal_estimate *crystal,
    int temp_valid, int64_t existing_mc, int64_t joint_mc,
    int ratio_valid, int64_t ratio_mc) {
    uint64_t processing_tick;
    /* Complete power diagnostics for channels skipped by the scheduler.
     * This is deliberately after uc_commit, with no controller state writes. */
    for (unsigned i = 0; i < TRACKQ_CHANNELS; ++i) {
        struct trackq_observer_mode *m = &trackq_observer.mode[i];
        if (!m->have_power && m->center_hz > m->delta_hz) {
            m->power[0] = track_band_power_at_hz_samples(track_samples[i], m->center_hz - m->delta_hz, trackq_observer.n);
            m->power[1] = track_band_power_at_hz_samples(track_samples[i], m->center_hz, trackq_observer.n);
            m->power[2] = track_band_power_at_hz_samples(track_samples[i], m->center_hz + m->delta_hz, trackq_observer.n);
            m->have_power = 1;
        }
    }
    processing_tick = trackq_observer_ticks();
    printf("trackq observer capture: v=1 session=%lu capture_id=%lu iteration=%u n=%lu fs_nom_hz=%lu rf_fs_nom_hz=%u tone_fs_nom_hz=10000 tick_hz=10000 capture_begin_tick=%llu capture_end_tick=%llu processing_end_tick=%llu ref_lo_inc=%lu nco_before=%lu fifo_ov_before=%u fifo_ov_after=%u fifo_uf_after=%u estimator=hann_log64_centered power=band3\n",
        (unsigned long)trackq_observer_session, (unsigned long)trackq_observer.id,
        iteration, (unsigned long)trackq_observer.n, (unsigned long)trackq_observer.fs_hz,
        TRACK3_RF_FS_HZ, (unsigned long long)trackq_observer.begin_tick,
        (unsigned long long)trackq_observer.end_tick, (unsigned long long)processing_tick,
        (unsigned long)trackq_observer.ref_lo_inc, (unsigned long)trackq_observer.nco_before,
        trackq_observer.fifo_before, trackq_observer.fifo_after, trackq_observer.underflow_after);
    for (unsigned i = 0; i < TRACKQ_CHANNELS; ++i) {
        struct trackq_observer_mode *m = &trackq_observer.mode[i];
        double l = trackq_observer_power_double(m->power[0]);
        double c = trackq_observer_power_double(m->power[1]);
        double r = trackq_observer_power_double(m->power[2]);
        double den = l - 2.0 * c + r;
        double detuning = 0.0;
        const char *status = "ok";
        int64_t side_q9 = (l + r > 0.0) ? (int64_t)(1e9 * (r - l) / (r + l)) : 0ll;
        if (!m->tone_enabled) status = "tone_disabled";
        else if (!m->have_power || l + c + r == 0.0) status = "no_power";
        else if (c <= l || c <= r) status = "not_local_max";
        else if (!(den < 0.0)) status = "not_concave";
        else {
            detuning = (double)m->delta_hz * 1000.0 * (l-r) / (2.0*den);
            if (!isfinite(detuning) || fabs(detuning) > (double)m->delta_hz*1000.0)
                status = "out_of_span";
        }
        printf("trackq observer mode: v=1 session=%lu capture_id=%lu iteration=%u channel=%u lo_inc_before=%lu tone_inc_left=%lu tone_inc_center=%lu tone_inc_right=%lu amplitude=%d tone_enabled=%d center_hz=%lu delta_hz=%lu controller_due=%d P_left=%llu P_center=%llu P_right=%llu side_error_q9=%lld side_valid=%d detuning_status=%s",
            (unsigned long)trackq_observer_session, (unsigned long)trackq_observer.id,
            iteration, i+1u, (unsigned long)m->lo_inc, (unsigned long)m->tone_inc[0],
            (unsigned long)m->tone_inc[1], (unsigned long)m->tone_inc[2], m->amplitude,
            m->tone_enabled, (unsigned long)m->center_hz, (unsigned long)m->delta_hz,
            m->due, (unsigned long long)m->power[0], (unsigned long long)m->power[1],
            (unsigned long long)m->power[2], (long long)side_q9, l+r > 0.0, status);
        if (!strcmp(status,"ok")) printf(" detuning_quad_raw_mHz=%lld", (long long)detuning);
        else printf(" detuning_quad_raw_mHz=(na)");
        printf(" drive_raw_mHz=%lld controller_mode=%s requested_mHz=%lld demand_mHz=%lld applied_increment=%lld limited=%d modal_fit_status=%s",
            (long long)m->drive_mhz, m->controller_mode, (long long)m->requested_mhz,
            (long long)m->demand_mhz, (long long)m->applied_increment, m->limited, m->fit_status);
        if (raw_valid[i]) printf(" raw_mode_frequency_mHz=%lld", (long long)raw_mhz[i]);
        else printf(" raw_mode_frequency_mHz=(na)");
        printf("\n");
    }
    printf("trackq observer result: v=1 session=%lu capture_id=%lu iteration=%u",
        (unsigned long)trackq_observer_session, (unsigned long)trackq_observer.id, iteration);
    trackq_log_milli("ref_bb",ref_valid,ref_bb_mhz,"Hz");
    trackq_log_milli("fs_ref",fs_valid,fs_ref_mhz,"Hz");
    trackq_log_milli("fs_crystal",crystal_valid,crystal->fs_mhz,"Hz");
    trackq_log_milli("temp_raw_joint",crystal_valid,crystal->temp_abs_mc,"C");
    trackq_log_milli("dtemp_existing",temp_valid,existing_mc,"C");
    trackq_log_milli("dtemp_joint",temp_valid,joint_mc,"C");
    trackq_log_milli("dtemp_ratio",ratio_valid,ratio_mc,"C");
    if (fs_valid) printf(" nco_inc_ref=%lu",(unsigned long)nco_ref);
    else printf(" nco_inc_ref=(na)");
    if (crystal_valid) printf(" nco_inc_crystal=%lu",(unsigned long)crystal->nco_inc);
    else printf(" nco_inc_crystal=(na)");
    printf("\n");
    /* Stamp after payload output. UART software/hardware queues may still drain;
     * this is not a timestamp of the final serial bit on the wire. */
    printf("trackq observer end: v=1 session=%lu capture_id=%lu iteration=%u logging_end_tick=%llu\n",
        (unsigned long)trackq_observer_session, (unsigned long)trackq_observer.id,
        iteration, (unsigned long long)trackq_observer_ticks());
}

/* Shadow-only synthesis experiment. Timing uses TIMER0 at nominal CPU rate,
 * independent of interrupt servicing but still board-clock derived. Never writes an output CSR. */
static struct {
    int ready;
    double slow_hz;
    uint32_t last_good, last_log, started, accepted, rejected;
} trackq_hybrid;

static void trackq_hybrid_reset(void) {
    memset(&trackq_hybrid, 0, sizeof(trackq_hybrid));
    trackq_hybrid.started = trackq_hybrid.last_log = (uint32_t)(ref_fft_cycles() / (CONFIG_CLOCK_FREQUENCY/1000u));
}

static void trackq_hybrid_step(int modal_ok, int ref_ok, const int64_t raw_modes_mhz[TRACKQ_CHANNELS],
        int64_t temp_mc, int64_t ref_bb_mhz, uint32_t ref_lo,
        uint32_t fifo_flags, unsigned modal_mask, uint32_t applied_inc) {
    if (!trackq_hybrid_enabled) return;
    const int64_t raw_mhz = raw_modes_mhz[TRACKQ_C300_CHANNEL];
    uint32_t now = (uint32_t)(ref_fft_cycles() / (CONFIG_CLOCK_FREQUENCY/1000u));
    uint32_t age = now - trackq_hybrid.last_good;
    double fs = 0.0, comp = 0.0;
    int fresh = 0, candidate_ok = 0;
    uint32_t candidate = 0, ref_round = 0;
    if (ref_ok) {
        double ref_sum = (double)ref_lo * (double)TRACK3_RF_FS_HZ / 67108864.0
                      + (double)ref_bb_mhz / 1000.0;
        if (ref_sum > 9000000.0 && ref_sum < 11000000.0)
            fs = (double)TRACK3_RF_FS_HZ * TRACKQ_REF_INPUT_HZ / ref_sum;
    }
    if (modal_ok && fs > 0.0 && !fifo_flags) {
        double g = 1.0 + (double)TRACKQ_TEMP_ALPHA_C300_Q6 *
                   (double)(temp_mc - 25000ll) / 1.0e15;
        if (g > 0.9 && g < 1.1 && raw_mhz > 0ll) {
            comp = ((double)raw_mhz / 1000.0) * fs / TRACK3_RF_FS_HZ / g;
            double nominal = (double)TRACKQ_TEMP_NOM_C300_MHZ / 1000.0;
            if (comp > nominal*0.9 && comp < nominal*1.1) {
                /* Restart after >10 s with no accepted modal observation.
                 * Backward-Euler low-pass: a=dt/(tau+dt), tau=30 s. */
                if (!trackq_hybrid.ready || age > 10000u)
                    trackq_hybrid.slow_hz = comp;
                else {
                    double a = (double)age / (30000.0 + (double)age);
                    trackq_hybrid.slow_hz += a*(comp-trackq_hybrid.slow_hz);
                }
                trackq_hybrid.ready = fresh = 1;
                trackq_hybrid.last_good = now;
                age = 0u;
            }
        }
    }
    if (fresh) ++trackq_hybrid.accepted;
    else ++trackq_hybrid.rejected;
    if (fs > 0.0 && !fifo_flags && trackq_hybrid.ready && age <= 10000u) {
        double target = TRACKQ_NCO_TARGET_HZ * trackq_hybrid.slow_hz /
                        ((double)TRACKQ_TEMP_NOM_C300_MHZ / 1000.0);
        double inc = 67108864.0 * target / fs;
        if (inc > 0.0 && inc < 33554431.0) {
            candidate = (uint32_t)(inc + 0.5);
            ref_round = (uint32_t)(67108864.0 * TRACKQ_NCO_TARGET_HZ / fs + 0.5);
            candidate_ok = 1;
        }
    }
    if ((uint32_t)(now-trackq_hybrid.last_log) < 5000u) return;
    trackq_hybrid.last_log = now;
    printf("HY2 t_ms=%lu fresh=%d valid=%d age_ms=%lu flags=%lu ok=%lu reject=%lu",
        (unsigned long)((uint32_t)(now-trackq_hybrid.started)), fresh, candidate_ok,
        (unsigned long)(trackq_hybrid.ready ? age : 0xffffffffu),
        (unsigned long)fifo_flags, (unsigned long)trackq_hybrid.accepted,
        (unsigned long)trackq_hybrid.rejected);
    unsigned reason = (!modal_ok ? 1u : 0u) | (!ref_ok || fs <= 0.0 ? 2u : 0u) |
                      (fifo_flags ? 4u : 0u) |
                      (!trackq_hybrid.ready || age > 10000u ? 8u : 0u);
    if (modal_ok && fs > 0.0 && !fifo_flags && !fresh) reason |= 16u;
    printf(" modes=%u reason=%u pre=%lu cap=%lu proc=%lu", modal_mask, reason,
        (unsigned long)hybrid_fifo_pre, (unsigned long)hybrid_fifo_capture,
        (unsigned long)hybrid_fifo_process);
    /* Same capture, nominal RF scale; never print stale fallbacks as valid. */
    trackq_log_milli("raw_C300", (modal_mask & 1u) != 0u, raw_modes_mhz[0], "Hz");
    trackq_log_milli("raw_A100", (modal_mask & 2u) != 0u, raw_modes_mhz[1], "Hz");
    trackq_log_milli("raw_C100", (modal_mask & 4u) != 0u, raw_modes_mhz[2], "Hz");
    trackq_log_milli("T", fresh, temp_mc, "C");
    trackq_log_milli("C25", fresh, (int64_t)(comp*1000.0), "Hz");
    trackq_log_milli("slow", trackq_hybrid.ready,
        (int64_t)(trackq_hybrid.slow_hz*1000.0), "Hz");
    trackq_log_milli("fs", fs > 0.0, (int64_t)(fs*1000.0), "Hz");
    if (candidate_ok) printf(" cand=%lu ref_round=%lu", (unsigned long)candidate,
        (unsigned long)ref_round);
    else printf(" cand=(na) ref_round=(na)");
    printf(" applied=%lu shadow=1\n", (unsigned long)applied_inc);
    trackq_hybrid.accepted = trackq_hybrid.rejected = 0u;
}

static void cmd_trackq_hybrid(char *args) {
    char *v = strtok(args, " \t");
    if (!v || strtok(NULL, " \t") || (strcmp(v,"0") && strcmp(v,"1"))) {
        puts("Usage: trackq_hybrid <0|1> (shadow only, tau=30 s)"); return;
    }
    if (trackq_hybrid_enabled) {
#if defined(CSR_TIMER0_BASE)
        ref_fft_timer_release();
#endif
        trackq_hybrid_enabled = 0;
    }
    if (v[0] == '1' && (ref_fft_enabled || !ref_fft_timer_start())) {
        puts("HY: stop ref_fft first; TIMER0 required"); return;
    }
    trackq_hybrid_enabled = v[0] == '1';
    if (trackq_hybrid_enabled) trackq_hybrid_reset();
    printf("HY config enabled=%d tau_ms=30000 stale_ms=10000 log_ms=5000 shadow=1 timing=TIMER0 nominal_tick_hz=1000 raw_modes=1\n", trackq_hybrid_enabled);
}

static void trackq_step(void) {
    if (trackq_hybrid_enabled) (void)ref_fft_cycles();
    unsigned i;
    unsigned capture_n = 0u;
    unsigned capture_settle = TRACK3_DEFAULT_SETTLE;
    int any_due = 0;
    int any_enabled = 0;
    int64_t bin_vertex_hf_mhz_log[TRACKQ_CHANNELS];
    int64_t corr_vertex_hf_mhz_log[TRACKQ_CHANNELS];
    int64_t ref_bin_vertex_baseband_mhz_log = (int64_t)TRACK3_DEFAULT_CENTER_HZ * 1000ll;
    int64_t ref_real_fs_mhz_log = (int64_t)TRACK3_RF_FS_HZ * 1000ll;
    int64_t temp_delta_mc_log = 0ll;
    int64_t temp_joint_mc_log = 0ll;
    int64_t temp_common_mppm_log = 0ll;
    int64_t temp_ratio_mc_log = 0ll;
    struct trackq_crystal_estimate crystal_log = {0};
    uint32_t ref_nco_inc_log = 0u;
    uint8_t fs_valid_log = 0u;
    uint8_t temp_ratio_valid_log = 0u;
    uint8_t bin_vertex_valid_log[TRACKQ_CHANNELS] = {0u, 0u, 0u};
    uint8_t ref_bin_vertex_valid_log = 0u;
    uint8_t temp_delta_valid_log = 0u;
    uint8_t crystal_valid_log = 0u;

    for (i = 0; i < TRACKQ_CHANNELS; i++) {
        if (!trackq[i].enabled)
            continue;
        any_enabled = 1;
        if (fft_fs_hz == 0u) {
            puts("trackq stopped: fft_fs must be > 0");
            trackq[i].enabled = 0;
            continue;
        }
        if (trackq[i].center_hz <= trackq[i].delta_hz) {
            puts("trackq stopped: center_hz must exceed delta_hz");
            trackq[i].enabled = 0;
            continue;
        }
        if (ce_ticks >= trackq[i].next_tick) {
            any_due = 1;
            if (capture_n == 0u) {
                capture_n = trackq[i].n;
                capture_settle = trackq[i].settle;
            }
        }
    }

    if (!any_enabled || !any_due)
        return;

    if (trackq_observer_enabled) trackq_observer_begin(capture_n);

    if (trackq_hybrid_enabled && !hybrid_fresh_capture_prepare()) {
        puts("HY capture failed: backlog drain limit; tracking stopped");
        for (i = 0; i < TRACKQ_CHANNELS; i++) trackq[i].enabled = 0;
        return;
    }
    if (!capture_ds_track_multi(capture_n, capture_settle)) {
        for (i = 0; i < TRACKQ_CHANNELS; i++)
            trackq[i].enabled = 0;
        return;
    }

    if (trackq_hybrid_enabled) {
        hybrid_fifo_capture = hybrid_fifo_flags();
    }

    if (trackq_observer_enabled) {
        trackq_observer.end_tick = trackq_observer_ticks();
        trackq_observer.fifo_after = main_ds_fifo_overflow_read();
        trackq_observer.underflow_after = main_ds_fifo_underflow_read();
    }
    if (trackq_hybrid_enabled) hybrid_fifo_boundary();

    for (i = 0; i < TRACKQ_CHANNELS; i++) {
        uint64_t left_pwr;
        uint64_t center_pwr;
        uint64_t right_pwr;
        int64_t num;
        int64_t den;
        int64_t center_hz_milli;
        int64_t center_base_hz_milli;
        int64_t h_hz_milli;
        int64_t vertex_hz_milli;
        int64_t phase_hz_milli;
        int64_t bin_vertex_hz_milli;
        int32_t correction_hz;
        int limited;
        uint32_t next_phase_inc;
        uint32_t phase_inc;
        int32_t error_mhz;
        int64_t filt_delta_mhz;
        int64_t ctrl_mhz;
        int64_t ctrl_limit_mhz;
        int demand_limited;
        int confident;
        int weak_mode;

        center_base_hz_milli = trackq_center_tone_hz_milli(i);
        center_hz_milli = (int64_t)trackq[i].center_hz * 1000ll;
        bin_vertex_hf_mhz_log[i] = uc_phase_inc_to_mhz(phase_down_read(i), TRACK3_RF_FS_HZ) +
                                   center_base_hz_milli;
        corr_vertex_hf_mhz_log[i] = bin_vertex_hf_mhz_log[i];
        if (!trackq[i].enabled || ce_ticks < trackq[i].next_tick ||
            trackq[i].n != capture_n)
            continue;

        left_pwr = track_band_power_at_hz_samples(track_samples[i], trackq[i].center_hz - trackq[i].delta_hz, trackq[i].n);
        center_pwr = track_band_power_at_hz_samples(track_samples[i], trackq[i].center_hz, trackq[i].n);
        right_pwr = track_band_power_at_hz_samples(track_samples[i], trackq[i].center_hz + trackq[i].delta_hz, trackq[i].n);

        if (trackq_observer_enabled) {
            struct trackq_observer_mode *m = &trackq_observer.mode[i];
            m->due = 1;
            m->have_power = 1;
            m->power[0] = left_pwr;
            m->power[1] = center_pwr;
            m->power[2] = right_pwr;
        }

        vertex_hz_milli = center_hz_milli;
        correction_hz = 0;
        error_mhz = 0;
        weak_mode = 0;

        confident = trackq_vertex_confident(left_pwr, center_pwr, right_pwr);
        if (confident) {
            num = (int64_t)left_pwr - (int64_t)right_pwr;
            den = 2ll * ((int64_t)left_pwr - (2ll * (int64_t)center_pwr) + (int64_t)right_pwr);
            if (den < 0ll) {
                h_hz_milli = (int64_t)trackq[i].delta_hz * 1000ll;
                vertex_hz_milli = center_hz_milli + ((h_hz_milli * num) / den);
                error_mhz = (int32_t)(vertex_hz_milli - center_hz_milli);
            }
        } else {
            error_mhz = trackq_side_error_mhz(left_pwr, right_pwr, trackq[i].delta_hz, trackq_deadband_pct[i]);
            if (error_mhz != 0)
                weak_mode = 1;
            else
                trackq[i].filt_error_mhz =
                    (trackq[i].filt_error_mhz * (TRACKQ_ERR_ALPHA_DEN - TRACKQ_ERR_ALPHA_NUM)) / TRACKQ_ERR_ALPHA_DEN;
        }

        filt_delta_mhz = ((int64_t)(error_mhz - trackq[i].filt_error_mhz) * (int64_t)TRACKQ_ERR_ALPHA_NUM) /
                         (int64_t)TRACKQ_ERR_ALPHA_DEN;
        trackq[i].filt_error_mhz += (int32_t)filt_delta_mhz;

        ctrl_mhz = ((int64_t)trackq[i].filt_error_mhz * (int64_t)TRACKQ_KP_NUM) / (int64_t)TRACKQ_KP_DEN;
        /* Clamp demand first: retain only sub-Hz quantization residue,
         * never a backlog rejected by the step limit. The separate NCO
         * remainder continues to preserve sub-LSB precision. */
        ctrl_limit_mhz = 1000ll * (weak_mode ?
            trackq_clamp_weak_step_hz(TRACKQ_MAX_STEP_HZ) : TRACKQ_MAX_STEP_HZ);
        demand_limited = (ctrl_mhz > ctrl_limit_mhz || ctrl_mhz < -ctrl_limit_mhz);
        if (ctrl_mhz > ctrl_limit_mhz)
            ctrl_mhz = ctrl_limit_mhz;
        else if (ctrl_mhz < -ctrl_limit_mhz)
            ctrl_mhz = -ctrl_limit_mhz;
        trackq[i].step_accum_mhz += (int32_t)ctrl_mhz;
        correction_hz = trackq[i].step_accum_mhz / 1000;
        correction_hz = weak_mode ? trackq_clamp_weak_step_hz(correction_hz) : trackq_clamp_step_hz(correction_hz);
        trackq[i].step_accum_mhz -= correction_hz * 1000;

        phase_inc = phase_down_read(i);
        next_phase_inc = trackq_adjust_increment(i, phase_inc, correction_hz, &limited);
        if (next_phase_inc != phase_inc)
            phase_down_write(i, next_phase_inc);
        if (limited)
            trackq[i].step_accum_mhz = 0;
        /* Captured samples precede the new LO command: reconstruct with
         * the previous increment, not the next tracking correction. */
        phase_hz_milli = uc_phase_inc_to_mhz(phase_inc, TRACK3_RF_FS_HZ);
        if (trackq_bin_vertex_estimate_mhz(track_samples[i], trackq[i].center_hz, trackq[i].n, &bin_vertex_hz_milli)) {
            bin_vertex_hf_mhz_log[i] = phase_hz_milli + center_base_hz_milli + (bin_vertex_hz_milli - center_hz_milli);
            bin_vertex_valid_log[i] = 1u;
        }
        if (trackq_observer_enabled) {
            struct trackq_observer_mode *m = &trackq_observer.mode[i];
            m->requested_mhz = (int64_t)correction_hz * 1000ll;
            m->demand_mhz = ctrl_mhz;
            m->applied_increment = (int64_t)next_phase_inc - phase_inc;
            m->limited = limited;
            m->controller_mode = confident ? "quad" : (weak_mode ? "side" : "deadband");
            m->fit_ok = bin_vertex_valid_log[i];
            m->fit_status = m->fit_ok ? "ok" : "binfit_rejected";
        }
        if (trackq_control_diag) {
            uint64_t sum = left_pwr + right_pwr;
            int64_t diff = (int64_t)right_pwr - (int64_t)left_pwr;
            /* Approximate milli-percent without overflowing diff * 100000. */
            int64_t imbalance = sum ? diff / (int64_t)(sum / 100000u + 1u) : 0ll;
            int64_t actual_mhz = ((int64_t)next_phase_inc - (int64_t)phase_inc) *
                                  (int64_t)TRACK3_RF_FS_HZ * 1000ll / (1ll << 26);
            printf("trackq ctrl: iteration=%lu ch=%u mode=%s P_L=%llu P_C=%llu P_R=%llu db=%u%%",
                   (unsigned long)(trackq_log_iteration + 1u), i + 1u,
                   confident ? "quad" : (weak_mode ? "side" : "deadband"),
                   (unsigned long long)left_pwr, (unsigned long long)center_pwr,
                   (unsigned long long)right_pwr, trackq_deadband_pct[i]);
            trackq_log_milli("imbalance", sum != 0u, imbalance, "%");
            trackq_log_milli("error", 1, error_mhz, "Hz");
            trackq_log_milli("filtered", 1, trackq[i].filt_error_mhz, "Hz");
            trackq_log_milli("demand", 1, ctrl_mhz, "Hz");
            trackq_log_milli("pending", 1, trackq[i].step_accum_mhz, "Hz");
            printf(" req=%ldHz dinc=%ld", (long)correction_hz,
                   (long)((int64_t)next_phase_inc - (int64_t)phase_inc));
            trackq_log_milli("applied", 1, actual_mhz, "Hz");
            printf(" binfit=%s limited=%d demand_limited=%d\n",
                   bin_vertex_valid_log[i] ? "ok" : "failed", limited, demand_limited);
        }
        trackq[i].next_tick = ce_ticks + TRACKQ_INTERVAL_TICKS;
    }

    /* Center the 64-sample reference window within the modal capture,
     * matching the centered-window reference comparison experiment. */
    if (capture_n >= TRACKQ_REF_FFT_N &&
        ref_fft_hann_log_estimate_mhz(
            track_samples_ref + (capture_n - TRACKQ_REF_FFT_N) / 2u,
            TRACKQ_REF_FFT_N, 0ll, 0u,
            &ref_bin_vertex_baseband_mhz_log)) {
        ref_bin_vertex_valid_log = 1u;
    }

    if (ref_bin_vertex_valid_log) {
        int64_t ref_phase_hz_milli = uc_phase_inc_to_mhz(main_phase_inc_down_ref_read(), TRACK3_RF_FS_HZ);
        int64_t ref_meas_hz_milli = ref_phase_hz_milli + ref_bin_vertex_baseband_mhz_log;
        int64_t nominal_fs_hz = (int64_t)TRACK3_RF_FS_HZ;

        if (ref_meas_hz_milli > 0ll) {
            int64_t fs_real_hz = (nominal_fs_hz * (int64_t)TRACKQ_REF_INPUT_HZ * 1000ll) / ref_meas_hz_milli;
            ref_real_fs_mhz_log = fs_real_hz * 1000ll;
            if (fs_real_hz > 0ll && fs_real_hz <= 0xffffffffll) {
                fs_valid_log = 1u;
                ref_nco_inc_log = uc_phase_inc_from_hz(TRACKQ_NCO_TARGET_HZ, (uint32_t)fs_real_hz);
                main_phase_inc_nco_write(ref_nco_inc_log);
                for (i = 0; i < TRACKQ_CHANNELS; ++i) {
                    if (bin_vertex_valid_log[i]) {
                        corr_vertex_hf_mhz_log[i] =
                            ((bin_vertex_hf_mhz_log[i] * fs_real_hz) + (nominal_fs_hz / 2ll)) / nominal_fs_hz;
                    }
                }
            }
        }
    }

    if (bin_vertex_valid_log[TRACKQ_C300_CHANNEL] &&
        bin_vertex_valid_log[TRACKQ_A100_CHANNEL] &&
        bin_vertex_valid_log[TRACKQ_C100_CHANNEL]) {
        crystal_valid_log = trackq_crystal_estimate(bin_vertex_hf_mhz_log,
                                                     &crystal_log);
    }

    if (bin_vertex_valid_log[TRACKQ_A100_CHANNEL] &&
        bin_vertex_valid_log[TRACKQ_C300_CHANNEL]) {
        temp_ratio_valid_log = trackq_temp_ratio_mc(
            bin_vertex_hf_mhz_log[TRACKQ_A100_CHANNEL],
            bin_vertex_hf_mhz_log[TRACKQ_C300_CHANNEL], &temp_ratio_mc_log);
    }

    if (fs_valid_log && bin_vertex_valid_log[TRACKQ_C300_CHANNEL] &&
        bin_vertex_valid_log[TRACKQ_A100_CHANNEL] && bin_vertex_valid_log[TRACKQ_C100_CHANNEL]) {
        int64_t pc, pa, pf;
        if (trackq_temp_ppm(corr_vertex_hf_mhz_log[TRACKQ_C300_CHANNEL], TRACKQ_TEMP_NOM_C300_MHZ, &pc) &&
            trackq_temp_ppm(corr_vertex_hf_mhz_log[TRACKQ_A100_CHANNEL], TRACKQ_TEMP_NOM_A100_MHZ, &pa) &&
            trackq_temp_ppm(corr_vertex_hf_mhz_log[TRACKQ_C100_CHANNEL], TRACKQ_TEMP_NOM_C100_MHZ, &pf)) {
            temp_delta_mc_log = (TRACKQ_TEMP_W_C300_NC_PER_PPM * pc +
                TRACKQ_TEMP_W_A100_NC_PER_PPM * pa + TRACKQ_TEMP_W_C100_NC_PER_PPM * pf) / 1000000000ll;
            temp_joint_mc_log = (TRACKQ_TEMP_JOINT_C300_Q9 * pc +
                TRACKQ_TEMP_JOINT_A100_Q9 * pa + TRACKQ_TEMP_JOINT_C100_Q9 * pf) / 1000000000ll;
            temp_common_mppm_log = (TRACKQ_TEMP_COMMON_C300_Q9 * pc +
                TRACKQ_TEMP_COMMON_A100_Q9 * pa + TRACKQ_TEMP_COMMON_C100_Q9 * pf) / 1000000000ll;
            temp_delta_valid_log = 1u;
        }
    }

    if (trackq_hybrid_enabled) hybrid_fifo_process = hybrid_fifo_flags();
    trackq_hybrid_step(crystal_valid_log, fs_valid_log,
        bin_vertex_hf_mhz_log, crystal_log.temp_abs_mc,
        ref_bin_vertex_baseband_mhz_log, main_phase_inc_down_ref_read(),
        hybrid_fifo_capture | (hybrid_fifo_process & 12u),
        (bin_vertex_valid_log[0] ? 1u : 0u) |
        (bin_vertex_valid_log[1] ? 2u : 0u) |
        (bin_vertex_valid_log[2] ? 4u : 0u), main_phase_inc_nco_read());
    /* Keep post-capture history until next preparation. DS overflow during
     * processing concerns samples not used in this record; UPS faults still
     * reject the candidate conservatively and trigger next-step recovery. */
    uc_commit();
    trackq_log_iteration++;
    if (!trackq_hybrid_enabled && (trackq_log_iteration % 5u) == 0u) {
        printf("trackq hf binfit: C300=%s%ld.%03ldHz A100=%s%ld.%03ldHz C100=%s%ld.%03ldHz ref=%s%ld.%03ldHz fs=%s%ld.%03ldHz dtemp=%s%s%ld.%03ldC\n",
               bin_vertex_valid_log[0] ? "" : "(na)",
               (long)(corr_vertex_hf_mhz_log[0] / 1000ll), (long)llabs(corr_vertex_hf_mhz_log[0] % 1000ll),
               bin_vertex_valid_log[1] ? "" : "(na)",
               (long)(corr_vertex_hf_mhz_log[1] / 1000ll), (long)llabs(corr_vertex_hf_mhz_log[1] % 1000ll),
               bin_vertex_valid_log[2] ? "" : "(na)",
               (long)(corr_vertex_hf_mhz_log[2] / 1000ll), (long)llabs(corr_vertex_hf_mhz_log[2] % 1000ll),
               ref_bin_vertex_valid_log ? "" : "(na)",
               (long)(ref_bin_vertex_baseband_mhz_log / 1000ll), (long)llabs(ref_bin_vertex_baseband_mhz_log % 1000ll),
                fs_valid_log ? "" : "(na)",
               (long)(ref_real_fs_mhz_log / 1000ll), (long)llabs(ref_real_fs_mhz_log % 1000ll),
               temp_delta_valid_log ? "" : "(na)",
               temp_delta_mc_log < 0ll ? "-" : "",
               (long)(llabs(temp_delta_mc_log) / 1000ll), (long)llabs(temp_delta_mc_log % 1000ll));
        printf("trackq temp diag: iteration=%lu", (unsigned long)trackq_log_iteration);
        trackq_log_milli("raw_C300", bin_vertex_valid_log[TRACKQ_C300_CHANNEL],
                         bin_vertex_hf_mhz_log[TRACKQ_C300_CHANNEL], "Hz");
        trackq_log_milli("raw_A100", bin_vertex_valid_log[TRACKQ_A100_CHANNEL],
                         bin_vertex_hf_mhz_log[TRACKQ_A100_CHANNEL], "Hz");
        trackq_log_milli("raw_C100", bin_vertex_valid_log[TRACKQ_C100_CHANNEL],
                         bin_vertex_hf_mhz_log[TRACKQ_C100_CHANNEL], "Hz");
        trackq_log_milli("dtemp_existing", temp_delta_valid_log, temp_delta_mc_log, "C");
        trackq_log_milli("dtemp_joint", temp_delta_valid_log, temp_joint_mc_log, "C");
        trackq_log_milli("c_hat", temp_delta_valid_log, temp_common_mppm_log, "ppm");
        trackq_log_milli("dtemp_ratio_A100_C300", temp_ratio_valid_log, temp_ratio_mc_log, "C");
        printf("\n");

        printf("trackq crystal diag: iteration=%lu",
               (unsigned long)trackq_log_iteration);
        trackq_log_milli("temp_raw_joint", crystal_valid_log,
                         crystal_log.temp_abs_mc, "C");
        if (crystal_valid_log)
            printf(" clock_scale=%ld.%09ld",
                   (long)(crystal_log.clock_scale_q9 / 1000000000ll),
                   (long)llabs(crystal_log.clock_scale_q9 % 1000000000ll));
        else
            printf(" clock_scale=(na)");
        trackq_log_milli("clock_offset", crystal_valid_log,
                         crystal_log.clock_offset_mppm, "ppm");
        trackq_log_milli("fs_crystal", crystal_valid_log,
                         crystal_log.fs_mhz, "Hz");
        if (crystal_valid_log)
            printf(" nco_inc_crystal=%lu", (unsigned long)crystal_log.nco_inc);
        else
            printf(" nco_inc_crystal=(na)");
        trackq_log_milli("residual_C300", crystal_valid_log,
                         crystal_log.residual_mppm[TRACKQ_C300_CHANNEL], "ppm");
        trackq_log_milli("residual_A100", crystal_valid_log,
                         crystal_log.residual_mppm[TRACKQ_A100_CHANNEL], "ppm");
        trackq_log_milli("residual_C100", crystal_valid_log,
                         crystal_log.residual_mppm[TRACKQ_C100_CHANNEL], "ppm");
        trackq_log_milli("fs_ref", fs_valid_log, ref_real_fs_mhz_log, "Hz");
        if (fs_valid_log)
            printf(" nco_inc_ref=%lu", (unsigned long)ref_nco_inc_log);
        else
            printf(" nco_inc_ref=(na)");
        printf("\n");
    }
    if (trackq_observer_enabled)
        trackq_observer_emit(trackq_log_iteration, bin_vertex_hf_mhz_log,
            bin_vertex_valid_log, ref_bin_vertex_valid_log, ref_bin_vertex_baseband_mhz_log,
            fs_valid_log, ref_real_fs_mhz_log, ref_nco_inc_log, crystal_valid_log,
            &crystal_log, temp_delta_valid_log, temp_delta_mc_log, temp_joint_mc_log,
            temp_ratio_valid_log, temp_ratio_mc_log);
}

static void cmd_track3(char *args) {
    char *tok_ch     = strtok(args, " \t");
    char *tok_start  = strtok(NULL, " \t");
    char *tok_step   = strtok(NULL, " \t");
    char *tok_steps  = strtok(NULL, " \t");
    char *tok_n      = strtok(NULL, " \t");
    char *tok_center = strtok(NULL, " \t");
    char *tok_delta  = strtok(NULL, " \t");
    unsigned ch;
    unsigned channel;
    uint32_t start_hz;
    uint32_t step_hz;
    unsigned max_steps;
    unsigned n;
    uint32_t center_hz;
    uint32_t delta_hz;
    unsigned band_bins = TRACK3_DEFAULT_BAND_BINS;
    unsigned settle = TRACK3_DEFAULT_SETTLE;
    uint32_t original_phase_inc;
    uint32_t sweep_hz;
    size_t cfg_need = 0;
    size_t cfg_len;
    kiss_fft_cfg cfg;

    if (!tok_ch || !tok_start) {
        puts("Usage: track3 <ch:1..5> <start_phase_down_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]");
        return;
    }

    ch         = (unsigned)strtoul(tok_ch, NULL, 0);
    if (ch < 1u || ch > 5u) {
        puts("track3 channel must be 1..5");
        return;
    }

    channel    = ch - 1u;
    start_hz   = (uint32_t)strtoul(tok_start, NULL, 0);
    step_hz    = tok_step   ? (uint32_t)strtoul(tok_step, NULL, 0) : TRACK3_DEFAULT_STEP_HZ;
    max_steps  = tok_steps  ? (unsigned)strtoul(tok_steps, NULL, 0) : TRACK3_DEFAULT_MAX_STEPS;
    n          = tok_n      ? (unsigned)strtoul(tok_n, NULL, 0) : TRACK3_DEFAULT_N;
    center_hz  = tok_center ? (uint32_t)strtoul(tok_center, NULL, 0) : TRACK3_DEFAULT_CENTER_HZ;
    delta_hz   = tok_delta  ? (uint32_t)strtoul(tok_delta, NULL, 0) : TRACK3_DEFAULT_DELTA_HZ;

    if (step_hz == 0u || max_steps == 0u) {
        puts("track3 requires step_hz > 0 and max_steps > 0");
        return;
    }
    if (!is_pow2_u(n) || n < 8u || n > FFT_MAX_N) {
        printf("track3 requires N to be power-of-2 and <= %u\n", FFT_MAX_N);
        return;
    }
    if (fft_fs_hz == 0u) {
        puts("track3 requires fft_fs > 0");
        return;
    }
    if (center_hz <= delta_hz) {
        puts("track3 requires center_hz > delta_hz");
        return;
    }

    sig3_set_channel_symmetric(channel, center_hz, delta_hz);
    sig3_update_increments();
    sig3_enable_channel(channel);

    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES) {
        printf("track3 fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)cfg_need, FFT_CFG_MAX_BYTES);
        return;
    }

    cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    cfg = kiss_fft_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg) {
        puts("track3 kiss_fft_alloc failed");
        return;
    }

    original_phase_inc = phase_down_read(channel);
    sweep_hz = start_hz;
    printf("track3: ch=%u start=%lu Hz step=%lu Hz max_steps=%u N=%u center=%lu Hz delta=%lu Hz Fs=%lu Hz sig3={%lu,%lu,%lu} Hz\n",
           ch,
           (unsigned long)start_hz,
           (unsigned long)step_hz,
           max_steps,
           n,
           (unsigned long)center_hz,
           (unsigned long)delta_hz,
           (unsigned long)fft_fs_hz,
           (unsigned long)(center_hz - delta_hz),
           (unsigned long)center_hz,
           (unsigned long)(center_hz + delta_hz));

    for (unsigned step = 0; step < max_steps; step++) {
        uint32_t phase_inc = uc_phase_inc_from_hz(sweep_hz, TRACK3_RF_FS_HZ);
        unsigned left_k;
        unsigned center_k;
        unsigned right_k;
        uint64_t left_pwr;
        uint64_t center_pwr;
        uint64_t right_pwr;

        phase_down_write(channel, phase_inc);
        uc_commit();

        if (!capture_ds_fft_channel(channel, n, settle)) {
            phase_down_write(channel, original_phase_inc);
            uc_commit();
            return;
        }

        kiss_fft(cfg, fft_in, fft_out);

        left_k   = (unsigned)((((uint64_t)(center_hz - delta_hz) * (uint64_t)n) + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);
        center_k = (unsigned)((((uint64_t)center_hz * (uint64_t)n) + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);
        right_k  = (unsigned)((((uint64_t)(center_hz + delta_hz) * (uint64_t)n) + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);

        if (right_k >= (n / 2u)) {
            puts("track3 expected bins exceed FFT Nyquist range");
            phase_down_write(channel, original_phase_inc);
            uc_commit();
            return;
        }

        left_pwr = fft_band_power_at(left_k, band_bins, n);
        center_pwr = fft_band_power_at(center_k, band_bins, n);
        right_pwr = fft_band_power_at(right_k, band_bins, n);

        printf("track3 step=%u phase_down_%u=%lu Hz inc=%lu bins={%u,%u,%u} pwr={%llu,%llu,%llu}\n",
               step,
               ch,
               (unsigned long)sweep_hz,
               (unsigned long)phase_inc,
               left_k,
               center_k,
               right_k,
               (unsigned long long)left_pwr,
               (unsigned long long)center_pwr,
               (unsigned long long)right_pwr);

        if (track3_triplet_match(left_pwr, center_pwr, right_pwr)) {
            printf("track3 lock: phase_down_%u=%lu Hz inc=%lu center=%lu left=%lu right=%lu\n",
                   ch,
                   (unsigned long)sweep_hz,
                   (unsigned long)phase_inc,
                   (unsigned long)center_hz,
                   (unsigned long)(center_hz - delta_hz),
                   (unsigned long)(center_hz + delta_hz));
            return;
        }

        track3_wait_ticks(TRACKQ_INTERVAL_TICKS);
        sweep_hz += step_hz;
    }

    phase_down_write(channel, original_phase_inc);
    uc_commit();
    printf("track3 no lock found in %u steps; restored phase_down_%u=%lu Hz\n",
           max_steps,
           ch,
           (unsigned long)uc_phase_inc_to_hz(original_phase_inc, TRACK3_RF_FS_HZ));
}

static void cmd_trackq_start(char *args) {
    if (ref_fft_enabled) {
        puts("trackq_start: run ref_fft_stop first");
        return;
    }
    char *tok_f1     = strtok(args, " \t");
    char *tok_f2     = strtok(NULL, " \t");
    char *tok_f3     = strtok(NULL, " \t");
    char *tok_n      = strtok(NULL, " \t");
    char *tok_center = strtok(NULL, " \t");
    char *tok_delta1 = strtok(NULL, " \t");
    char *tok_delta2 = strtok(NULL, " \t");
    char *tok_delta3 = strtok(NULL, " \t");
    uint32_t f1_hz = tok_f1 ? (uint32_t)strtoul(tok_f1, NULL, 0) : 0u;
    uint32_t f2_hz = tok_f2 ? (uint32_t)strtoul(tok_f2, NULL, 0) : 0u;
    uint32_t f3_hz = tok_f3 ? (uint32_t)strtoul(tok_f3, NULL, 0) : 0u;
    unsigned n = tok_n ? (unsigned)strtoul(tok_n, NULL, 0) : TRACK3_DEFAULT_N;
    uint32_t center_hz = tok_center ? (uint32_t)strtoul(tok_center, NULL, 0) : TRACK3_DEFAULT_CENTER_HZ;
    uint32_t delta_ch1_hz = tok_delta1 ? (uint32_t)strtoul(tok_delta1, NULL, 0) : trackq_default_delta_hz(0u);
    uint32_t delta_ch2_hz = tok_delta2 ? (uint32_t)strtoul(tok_delta2, NULL, 0) : trackq_default_delta_hz(1u);
    uint32_t delta_ch3_hz = tok_delta3 ? (uint32_t)strtoul(tok_delta3, NULL, 0) : trackq_default_delta_hz(2u);

    if (!is_pow2_u(n) || n < TRACKQ_REF_FFT_N || n > FFT_MAX_N) {
        printf("trackq_start requires N to be power-of-2, >= 64 and <= %u\n", FFT_MAX_N);
        return;
    }
    if (delta_ch1_hz == 0u || delta_ch2_hz == 0u || delta_ch3_hz == 0u) {
        puts("trackq_start requires delta_hz > 0");
        return;
    }
    if (center_hz <= delta_ch1_hz || center_hz <= delta_ch2_hz || center_hz <= delta_ch3_hz) {
        puts("trackq_start requires center_hz > delta_hz for all tracked channels");
        return;
    }
    if (fft_fs_hz == 0u) {
        puts("trackq_start requires fft_fs > 0");
        return;
    }

    if (trackq_hybrid_enabled) trackq_hybrid_reset();
    sig3_set_channel_symmetric(0u, center_hz, delta_ch1_hz);
    sig3_set_channel_symmetric(1u, center_hz, delta_ch2_hz);
    sig3_set_channel_symmetric(2u, center_hz, delta_ch3_hz);
    sig3_update_increments();
    sig3_enable_channel(0u);
    sig3_enable_channel(1u);
    sig3_enable_channel(2u);

    if (f1_hz) phase_down_write(0, uc_phase_inc_from_hz(f1_hz, TRACK3_RF_FS_HZ));
    if (f2_hz) phase_down_write(1, uc_phase_inc_from_hz(f2_hz, TRACK3_RF_FS_HZ));
    if (f3_hz) phase_down_write(2, uc_phase_inc_from_hz(f3_hz, TRACK3_RF_FS_HZ));
    if (f1_hz || f2_hz || f3_hz)
        uc_commit();

    for (unsigned i = 0; i < TRACKQ_CHANNELS; i++) {
        trackq[i].enabled = 1;
        trackq[i].n = n;
        trackq[i].settle = TRACK3_DEFAULT_SETTLE;
        trackq[i].center_hz = center_hz;
        switch (i) {
            case 0: trackq[i].delta_hz = delta_ch1_hz; break;
            case 1: trackq[i].delta_hz = delta_ch2_hz; break;
            default: trackq[i].delta_hz = delta_ch3_hz; break;
        }
        trackq[i].next_tick = ce_ticks + TRACKQ_INTERVAL_TICKS;
        trackq[i].filt_error_mhz = 0;
        trackq[i].step_accum_mhz = 0;
        trackq_inc_remainder[i] = 0ll;
    }

    printf("trackq_start: C300=%lu Hz A100=%lu Hz C100=%lu Hz N=%u center=%lu Hz delta={%lu,%lu,%lu} Hz sig3={{%lu,%lu,%lu},{%lu,%lu,%lu},{%lu,%lu,%lu}} Hz interval=1 s ref=Hann/log-power FFT64 centered\n",
           (unsigned long)uc_phase_inc_to_hz(phase_down_read(0), TRACK3_RF_FS_HZ),
           (unsigned long)uc_phase_inc_to_hz(phase_down_read(1), TRACK3_RF_FS_HZ),
           (unsigned long)uc_phase_inc_to_hz(phase_down_read(2), TRACK3_RF_FS_HZ),
           n,
           (unsigned long)center_hz,
           (unsigned long)trackq[0].delta_hz,
           (unsigned long)trackq[1].delta_hz,
           (unsigned long)trackq[2].delta_hz,
           (unsigned long)(center_hz - trackq[0].delta_hz),
           (unsigned long)center_hz,
           (unsigned long)(center_hz + trackq[0].delta_hz),
           (unsigned long)(center_hz - trackq[1].delta_hz),
           (unsigned long)center_hz,
           (unsigned long)(center_hz + trackq[1].delta_hz),
           (unsigned long)(center_hz - trackq[2].delta_hz),
           (unsigned long)center_hz,
           (unsigned long)(center_hz + trackq[2].delta_hz));
}

static void cmd_trackq_probe(char *args) {
    char *tok_n      = strtok(args, " \t");
    char *tok_center = strtok(NULL, " \t");
    char *tok_delta  = strtok(NULL, " \t");
    unsigned n = tok_n ? (unsigned)strtoul(tok_n, NULL, 0) : TRACK3_DEFAULT_N;
    uint32_t center_hz = tok_center ? (uint32_t)strtoul(tok_center, NULL, 0) : TRACK3_DEFAULT_CENTER_HZ;
    uint32_t delta_hz = tok_delta ? (uint32_t)strtoul(tok_delta, NULL, 0) : TRACKQ_C300_DELTA_HZ;
    uint64_t left_pwr;
    uint64_t center_pwr;
    uint64_t right_pwr;
    int64_t num;
    int64_t den;
    int64_t center_hz_milli;
    int64_t h_hz_milli;
    int64_t vertex_hz_milli;
    if (!is_pow2_u(n) || n < 8u || n > FFT_MAX_N) {
        printf("trackq_probe requires N to be power-of-2 and <= %u\n", FFT_MAX_N);
        return;
    }
    if (center_hz <= delta_hz) {
        puts("trackq_probe requires center_hz > delta_hz");
        return;
    }
    if (fft_fs_hz == 0u) {
        puts("trackq_probe requires fft_fs > 0");
        return;
    }

    /* Drop stale samples after manual LO changes, then wait for a fresh window. */
    main_ds_fifo_clear_write(1);
    (void)ds_fifo_flush_all(n + TRACK3_DEFAULT_SETTLE);
    track3_wait_ticks(n + TRACK3_DEFAULT_SETTLE);

    if (!capture_ds_fft_channel(0u, n, TRACK3_DEFAULT_SETTLE)) {
        puts("trackq_probe capture failed");
        return;
    }

    left_pwr = track_band_power_at_hz(center_hz - delta_hz, n);
    center_pwr = track_band_power_at_hz(center_hz, n);
    right_pwr = track_band_power_at_hz(center_hz + delta_hz, n);

    center_hz_milli = (int64_t)center_hz * 1000ll;
    vertex_hz_milli = center_hz_milli;
    if (track3_triplet_match(left_pwr, center_pwr, right_pwr)) {
        num = (int64_t)left_pwr - (int64_t)right_pwr;
        den = 2ll * ((int64_t)left_pwr - (2ll * (int64_t)center_pwr) + (int64_t)right_pwr);
        if (den < 0ll) {
            h_hz_milli = (int64_t)delta_hz * 1000ll;
            vertex_hz_milli = center_hz_milli + ((h_hz_milli * num) / den);
        }
    }

    printf("trackq_probe: phase_down_1=%lu Hz N=%u center=%lu Hz delta=%lu Hz pwr={%llu,%llu,%llu} vertex=%ld.%03ldHz\n",
           (unsigned long)uc_phase_inc_to_hz(main_phase_inc_down_1_read(), TRACK3_RF_FS_HZ),
           n,
           (unsigned long)center_hz,
           (unsigned long)delta_hz,
           (unsigned long long)left_pwr,
           (unsigned long long)center_pwr,
           (unsigned long long)right_pwr,
           (long)(vertex_hz_milli / 1000ll),
           (long)labs(vertex_hz_milli % 1000ll));
}

/* Live settings persist across trackq_start; a firmware restart restores
 * deadbands {10,1,10} and enables the per-update decision diagnostics. */
static void cmd_trackq_deadband(char *args) {
    char *channel = strtok(args, " \t");
    char *value = strtok(NULL, " \t");
    char *end;
    unsigned long ch, pct;
    if (!channel) {
        printf("trackq_deadband: C300=%u%% A100=%u%% C100=%u%%\n",
               trackq_deadband_pct[0], trackq_deadband_pct[1], trackq_deadband_pct[2]);
        return;
    }
    if (!value || strtok(NULL, " \t")) {
        puts("Usage: trackq_deadband [<ch:1..3> <percent:0..100>]");
        return;
    }
    ch = strtoul(channel, &end, 10);
    if (*end || ch < 1u || ch > TRACKQ_CHANNELS) {
        puts("trackq_deadband: channel must be 1..3");
        return;
    }
    pct = strtoul(value, &end, 10);
    if (*end || pct > 100u) {
        puts("trackq_deadband: percent must be 0..100");
        return;
    }
    trackq_deadband_pct[ch - 1u] = (unsigned)pct;
    printf("trackq_deadband: ch%lu=%lu%%\n", ch, pct);
}

static void cmd_trackq_diag(char *args) {
    char *value = strtok(args, " \t");
    if (!value || strtok(NULL, " \t") || (strcmp(value, "0") && strcmp(value, "1"))) {
        puts("Usage: trackq_diag <0|1>");
        return;
    }
    trackq_control_diag = value[0] == '1';
    printf("trackq control diagnostics %s\n", trackq_control_diag ? "enabled" : "disabled");
}

static void cmd_trackq_observer_diag(char *args) {
    char *value = strtok(args, " \t");
    if (!value || strtok(NULL, " \t") || (strcmp(value,"0") && strcmp(value,"1"))) {
        puts("Usage: trackq_observer_diag <0|1>");
        return;
    }
    trackq_observer_enabled = value[0] == '1';
    if (trackq_observer_enabled) {
        ++trackq_observer_session;
        trackq_observer_capture_id = 0;
        trackq_observer_last_tick = ce_ticks;
        trackq_observer_tick_total = 0;
    }
    printf("trackq observer config: v=1 enabled=%d session=%lu tick_hz=10000 timestamps=cpu_boundaries estimator=hann_log64_centered\n",
        trackq_observer_enabled,(unsigned long)trackq_observer_session);
}

static void cmd_trackq_stop(char *args) {
    (void)args;
    if (trackq_hybrid_enabled) {
#if defined(CSR_TIMER0_BASE)
        ref_fft_timer_release();
#endif
        trackq_hybrid_enabled = 0;
    }
    for (unsigned i = 0; i < TRACKQ_CHANNELS; i++)
        trackq[i].enabled = 0;
    puts("trackq_stop: quadratic tracking disabled on ch1..ch3");
}
/* Quiet, finite sweep: buffer results, restore mixer, then print. */
#define QS_MAX_ROWS 128u
#define QS_N 2048u
static struct {
    uint64_t power[3];
    uint32_t lo, t_ms, duration_us, flags;
    uint32_t modal_hash, ref_hash, samples_read;
    uint32_t center_lo, next_center_lo;
    unsigned follow_status;
    int32_t compare_row;
    uint32_t compare_n, modal_diff_n, modal_max_abs_diff;
    int32_t modal_min, modal_max;
    uint32_t lag10_n, lag10_diff_n, lag10_max_abs_diff;
    int64_t rf_target_mhz;
    uint32_t ref_updates;
    int32_t ref_control_mhz;
    uint64_t power_y, power_iq, ac_x_mcounts2, ac_y_mcounts2;
    int32_t mean_x_mcounts, mean_y_mcounts, y_min, y_max;
    int32_t bb_x_mhz, bb_y_mhz;
    uint32_t bb_x_flags, bb_y_flags;
    int32_t offset_hz, ref_mhz, det_mhz;
    int32_t ref_first_mhz, ref_last_mhz;
    int32_t loop_bb_mhz, loop_ref_mhz, modal_fit_bb_mhz;
    unsigned vertex_ok;
    uint32_t ups_wait, ups_capture, ups_process, ups_power, ups_ref;
} qs_rows[QS_MAX_ROWS];

/* Previous complete modal record at each of the three cycle LO positions.
 * Static storage avoids putting 12 KiB on the CPU stack. */
static int16_t qs_cycle_previous[3][QS_N];
static int32_t qs_cycle_previous_row[3];

/* FNV-1a over raw int16 sample bits, low byte then high byte.
 * Unsigned wraparound is intentional; no FFT or sample modification. */
static uint32_t qs_sample_hash(uint32_t hash, int16_t sample) {
    uint16_t bits = (uint16_t)sample;
    hash = (hash ^ (uint32_t)(bits & 255u)) * 16777619u;
    return (hash ^ (uint32_t)(bits >> 8)) * 16777619u;
}

static unsigned qs_flags(void) {
    return (main_ds_fifo_overflow_read() ? 1u : 0u) |
           (main_ds_fifo_underflow_read() ? 2u : 0u) |
           (main_ups_fifo_overflow_read() ? 4u : 0u) |
           (main_ups_fifo_underflow_read() ? 8u : 0u);
}

static int qs_wait_sample(void) {
    uint64_t start = ref_fft_cycles();
    while (!(main_ds_fifo_flags_read() & 1u)) {
        track3_service_background_budget(4u);
        if (ref_fft_cycles() - start > CONFIG_CLOCK_FREQUENCY / 2u) return 0;
    }
    return 1;
}

static int qs_drain(void) {
    for (unsigned i = 0; i < 65536u; ++i) {
        if (!(main_ds_fifo_flags_read() & 1u)) return 1;
        main_ds_fifo_pop_write(1);
        (void)main_ds_fifo_yref_read();
        track3_service_background_budget(4u);
    }
    return 0;
}

/* Return the preceding phase's UPS flags, then start a new interval.
 * A short serviced delay lets clear/status synchronize across clock domains.
 * These are interval flags, not counts of starvation events. */
static unsigned qs_ups_boundary(void) {
    unsigned flags = qs_flags() & 12u;
    main_ups_fifo_clear_write(1);
    uint64_t until = ref_fft_cycles() + CONFIG_CLOCK_FREQUENCY/100000u;
    while (ref_fft_cycles() < until) track3_service_background_budget(4u);
    return flags;
}

/* Explicit physical RF center for the reference-controlled single sweep. */
static uint32_t qs_ref_center_hz;
static int16_t qs_ref_control_samples[64];

/* Shared physical-axis model. Software DDS rate scales with DSP fs/6500. */
static int qs_ref_drive_model(uint32_t lo, uint32_t tone_inc, uint32_t ref_lo,
                              int64_t ref_mhz, double *fs_out, double *rf_out) {
    double fs=0.0;
    if (!ref_reconstruct_fs(ref_lo,ref_mhz,&fs)) return 0;
    *fs_out = fs;
    *rf_out = ((double)lo/(double)(1ul<<26) +
               (double)tone_inc/(4294967296.0*6500.0))*fs;
    return 1;
}

/* Fresh reference-only record during settling; no UART and no modal FFT. */
static int qs_ref_control_measure(int64_t *ref) {
    if (!qs_drain()) return 0;
    for (unsigned i = 0; i < 64u; ++i) {
        if (!qs_wait_sample()) return 0;
        main_ds_fifo_pop_write(1);
        qs_ref_control_samples[i] = (int16_t)(main_ds_fifo_yref_read() & 0xffffu);
        track3_service_background_budget(4u);
    }
    return ref_fft_hann_log_estimate_mhz(qs_ref_control_samples, 64u, 0ll, 0u, ref);
}

/* single: 0=normal, 1=crystal sweep, 2=DAC1 loopback sweep,
 * 3=repeated fixed triplets, 4=adaptive peak, 5=reference-controlled sweep. */
static void qs_run(char *args, int fixed, int single) {
    const int cyclic = single == 3 || single == 4;
    if (trackq_hybrid_enabled) { puts("Run trackq_hybrid 0 before quiet tests"); return; }
    uint32_t v[4] = {0u, 0u, 0u, 1000u};
    if (fixed) { v[1] = fixed == 2 ? 2u : 90u; v[2] = 2000u; }
    char *tok = strtok(args, " \t"), *end;
    unsigned argc = 0u;
    while (tok && argc < 4u) {
        if (*tok < '0' || *tok > '9') break;
        unsigned long x = strtoul(tok, &end, 10);
        if (*end || x > 10000ul) break;
        v[argc++] = (uint32_t)x;
        tok = strtok(NULL, " \t");
    }
    if (fixed == 2 && (tok || argc != 1u || v[0] < 1u || v[0] > 3u)) {
        puts("Usage: trackq_check <ch 1..3>");
        return;
    }
    if (fixed) {
        if (tok || argc < 1u || argc > 3u || v[0] < 1u || v[0] > 3u ||
            v[1] < 2u || v[1] > QS_MAX_ROWS || v[2] < 100u || v[2] > 5000u) {
            puts("Usage: trackq_hold <ch 1..3> [samples 2..128=90] [wait_ms 100..5000=2000]");
            return;
        }
        v[3] = v[2];
    } else if (cyclic) {
        if (tok || argc < 3u || v[0] < 1u || v[0] > 3u ||
            v[1] < 1u || v[1] > 2000u || v[2] < 1u || v[2] > QS_MAX_ROWS/6u ||
            v[3] < 100u || v[3] > 5000u) {
            puts("Usage: trackq_single_cycle / trackq_single_follow <ch 1..3> <span_Hz 1..2000> <cycles 1..21> [settle_ms 100..5000, default 1000]");
            return;
        }
    } else if (tok || argc < 3u || v[0] < 1u || v[0] > 3u ||
        v[1] < 1u || v[1] > 2000u || v[2] < 1u ||
        v[2] > 2u*v[1] || (2u*v[1]) % v[2] ||
        v[3] < 100u || v[3] > 5000u ||
        2u*(2u*v[1]/v[2]+1u) > QS_MAX_ROWS) {
        puts("Usage: trackq_sweep <ch 1..3> <span_Hz 1..2000> <step_Hz> [settle_ms 100..5000, default 1000]; step divides 2*span; <=128 rows including return sweep");
        return;
    }
    unsigned ch = v[0]-1u, points = (fixed || cyclic) ? 0u : 2u*v[1]/v[2]+1u, count = 0u;
    unsigned total = cyclic ? 6u*v[2] : (fixed ? v[1] : 2u*points);
    /* Single selected channel only: borrow another stopped mode buffer for Y. */
    int16_t *modal_y = track_samples[(ch+1u)%TRACKQ_CHANNELS];
    uint32_t center = sig3_freq_hz[ch][1];
    uint32_t left = sig3_freq_hz[ch][0], right = sig3_freq_hz[ch][2];
    if (ref_fft_enabled || fft_fs_hz != 10000u || !sig3_enable ||
        !sig3_channel_enable[ch] || !sig3_amp[ch] ||
        center <= left || right <= center || center-left != right-center || right >= 5000u) {
        puts("trackq_sweep: stop ref_fft; require enabled symmetric triple tones and fft_fs=10000");
        return;
    }
    if (fixed == 3 && (main_input_select_read() != 0u ||
        main_upsampler_input_mux_read() != 1u || main_output_select_ch2_read() != ch+5u ||
        center-left < 40u)) {
        puts("loopback_check: require input_select 0, upsampler_input_mux 1, DAC2 select=channel+4, tone spacing >=40 Hz");
        return;
    }
    if (single && (main_input_select_read() != 0u ||
        main_upsampler_input_mux_read() != 1u || main_output_select_ch1_read() != 15u)) {
        puts("single sweep: require input_select 0, upsampler_input_mux 1, output_select_ch1 15; ADC1=crystal response for ST / DAC1 for SL; reference on ADC2");
        return;
    }
    if ((single == 1 || single == 2) && center-left < 40u) {
        puts("single sweep: configure diagnostic tone spacing >=40 Hz for central FFT search");
        return;
    }
    uint32_t saved_lo = phase_down_read(ch), follow_lo = saved_lo;
    int64_t step_bound = fixed ? 0ll : (int64_t)uc_phase_inc_from_hz(v[1], TRACK3_RF_FS_HZ);
    if ((int64_t)saved_lo <= step_bound || (int64_t)saved_lo+step_bound >= (1ll<<25)) {
        puts("trackq_sweep: mixer sweep exceeds positive RF Nyquist range");
        return;
    }
    if (single == 5 && (v[3] < 1000u || qs_ref_center_hz <= v[1]+center ||
        qs_ref_center_hz+v[1] >= 32000000u)) {
        puts("trackq_ref_sweep: require settle_ms >=1000 and physical RF targets inside (center tone,32 MHz)");
        return;
    }
    if (!ref_fft_timer_start()) return;
    for (unsigned i = 0; i < TRACKQ_CHANNELS; ++i) trackq[i].enabled = 0;
    if (single) sig3_single_channel = (int)ch;
    uint32_t ref_lo = main_phase_inc_down_ref_read();
    uint32_t nco = main_phase_inc_nco_read();
    if (fixed == 2) {
        printf("trackq_check: ch=%u lo_inc=%lu drive_raw_mHz=%lld tones=%lu/%lu/%lu Hz; 2 s recovery + two captures (~7 s); tracking stopped\n",
            ch+1u, (unsigned long)saved_lo,
            (long long)(uc_phase_inc_to_mhz(saved_lo, TRACK3_RF_FS_HZ) + sig3_phase_inc_to_mhz(sig3_inc[ch][1], 10000u)),
            (unsigned long)left, (unsigned long)center, (unsigned long)right);
    } else if (single == 5) {
        printf("RS2 ch=%u rf_center_Hz=%lu reflo=%lu nco=%lu amp=%d tone_inc=%lu span_Hz=%lu step_Hz=%lu settle_ms=%lu points=%u fs_nom=65000000 ref_hz=%u\n",
            ch+1u, (unsigned long)qs_ref_center_hz, (unsigned long)ref_lo, (unsigned long)nco,
            (int)sig3_amp[ch], (unsigned long)sig3_inc[ch][1], (unsigned long)v[1],
            (unsigned long)v[2], (unsigned long)v[3], total, TRACKQ_REF_INPUT_HZ);
        puts("RS columns: index,dir,offset_Hz,target_RF_mHz,LO_inc,t_ms,read_us,PC,ref_center_mHz,ref_first_mHz,ref_last_mHz,achieved_RF_center_mHz,achieved_RF_first_mHz,achieved_RF_last_mHz,flags,ref_updates,control_ref_mHz,ups_wait,ups_capture,ups_power,ups_ref,PY,PIQ,mean_X_mcounts,mean_Y_mcounts,min_X,max_X,min_Y,max_Y,AC_X_mcounts2,AC_Y_mcounts2,bb_X_mHz,bb_Y_mHz,bb_X_flags,bb_Y_flags,hash_X,hash_Y,samples_read");
        puts("RS: reference-controlled single-tone RF sweep; updates about every 100 ms during settling; final 250 ms and capture hold LO; capture reference from same record; flags 1..256 as quiet tests, 4096=control failure 8192=reference endpoint change >10 Hz 16384=achieved target error >5 Hz at any window; RF achievement inferred from reference, not independent measurement; initial LO restored; output buffered");
    } else if (cyclic) {
        printf("%s ch=%u base=%lu reflo=%lu nco=%lu amp=%d center_Hz=%lu center_inc=%lu span_Hz=%lu cycles=%lu settle_ms=%lu n=2048 fs_nom=10000 rf_nom=65000000 ref_hz=%u points=%u\n",
            single == 4 ? "AT2" : "CY4", ch+1u, (unsigned long)saved_lo, (unsigned long)ref_lo, (unsigned long)nco,
            (int)sig3_amp[ch], (unsigned long)center, (unsigned long)sig3_inc[ch][1],
            (unsigned long)v[1], (unsigned long)v[2], (unsigned long)v[3], TRACKQ_REF_INPUT_HZ, total);
        puts("CY columns: index,cycle,dir(0=up/1=down),offset_Hz,LO_inc,t_ms,read_us,PC,ref64_center_mHz,ref64_first_mHz,ref64_last_mHz,flags,ups_wait,ups_capture,ups_power,ups_ref,modal_hash_hex,ref_hash_hex,samples_read,compare_row,compare_n,modal_diff_n,modal_max_abs_diff");
        puts("CY: sequence -span,0,+span,+span,0,-span; single selected central tone; reference windows from same record; flags 1=DSov 2=DSuf 4=UPSov 8=UPSuf 16=timeout 32=drain 64=center_ref_invalid 128=first_ref_invalid 256=last_ref_invalid; 4 s recovery; output buffered; estimate maxima offline from each triplet; t_ms=capture start");
    } else {
    printf("%s ch=%u base=%lu reflo=%lu nco=%lu amp=%d tones=%lu,%lu,%lu inc=%lu,%lu,%lu n=2048 fs=10000 rf=65000000 ref_hz=%u span=%lu step=%lu settle_ms=%lu points=%u\n",
        fixed == 3 ? "LB1" : (fixed ? "QH3" : (single == 2 ? "SL1" : (single ? "ST2" : "QS1"))), ch+1u, (unsigned long)saved_lo, (unsigned long)ref_lo, (unsigned long)nco,
        (int)sig3_amp[ch], (unsigned long)left, (unsigned long)center, (unsigned long)right,
        (unsigned long)sig3_inc[ch][0], (unsigned long)sig3_inc[ch][1], (unsigned long)sig3_inc[ch][2],
        TRACKQ_REF_INPUT_HZ, (unsigned long)(fixed ? 0u : v[1]), (unsigned long)(fixed ? 0u : v[2]), (unsigned long)v[3], total);
    puts("QS columns: index,dir(0=up/1=down),offset_Hz,LO_inc,t_ms,read_us,PL,PC,PR,ref_mHz,det_mHz,vertex_ok,flags; flags DSov=1 DSuf=2 UPSov=4 UPSuf=8 timeout=16 drain=32 ref_invalid=64; det=0 unless vertex_ok=1; output buffered until sweep ends");
    if (fixed) puts("QH uses same columns, dir=2 offset=0; appends ref_first_mHz,ref_last_mHz,ups_wait,ups_capture,ups_process,ups_power,ups_ref; flags 128=first_ref_invalid 256=last_ref_invalid; UPS phase bits 4=overflow 8=underflow; flags aggregate this row; startup logged at end; no LO or NCO writes");
    }
    if (single == 1) puts("ST: only selected channel central tone enabled; PL/PR are unexcited diagnostic bands; det/vertex disabled. Appends ref_first_mHz,ref_last_mHz,ups_wait,ups_capture,ups_process,ups_power,ups_ref,ref2048_mHz,response_bb2048_mHz,trackq_binfit_bb_mHz; invalid bits=128/256/1024/512/2048; 4 s FIFO recovery; tone selection and mixer restored before output.");
    if (single == 2) puts("SL: DAC1 sum output -> ADC1; external reference -> ADC2; only selected central tone active. Appends ref_first_mHz,ref_last_mHz,ups_wait,ups_capture,ups_process,ups_power,ups_ref,dac_bb2048_mHz,ref_bb2048_mHz; invalid bits=128/256/512/1024; det/vertex disabled; 4 s recovery; tone selection and mixer restored before output.");
    if (fixed == 3) puts("LB appends dac_bb2048_mHz,ref_bb2048_mHz to QH3 columns; flags 512=DAC FFT invalid, 1024=reference FFT2048 invalid; same capture, central tone search limited to +/- delta/3");
    if (cyclic) puts("CY2 fingerprints: FNV1a32 over ordered raw int16 samples (low byte then high byte); compare complete records only; equal hashes suggest repeated samples but do not prove freshness or duplication");
    if (cyclic) {
        puts("CY3 comparison: previous complete record at same LO; compare_row=-1 means no baseline; compare_n=2048 with modal_diff_n=0 means all raw modal samples identical; max_abs_diff in ADC counts; reference hashes remain diagnostic");
        for (unsigned slot = 0; slot < 3u; ++slot) qs_cycle_previous_row[slot] = -1;
    }
    if (single == 4) puts("AT appends center_LO_inc; AP after each complete triplet: last_index,center_LO_inc,next_center_LO_inc,status,peak_offset_mHz; status 0=bad power/capture 1=bracketed fit 2=left strongest 3=right strongest 4=tie 5=travel bound; peak_offset only valid status=1; side move=half span; total travel bounded to +/-2000 nominal Hz; reference not used for steering; original LO restored at end");
    if (cyclic) puts("Periodicity: five trailing fields modal_min,modal_max,lag10_n,lag10_diff_n,lag10_max_abs_diff; compare x[i] with x[i-10] within same record; complete capture has 2038 pairs; ADC counts; zero lag differences can also mean constant data");
    if (single == 5) puts("RS2: PC=PX; PY same narrow-band estimator on raw Y; PIQ=PX+PY; means/counts x1000, AC variance counts_squared x1000; bb X/Y from rising-crossing regression (flags 1=few 2=small range 4=period spread 8=slope); added flags 32768=bb offset >10 Hz 65536=X frequency unreliable 131072=Y frequency unreliable 262144=power sum overflow; hashes FNV1a raw samples; no extra capture; LO/NCO control unchanged");
    uint64_t origin = ref_fft_cycles();
    unsigned startup_ups = 0u, history_ups = 0u;
    if (fixed || single) {
        /* Recover from UART/console starvation before measurement intervals.
         * Retain old flags for the final startup report; no UART here. */
        uint64_t until = ref_fft_cycles() + (single ? 4ull : 2ull)*CONFIG_CLOCK_FREQUENCY;
        while (ref_fft_cycles() < until) {
            track3_service_background_budget(4u);
            if (main_ds_fifo_flags_read() & 1u) {
                main_ds_fifo_pop_write(1);
                (void)main_ds_fifo_yref_read();
            }
        }
        startup_ups = qs_ups_boundary();
    }
    for (unsigned row = 0; row < total; ++row) {
        unsigned pos = (fixed || cyclic) ? 0u : (row < points ? row : 2u*points-1u-row);
        int32_t offset = fixed ? 0 : -(int32_t)v[1] + (int32_t)(pos*v[2]);
        if (cyclic) {
            static const int8_t order[6] = {-1, 0, 1, 1, 0, -1};
            offset = (int32_t)order[row % 6u] * (int32_t)v[1];
        }
        int64_t di = ((int64_t)offset * (1ll<<26)) / TRACK3_RF_FS_HZ;
        memset(&qs_rows[row], 0, sizeof(qs_rows[row]));
        qs_rows[row].compare_row = -1;
        unsigned compare_slot = offset < 0 ? 0u : (offset > 0 ? 2u : 1u);
        if (cyclic) qs_rows[row].compare_row = qs_cycle_previous_row[compare_slot];
        qs_rows[row].center_lo = single == 4 ? follow_lo : saved_lo;
        qs_rows[row].offset_hz = offset;
        qs_rows[row].lo = (uint32_t)((int64_t)qs_rows[row].center_lo+di);
        if (single == 4 && qs_rows[row].compare_row >= 0 &&
            qs_rows[qs_rows[row].compare_row].lo != qs_rows[row].lo)
            qs_rows[row].compare_row = -1;
        if (single == 5) {
            qs_rows[row].rf_target_mhz = ((int64_t)qs_ref_center_hz+offset)*1000ll;
            qs_rows[row].lo = phase_down_read(ch);
        }
        if (!fixed && single != 5) {
            phase_down_write(ch, qs_rows[row].lo);
            uc_commit();
        }
        /* Keep software excitation fed during settling. Clear UPS flags here,
         * retaining any starvation during settling AND acquisition. */
        if (!fixed && !single) main_ups_fifo_clear_write(1);
        uint64_t until = ref_fft_cycles() + (uint64_t)v[3]*(CONFIG_CLOCK_FREQUENCY/1000u);
        uint64_t control_next = ref_fft_cycles();
        uint32_t control_flags = 0u;
        while (ref_fft_cycles() < until) {
            uint64_t now = ref_fft_cycles();
            if (single == 5 && now < until && now >= control_next &&
                until-now > CONFIG_CLOCK_FREQUENCY/4u) {
                int64_t measured = 0;
                double fs = 0.0, rf = 0.0;
                if (qs_ref_control_measure(&measured) &&
                    qs_ref_drive_model(qs_rows[row].lo, sig3_inc[ch][1], ref_lo, measured, &fs, &rf)) {
                    double desired = ((double)qs_rows[row].rf_target_mhz/1000.0/fs -
                        (double)sig3_inc[ch][1]/(4294967296.0*6500.0))*(double)(1ul<<26);
                    if (desired >= 0.5 && desired < (double)(1ul<<25)-0.5) {
                        qs_rows[row].lo = (uint32_t)(desired+0.5);
                        phase_down_write(ch, qs_rows[row].lo);
                        uc_commit();
                        ++qs_rows[row].ref_updates;
                        qs_rows[row].ref_control_mhz = (int32_t)measured;
                    } else control_flags |= 4096u;
                } else control_flags |= 4096u;
                control_next = ref_fft_cycles() + CONFIG_CLOCK_FREQUENCY/10u;
            }
            track3_service_background_budget(4u);
            if (main_ds_fifo_flags_read() & 1u) {
                main_ds_fifo_pop_write(1);
                (void)main_ds_fifo_yref_read();
            }
        }
        if (!qs_drain()) {
            qs_rows[row].ups_wait = (fixed || single) ? (qs_flags() & 12u) : 0u;
            qs_rows[row].flags = 32u | qs_rows[row].ups_wait | control_flags;
            history_ups |= qs_rows[row].ups_wait;
            count = row+1u;
            break;
        }
        if (fixed || single) qs_rows[row].ups_wait = qs_ups_boundary();
        main_ds_fifo_clear_write(1);
        /* Let the clear pulse and status cross clock domains. */
        until = ref_fft_cycles() + CONFIG_CLOCK_FREQUENCY/100000u;
        while (ref_fft_cycles() < until) track3_service_background_budget(4u);
        uint64_t started = ref_fft_cycles();
        qs_rows[row].t_ms = (uint32_t)((started-origin)/(CONFIG_CLOCK_FREQUENCY/1000u));
        unsigned i;
        int32_t sum_x=0, sum_y=0;
        int64_t sum_xx=0ll, sum_yy=0ll;
        if (single == 5) {
            qs_rows[row].modal_min=32767; qs_rows[row].modal_max=-32768;
            qs_rows[row].y_min=32767; qs_rows[row].y_max=-32768;
            qs_rows[row].modal_hash=2166136261u;
            qs_rows[row].ref_hash=2166136261u; /* RS2 reuses field for modal Y hash. */
        }
        if (cyclic) {
            qs_rows[row].modal_min = 32767;
            qs_rows[row].modal_max = -32768;
            qs_rows[row].modal_hash = 2166136261u;
            qs_rows[row].ref_hash = 2166136261u;
        }
        for (i = 0; i < QS_N; ++i) {
            iq6_frame_t frame;
            if (!qs_wait_sample()) break;
            ds_fifo_read_frame(&frame);
            track_samples[ch][i] = frame.x[ch];
            track_samples_ref[i] = frame.y[5];
            if (single == 5) {
                int32_t x=frame.x[ch], y=frame.y[ch];
                modal_y[i]=(int16_t)y;
                sum_x+=x; sum_y+=y;
                sum_xx+=(int64_t)x*x; sum_yy+=(int64_t)y*y;
                if (x<qs_rows[row].modal_min) qs_rows[row].modal_min=x;
                if (x>qs_rows[row].modal_max) qs_rows[row].modal_max=x;
                if (y<qs_rows[row].y_min) qs_rows[row].y_min=y;
                if (y>qs_rows[row].y_max) qs_rows[row].y_max=y;
                qs_rows[row].modal_hash=qs_sample_hash(qs_rows[row].modal_hash,(int16_t)x);
                qs_rows[row].ref_hash=qs_sample_hash(qs_rows[row].ref_hash,(int16_t)y);
            }
            if (cyclic) {
                int16_t sample = track_samples[ch][i];
                if ((int32_t)sample < qs_rows[row].modal_min) qs_rows[row].modal_min = sample;
                if ((int32_t)sample > qs_rows[row].modal_max) qs_rows[row].modal_max = sample;
                if (i >= 10u) {
                    int32_t diff = (int32_t)sample - (int32_t)track_samples[ch][i-10u];
                    uint32_t magnitude = (uint32_t)(diff < 0 ? -diff : diff);
                    ++qs_rows[row].lag10_n;
                    if (magnitude) ++qs_rows[row].lag10_diff_n;
                    if (magnitude > qs_rows[row].lag10_max_abs_diff)
                        qs_rows[row].lag10_max_abs_diff = magnitude;
                }
                if (qs_rows[row].compare_row >= 0) {
                    int32_t diff = (int32_t)sample - (int32_t)qs_cycle_previous[compare_slot][i];
                    uint32_t magnitude = (uint32_t)(diff < 0 ? -diff : diff);
                    ++qs_rows[row].compare_n;
                    if (magnitude) ++qs_rows[row].modal_diff_n;
                    if (magnitude > qs_rows[row].modal_max_abs_diff)
                        qs_rows[row].modal_max_abs_diff = magnitude;
                }
                qs_cycle_previous[compare_slot][i] = sample;
                qs_rows[row].modal_hash = qs_sample_hash(qs_rows[row].modal_hash, track_samples[ch][i]);
                qs_rows[row].ref_hash = qs_sample_hash(qs_rows[row].ref_hash, track_samples_ref[i]);
            }
            track3_service_background_budget(4u);
        }
        if (cyclic)
            qs_cycle_previous_row[compare_slot] = i == QS_N ? (int32_t)row : -1;
        qs_rows[row].samples_read = i;
        qs_rows[row].duration_us = (uint32_t)((ref_fft_cycles()-started)/(CONFIG_CLOCK_FREQUENCY/1000000u));
        qs_rows[row].flags = qs_flags() | (i == QS_N ? 0u : 16u) | control_flags;
        if (fixed || single) {
            qs_rows[row].ups_capture = qs_ups_boundary();
            qs_rows[row].flags |= qs_rows[row].ups_wait | qs_rows[row].ups_capture;
            history_ups |= qs_rows[row].ups_wait | qs_rows[row].ups_capture;
        }
        count = row+1u;
        if (i != QS_N) break;
        qs_prime_excitation();
        if (!cyclic && single != 5)
            qs_rows[row].power[0] = track_band_power_at_hz_samples(track_samples[ch], left, QS_N);
        qs_rows[row].power[1] = track_band_power_at_hz_samples(track_samples[ch], center, QS_N);
        if (single == 5) {
            qs_rows[row].power_y=track_band_power_at_hz_samples(modal_y,center,QS_N);
            uint64_t px=qs_rows[row].power[1], py=qs_rows[row].power_y;
            if (UINT64_MAX-px<py) {
                qs_rows[row].flags |= 262144u;
                qs_rows[row].power_iq=UINT64_MAX;
            } else qs_rows[row].power_iq=px+py;
            qs_rows[row].mean_x_mcounts=(int32_t)(((int64_t)sum_x*1000ll)/QS_N);
            qs_rows[row].mean_y_mcounts=(int32_t)(((int64_t)sum_y*1000ll)/QS_N);
            /* Full int16 range: variance numerator *1000 fits signed int64. */
            qs_rows[row].ac_x_mcounts2=(uint64_t)(((sum_xx*QS_N-(int64_t)sum_x*sum_x)*1000ll)/((int64_t)QS_N*QS_N));
            qs_rows[row].ac_y_mcounts2=(uint64_t)(((sum_yy*QS_N-(int64_t)sum_y*sum_y)*1000ll)/((int64_t)QS_N*QS_N));
            int64_t measured=0ll;
            struct ref_zc_diag diagnostic;
            int valid=ref_zc_estimate_mhz(track_samples[ch],QS_N,&measured,&diagnostic);
            qs_rows[row].bb_x_flags=diagnostic.flags;
            if (valid) qs_rows[row].bb_x_mhz=(int32_t)measured;
            if (!valid || diagnostic.flags) qs_rows[row].flags |= 65536u;
            if (valid && (measured < (int64_t)center*1000ll-10000ll ||
                          measured > (int64_t)center*1000ll+10000ll)) qs_rows[row].flags |= 32768u;
            valid=ref_zc_estimate_mhz(modal_y,QS_N,&measured,&diagnostic);
            qs_rows[row].bb_y_flags=diagnostic.flags;
            if (valid) qs_rows[row].bb_y_mhz=(int32_t)measured;
            if (!valid || diagnostic.flags) qs_rows[row].flags |= 131072u;
            if (valid && (measured < (int64_t)center*1000ll-10000ll ||
                          measured > (int64_t)center*1000ll+10000ll)) qs_rows[row].flags |= 32768u;
        }
        if (!cyclic && single != 5)
            qs_rows[row].power[2] = track_band_power_at_hz_samples(track_samples[ch], right, QS_N);
        double l = trackq_observer_power_double(qs_rows[row].power[0]);
        double c = trackq_observer_power_double(qs_rows[row].power[1]);
        double r = trackq_observer_power_double(qs_rows[row].power[2]);
        double den = l-2.0*c+r;
        if (!single && c > l && c > r && den < 0.0) {
            double det = (double)(center-left)*(l-r)/(2.0*den);
            if (det >= -(double)(center-left) && det <= (double)(center-left)) {
                qs_rows[row].vertex_ok = 1u;
                qs_rows[row].det_mhz = (int32_t)(det*1000.0);
            }
        }
        if (fixed || single) qs_rows[row].ups_power = qs_ups_boundary();
        int64_t ref = 0;
        track3_service_background_budget(16u);
        if (ref_fft_hann_log_estimate_mhz(track_samples_ref+(QS_N-64u)/2u, 64u, 0ll, 0u, &ref))
            qs_rows[row].ref_mhz = (int32_t)ref;
        else qs_rows[row].flags |= 64u;
        track3_service_background_budget(16u);
        if (fixed || single) {
            /* Same record: endpoint windows expose changes hidden by the
             * centered reference estimate; no extra acquisition. */
            if (ref_fft_hann_log_estimate_mhz(track_samples_ref, 64u, 0ll, 0u, &ref))
                qs_rows[row].ref_first_mhz = (int32_t)ref;
            else qs_rows[row].flags |= 128u;
            track3_service_background_budget(16u);
            if (ref_fft_hann_log_estimate_mhz(track_samples_ref+QS_N-64u, 64u, 0ll, 0u, &ref))
                qs_rows[row].ref_last_mhz = (int32_t)ref;
            else qs_rows[row].flags |= 256u;
            track3_service_background_budget(16u);
            if (fixed == 3 || single == 1 || single == 2) {
                int64_t measured = 0;
                unsigned width = (center-left)/3u;
                if (ref_fft_hann_log_estimate_mhz(track_samples[ch], QS_N,
                        (int64_t)center*1000ll, width, &measured) &&
                    measured >= (int64_t)(center-width)*1000ll &&
                    measured <= (int64_t)(center+width)*1000ll)
                    qs_rows[row].loop_bb_mhz = (int32_t)measured;
                else qs_rows[row].flags |= 512u;
                track3_service_background_budget(16u);
                if (ref_fft_hann_log_estimate_mhz(track_samples_ref, QS_N, 0ll, 0u, &measured))
                    qs_rows[row].loop_ref_mhz = (int32_t)measured;
                else qs_rows[row].flags |= 1024u;
                track3_service_background_budget(16u);
            }
            if (single == 1) {
                /* Test the actual tracking estimator, not only Hann/log FFT.
                 * Both use the same untouched time-domain modal record. */
                int64_t measured = 0;
                qs_prime_excitation();
                if (trackq_bin_vertex_estimate_mhz(track_samples[ch], center, QS_N, &measured))
                    qs_rows[row].modal_fit_bb_mhz = (int32_t)measured;
                else qs_rows[row].flags |= 2048u;
                track3_service_background_budget(16u);
            }
            qs_rows[row].ups_ref = qs_ups_boundary();
            qs_rows[row].ups_process = qs_rows[row].ups_power | qs_rows[row].ups_ref;
            qs_rows[row].flags |= qs_rows[row].ups_process;
            history_ups |= qs_rows[row].ups_process;
        }
        if (single == 5) {
            int32_t refs[3] = {qs_rows[row].ref_mhz, qs_rows[row].ref_first_mhz, qs_rows[row].ref_last_mhz};
            if (!qs_rows[row].ref_updates) qs_rows[row].flags |= 4096u;
            if (!(qs_rows[row].flags & (64u|128u|256u))) {
                int64_t delta = (int64_t)refs[2]-refs[1];
                if (delta > 10000ll || delta < -10000ll) qs_rows[row].flags |= 8192u;
                for (unsigned j = 0; j < 3u; ++j) {
                    double fs = 0.0, rf = 0.0;
                    if (!qs_ref_drive_model(qs_rows[row].lo, sig3_inc[ch][1], ref_lo, refs[j], &fs, &rf)) {
                        qs_rows[row].flags |= 4096u;
                        continue;
                    }
                    double error = rf - (double)qs_rows[row].rf_target_mhz/1000.0;
                    if (error > 5.0 || error < -5.0) qs_rows[row].flags |= 16384u;
                }
            }
        }
        if (single == 4 && row % 3u == 2u) {
            unsigned first = row-2u, middle = row-1u;
            unsigned li = (row%6u == 2u) ? first : row;
            unsigned ri = (row%6u == 2u) ? row : first;
            double pl = trackq_observer_power_double(qs_rows[li].power[1]);
            double pc = trackq_observer_power_double(qs_rows[middle].power[1]);
            double pr = trackq_observer_power_double(qs_rows[ri].power[1]);
            unsigned faults = (qs_rows[first].flags | qs_rows[middle].flags | qs_rows[row].flags) & 63u;
            double spacing_inc = (double)(qs_rows[ri].lo - qs_rows[middle].lo);
            double correction_inc = 0.0;
            unsigned status = 0u;
            if (!faults && pl > 0.0 && pc > 0.0 && pr > 0.0 && spacing_inc > 0.0) {
                double den = pl-2.0*pc+pr;
                if (pc > pl && pc > pr && den < 0.0) {
                    correction_inc = spacing_inc*(pl-pr)/(2.0*den);
                    status = 1u;
                    qs_rows[row].det_mhz = (int32_t)(correction_inc * (double)TRACK3_RF_FS_HZ * 1000.0 / (double)(1ul<<26));
                    qs_rows[row].vertex_ok = 1u;
                } else if (pl > pc && pl > pr) {
                    correction_inc = -spacing_inc/2.0;
                    status = 2u;
                } else if (pr > pc && pr > pl) {
                    correction_inc = spacing_inc/2.0;
                    status = 3u;
                } else status = 4u;
            }
            /* Strict center dominance bounds a fitted move to half a span.
             * Round to the RF accumulator's integer resolution. */
            int64_t move = (int64_t)(correction_inc + (correction_inc >= 0.0 ? 0.5 : -0.5));
            int64_t next = (int64_t)follow_lo + move;
            int64_t bound = ((int64_t)2000*(1ll<<26))/TRACK3_RF_FS_HZ;
            int64_t span_inc = ((int64_t)v[1]*(1ll<<26))/TRACK3_RF_FS_HZ;
            if (next-(int64_t)saved_lo > bound || (int64_t)saved_lo-next > bound ||
                next <= span_inc || next+span_inc >= (1ll<<25)) {
                status = 5u;
                next = follow_lo;
            }
            qs_rows[row].follow_status = status;
            follow_lo = (uint32_t)next;
            qs_rows[row].next_center_lo = follow_lo;
        }
    }
    if (!fixed) {
        phase_down_write(ch, saved_lo);
        uc_commit();
    }
    if (single) {
        sig3_single_channel = -1;
        /* Flush queued single-tone frames by continuing normal excitation.
         * FIFO capacity is 16384 frames (1.6384 s nominal). */
        uint64_t until = ref_fft_cycles() + 4ull*CONFIG_CLOCK_FREQUENCY;
        while (ref_fft_cycles() < until) track3_service_background_budget(4u);
    }
    #if defined(CSR_TIMER0_BASE)
    ref_fft_timer_release();
    #endif
    if (fixed == 2) {
        for (unsigned i = 0; i < count; ++i) {
            uint64_t l = qs_rows[i].power[0], c = qs_rows[i].power[1], r = qs_rows[i].power[2];
            unsigned usable = (qs_rows[i].flags & 63u) == 0u && (l || c || r);
            const char *peak = !(l || c || r) ? "none" :
                (c > l && c > r) ? "center" :
                (l > c && l > r) ? "left" :
                (r > l && r > c) ? "right" : "tie";
            printf("check ch=%u sample=%u PL=%llu PC=%llu PR=%llu peak=%s center_max=%s measurement=%s flags=%lu ups_wait=%lu ups_capture=%lu ups_process=%lu ups_power=%lu ups_ref=%lu\n",
                ch+1u, i+1u, (unsigned long long)l, (unsigned long long)c,
                (unsigned long long)r, peak, !(l || c || r) ? "unavailable" :
                (c > l && c > r) ? "yes" : "no", usable ? "ok" : "unverified",
                (unsigned long)qs_rows[i].flags, (unsigned long)qs_rows[i].ups_wait,
                (unsigned long)qs_rows[i].ups_capture, (unsigned long)qs_rows[i].ups_process,
                (unsigned long)qs_rows[i].ups_power, (unsigned long)qs_rows[i].ups_ref);
            track3_service_background_budget(16u);
        }
        printf("check startup_ups=%u history_ups=%u (bit 4=overflow, 8=underflow; startup includes pre-test history)\n", startup_ups, history_ups);
        puts("trackq_check done: tracking remains stopped; flags bits 1/2=DS ov/uf, 4/8=UPS ov/uf, 16=timeout, 32=drain; reference-only flags do not invalidate power comparison");
        return;
    }
    for (unsigned i = 0; i < count; ++i) {
        if (single == 5) {
            int32_t refs[3] = {qs_rows[i].ref_mhz, qs_rows[i].ref_first_mhz, qs_rows[i].ref_last_mhz};
            int64_t achieved[3] = {0ll,0ll,0ll};
            unsigned invalid[3] = {64u,128u,256u};
            for (unsigned j=0; j<3u; ++j) {
                double fs=0.0, rf=0.0;
                if (!(qs_rows[i].flags & (16u|32u|invalid[j])) &&
                    qs_ref_drive_model(qs_rows[i].lo, sig3_inc[ch][1], ref_lo, refs[j], &fs, &rf))
                    achieved[j] = (int64_t)(rf*1000.0+0.5);
            }
            printf("RS,%u,%u,%ld,%lld,%lu,%lu,%lu,%llu,%ld,%ld,%ld,%lld,%lld,%lld,%lu,%lu,%ld,%lu,%lu,%lu,%lu",
                i, i<points ? 0u : 1u, (long)qs_rows[i].offset_hz,
                (long long)qs_rows[i].rf_target_mhz, (unsigned long)qs_rows[i].lo,
                (unsigned long)qs_rows[i].t_ms, (unsigned long)qs_rows[i].duration_us,
                (unsigned long long)qs_rows[i].power[1], (long)refs[0], (long)refs[1], (long)refs[2],
                (long long)achieved[0], (long long)achieved[1], (long long)achieved[2],
                (unsigned long)qs_rows[i].flags, (unsigned long)qs_rows[i].ref_updates,
                (long)qs_rows[i].ref_control_mhz, (unsigned long)qs_rows[i].ups_wait,
                (unsigned long)qs_rows[i].ups_capture, (unsigned long)qs_rows[i].ups_power,
                (unsigned long)qs_rows[i].ups_ref);
            printf(",%llu,%llu,%ld,%ld,%ld,%ld,%ld,%ld,%llu,%llu,%ld,%ld,%lu,%lu,%08lx,%08lx,%lu\n",
                (unsigned long long)qs_rows[i].power_y,(unsigned long long)qs_rows[i].power_iq,
                (long)qs_rows[i].mean_x_mcounts,(long)qs_rows[i].mean_y_mcounts,
                (long)qs_rows[i].modal_min,(long)qs_rows[i].modal_max,
                (long)qs_rows[i].y_min,(long)qs_rows[i].y_max,
                (unsigned long long)qs_rows[i].ac_x_mcounts2,(unsigned long long)qs_rows[i].ac_y_mcounts2,
                (long)qs_rows[i].bb_x_mhz,(long)qs_rows[i].bb_y_mhz,
                (unsigned long)qs_rows[i].bb_x_flags,(unsigned long)qs_rows[i].bb_y_flags,
                (unsigned long)qs_rows[i].modal_hash,(unsigned long)qs_rows[i].ref_hash,
                (unsigned long)qs_rows[i].samples_read);
            track3_service_background_budget(16u);
            continue;
        }
        if (cyclic) {
            printf("%s,%u,%u,%u,%ld,%lu,%lu,%lu,%llu,%ld,%ld,%ld,%lu,%lu,%lu,%lu,%lu,%08lx,%08lx,%lu,%ld,%lu,%lu,%lu",
                single == 4 ? "AT" : "CY", i, i/6u, (i%6u)/3u, (long)qs_rows[i].offset_hz,
                (unsigned long)qs_rows[i].lo, (unsigned long)qs_rows[i].t_ms,
                (unsigned long)qs_rows[i].duration_us, (unsigned long long)qs_rows[i].power[1],
                (long)qs_rows[i].ref_mhz, (long)qs_rows[i].ref_first_mhz, (long)qs_rows[i].ref_last_mhz,
                (unsigned long)qs_rows[i].flags, (unsigned long)qs_rows[i].ups_wait,
                (unsigned long)qs_rows[i].ups_capture, (unsigned long)qs_rows[i].ups_power,
                (unsigned long)qs_rows[i].ups_ref, (unsigned long)qs_rows[i].modal_hash,
                (unsigned long)qs_rows[i].ref_hash, (unsigned long)qs_rows[i].samples_read,
                (long)qs_rows[i].compare_row, (unsigned long)qs_rows[i].compare_n,
                (unsigned long)qs_rows[i].modal_diff_n, (unsigned long)qs_rows[i].modal_max_abs_diff);
            if (single == 4) printf(",%lu", (unsigned long)qs_rows[i].center_lo);
            printf(",%ld,%ld,%lu,%lu,%lu", (long)qs_rows[i].modal_min,
                (long)qs_rows[i].modal_max, (unsigned long)qs_rows[i].lag10_n,
                (unsigned long)qs_rows[i].lag10_diff_n, (unsigned long)qs_rows[i].lag10_max_abs_diff);
            puts("");
            if (single == 4 && i%3u == 2u)
                printf("AP,%u,%lu,%lu,%u,%ld\n", i, (unsigned long)qs_rows[i].center_lo,
                    (unsigned long)qs_rows[i].next_center_lo, qs_rows[i].follow_status,
                    (long)qs_rows[i].det_mhz);
            track3_service_background_budget(16u);
            continue;
        }
        printf("%s,%u,%u,%ld,%lu,%lu,%lu,%llu,%llu,%llu,%ld,%ld,%u,%lu",
            fixed == 3 ? "LB" : (fixed ? "QH" : (single == 2 ? "SL" : (single ? "ST" : "QS"))), i, fixed ? 2u : (i < points ? 0u : 1u), (long)qs_rows[i].offset_hz,
            (unsigned long)qs_rows[i].lo, (unsigned long)qs_rows[i].t_ms,
            (unsigned long)qs_rows[i].duration_us,
            (unsigned long long)qs_rows[i].power[0], (unsigned long long)qs_rows[i].power[1],
            (unsigned long long)qs_rows[i].power[2], (long)qs_rows[i].ref_mhz,
            (long)qs_rows[i].det_mhz, qs_rows[i].vertex_ok, (unsigned long)qs_rows[i].flags);
        if (fixed || single) printf(",%ld,%ld,%lu,%lu,%lu,%lu,%lu", (long)qs_rows[i].ref_first_mhz, (long)qs_rows[i].ref_last_mhz,
            (unsigned long)qs_rows[i].ups_wait, (unsigned long)qs_rows[i].ups_capture, (unsigned long)qs_rows[i].ups_process,
                (unsigned long)qs_rows[i].ups_power, (unsigned long)qs_rows[i].ups_ref);
        if (fixed == 3 || single == 2) printf(",%ld,%ld", (long)qs_rows[i].loop_bb_mhz, (long)qs_rows[i].loop_ref_mhz);
        if (single == 1) printf(",%ld,%ld,%ld", (long)qs_rows[i].loop_ref_mhz,
            (long)qs_rows[i].loop_bb_mhz, (long)qs_rows[i].modal_fit_bb_mhz);
        puts("");
        track3_service_background_budget(16u);
    }
    if (fixed || single) printf("QH diagnostics: startup_ups=%u history_ups=%u\n", startup_ups, history_ups);
    puts(fixed ? "QH end: fixed mixer retained; trackq remains stopped; NCO unchanged" : "QS end: mixer restored; trackq remains stopped; NCO unchanged");
}

static void cmd_trackq_ref_sweep(char *args) {
    uint32_t values[5] = {0u,0u,0u,0u,4000u};
    unsigned argc=0u;
    char *tok=strtok(args, " \t"), *end;
    while (tok && argc<5u) {
        if (*tok<'0' || *tok>'9') break;
        unsigned long value=strtoul(tok,&end,10);
        if (*end || value>32000000ul) break;
        values[argc++]=(uint32_t)value;
        tok=strtok(NULL," \t");
    }
    if (tok || argc<4u || values[0]<1u || values[0]>3u ||
        values[1]<10000u || values[1]>=32000000u) {
        puts("Usage: trackq_ref_sweep <ch 1..3> <physical_center_RF_Hz> <span_Hz> <step_Hz> [settle_ms 1000..5000, default 4000]");
        return;
    }
    qs_ref_center_hz=values[1];
    char normalized[80];
    snprintf(normalized,sizeof(normalized),"%lu %lu %lu %lu", (unsigned long)values[0],
        (unsigned long)values[2],(unsigned long)values[3],(unsigned long)values[4]);
    qs_run(normalized,0,5);
}

static void cmd_loopback_single_sweep(char *args) { qs_run(args, 0, 2); }
static void cmd_trackq_single_sweep(char *args) { qs_run(args, 0, 1); }
static void cmd_trackq_single_cycle(char *args) { qs_run(args, 0, 3); }
static void cmd_trackq_single_follow(char *args) { qs_run(args, 0, 4); }
static void cmd_trackq_sweep(char *args) { qs_run(args, 0, 0); }
static void cmd_trackq_hold(char *args) { qs_run(args, 1, 0); }
static void cmd_trackq_check(char *args) { qs_run(args, 2, 0); }
static void cmd_loopback_check(char *args) { qs_run(args, 3, 0); }

static void cmd_fft64_peak(char *args) {
    (void)args;

    const unsigned n = 64u;

    for (unsigned i = 0; i < n; i++) {
        if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
            printf("Not enough DS FIFO samples: got %u/%u\n", i, n);
            return;
        }

        iq6_frame_t frame;
        ds_fifo_read_frame(&frame);
        int16_t sx = frame.x[0];
        int16_t sy = frame.y[0];

        fft_in[i].r = (kiss_fft_scalar)sx;
        fft_in[i].i = (kiss_fft_scalar)sy;
    }

    size_t cfg_need = 0;
    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES) {
        printf("fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)cfg_need, FFT_CFG_MAX_BYTES);
        return;
    }

    size_t cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    kiss_fft_cfg cfg = kiss_fft_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg) {
        puts("kiss_fft_alloc failed");
        return;
    }

    kiss_fft(cfg, fft_in, fft_out);

    uint64_t peak_pwr = 0;
    unsigned peak_k = 0;
    unsigned bins = (n / 2u);

    for (unsigned k = 1; k < bins; k++) { /* skip DC */
        int32_t re = (int32_t)fft_out[k].r;
        int32_t im = (int32_t)fft_out[k].i;
        uint64_t pwr = (uint64_t)((int64_t)re * re) + (uint64_t)((int64_t)im * im);

        if (pwr > peak_pwr) {
            peak_pwr = pwr;
            peak_k = k;
        }
    }

    {
        uint64_t f_hz = ((uint64_t)peak_k * (uint64_t)fft_fs_hz) / (uint64_t)n;
        printf("fft64 peak: bin=%u f=%llu Hz pwr=%llu (Fs=%lu, N=%u, df=%lu Hz)\n",
               peak_k,
               (unsigned long long)f_hz,
               (unsigned long long)peak_pwr,
               (unsigned long)fft_fs_hz,
               n,
               (unsigned long)(fft_fs_hz / n));
    }
}
static inline unsigned parse_u(const char *s, unsigned max, const char *what) {
    unsigned v = (unsigned)strtoul(s ? s : "0", NULL, 0);
    if (v >= max) printf("Error: %s must be 0..%u\n", what, max - 1);
    return v;
}

static inline int parse_s(const char *s, int minv, int maxv, const char *what) {
    long v = strtol(s ? s : "0", NULL, 0);
    if (v < minv || v > maxv) {
        printf("Error: %s must be %d..%d\n", what, minv, maxv);
    }
    return (int)v;
}

/* ---- COMMIT helper ---- */
static inline void uc_commit(void) {
    cfg_link_commit_write(1);
}

/* ---- ISR ---- */
static void ce_down_isr(void) {
    evm_pending_write(1);
    evm_enable_write(0);
    if (ce_event < 0xffffffffu) ce_event++;
    ce_ticks++;
}

/* ---- Help ---- */
static void uc_help(char *args) {
    (void)args;
    puts_help_header("UberClock commands");

    puts("  phase_nco        <val>      (0..16777215)");
    puts("  nco_mag          <val>      (signed 12-bit: -2048..2047)");

    puts("  phase_down_1 <val> ... phase_down_5 <val>  (0..524287)");
    puts("  phase_down_ref   <val>      (0..524287)");

    puts("  phase_cpu1       <val>      (0..16777215)");
    puts("  phase_cpu2       <val>      (0..16777215)");
    puts("  phase_cpu3       <val>      (0..16777215)");
    puts("  phase_cpu4       <val>      (0..16777215)");
    puts("  phase_cpu5       <val>      (0..16777215)");

    puts("  mag_cpu1         <val>      (signed 12-bit: -2048..2047)");
    puts("  mag_cpu2         <val>      (signed 12-bit: -2048..2047)");
    puts("  mag_cpu3         <val>      (signed 12-bit: -2048..2047)");
    puts("  mag_cpu4         <val>      (signed 12-bit: -2048..2047)");
    puts("  mag_cpu5         <val>      (signed 12-bit: -2048..2047)");

    puts("  input_select         <0..3> (0=ADC,1=NCO,2=SUM,3=reserved)");
    puts("  upsampler_input_mux  <0..2> (0=Gain,1=CPU,2=CPU NCO)");

    puts("  output_select_ch1    <0..15> (14=downsampled_y_ref, 15=sum)");
    puts("  output_select_ch2    <0..15> (14=downsampled_y_ref, 15=sum)");
    puts("  gain1|gain2|gain3|gain4|gain5 <int32>");
    puts("  final_shift          <0..7>");

    puts("  lowspeed_dbg_select  <0..7> (6=downsampled_y_ref)");
    puts("  highspeed_dbg_select <0..3> (1=filter_in_1)");

    puts("  upsampler_x          <val>  (signed 16-bit, replicated to ch1..ch5)");
    puts("  upsampler_y          <val>  (signed 16-bit, replicated to ch1..ch5)");
    puts("  sig3_start                  (start 5 independent 3-tone generators)");
    puts("  sig3_stop                   (stop 5 independent 3-tone generators)");
    puts("  sig3_amp <val> | <ch> <val> (set per-tone amplitude for all or one channel)");
    puts("  sig3_freqs <ch> <f1> <f2> <f3> (set 3-tone frequencies for one channel)");
    puts("  sig3_enable_ch     <ch>     (enable one sig3 channel)");
    puts("  sig3_disable_ch    <ch>     (disable one sig3 channel)");
    puts("  ds_pop                      (pop one 6-channel downsampled frame, ch1..ch5 + ref)");
    puts("  ds_status                   (read downsample FIFO flags/overflow)");
    puts("  ups_push <x> <y>             (enqueue one 5-channel frame, replicated)");
    puts("  ups_status                  (read upsampler FIFO flags/overflow)");
    puts("  dsp_run <0|1>               (enable/disable non-blocking DSP pump)");
    puts("  fft_ds [N]                  (FFT over DS FIFO IQ samples, N=8..2048, prints bins)");
    puts("  fft_ds_peak [N]             (FFT over DS FIFO IQ samples, N=8..2048, peak only)");
    puts("  fft_fs <Hz>                 (set DS sample rate used for fft_ds Hz print)");
    puts("  track3 <ch> <start_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]");
    puts("  trackq_start <C300_mixer_hz> <A100_mixer_hz> <C100_mixer_hz> [N] [center_hz] [delta_ch1_hz] [delta_ch2_hz] [delta_ch3_hz]");
    puts("  trackq_probe [N] [center_hz] [delta_hz]");
    puts("  trackq_stop                 (stop 3-point quadratic tracking)");
    puts("  trackq_deadband [ch pct]    (show/set side deadband; ch 1..3, percent 0..100)");
    puts("  trackq_check <ch 1..3> (two quiet captures; compare three powers)");
    puts("  loopback_check <ch> [samples=90] [wait_ms=2000] (DAC2 to ADC1 vs reference)");
    puts("  trackq_hold <ch> [samples=90] [wait_ms=2000] (quiet fixed mixer test)");
    puts("  loopback_single_sweep <ch> <span_Hz> <step_Hz> [settle_ms] (DAC1 to ADC1, central tone FFT) ");
    puts("  trackq_ref_sweep <ch> <physical_center_RF_Hz> <span_Hz> <step_Hz> [settle_ms=4000]");
    puts("  trackq_single_follow <ch> <span_Hz> <cycles> [settle_ms] (quiet adaptive amplitude peak test)");
    puts("  trackq_single_cycle <ch> <span_Hz> <cycles> [settle_ms] (repeated quiet single-tone triplets)");
    puts("  trackq_single_sweep <ch> <span_Hz> <step_Hz> [settle_ms] (central tone only, settings restored)");
    puts("  trackq_sweep <ch> <span_Hz> <step_Hz> [settle_ms] (quiet buffered sweep)");
    puts("  trackq_observer_diag <0|1> (coherent observer measurements; default off)");
    puts("  trackq_hybrid <0|1>       (shadow synthesis, tau=30 s, compact 5 s log)");
    puts("  trackq_diag <0|1>          (per-update powers and requested/applied correction)");
    puts("  ref_fft_start               (stop trackq; compare reference FFT64/FFT2048/ZC2048)");
    puts("  ref_fft_stop                (stop reference-only measurement)");

    puts("  cap_arm              (pulse arm capture)");
    puts("  cap_done             (read cap_done)");
    puts("  cap_rd <idx>          (read cap_data at idx)");
    puts("  cap_enable   <0|1>    (0=ramp->DDR, 1=capture design->DDR)");
    puts("  cap_beats    <N>      (# of 256-bit beats captured by the gateware)");
    puts("");
}

/* ---- Phase/NCO/downconversion ---- */
static void cmd_phase_nco(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_nco");
    if (p >= (1u << 26)) return;
    main_phase_inc_nco_write(p);
    uc_commit();
    printf("Input NCO phase increment set to %u\n", p);
}

static void cmd_phase_cpu1(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu1");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu1_write(p);
    uc_commit();
    printf("CPU phase increment ch1 set to %u\n", p);
}

static void cmd_phase_cpu2(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu2");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu2_write(p);
    uc_commit();
    printf("CPU phase increment ch2 set to %u\n", p);
}

static void cmd_phase_cpu3(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu3");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu3_write(p);
    uc_commit();
    printf("CPU phase increment ch3 set to %u\n", p);
}

static void cmd_phase_cpu4(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu4");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu4_write(p);
    uc_commit();
    printf("CPU phase increment ch4 set to %u\n", p);
}

static void cmd_phase_cpu5(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_cpu5");
    if (p >= (1u << 26)) return;
    main_phase_inc_cpu5_write(p);
    uc_commit();
    printf("CPU phase increment ch5 set to %u\n", p);
}

static void cmd_nco_mag(char *a) {
    int v = parse_s(a, -2048, 2047, "nco_mag");
    if (v < -2048 || v > 2047) return;
    /* store as 12-bit signed in low bits */
    main_nco_mag_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("nco_mag set to %d\n", v);
}

static void cmd_phase_down_ref(char *a) {
    unsigned p = parse_u(a, 1u << 26, "phase_down_ref");
    if (p >= (1u << 26)) return;
    main_phase_inc_down_ref_write(p);
    uc_commit();
    printf("Downconversion phase ref increment set to %u\n", p);
}

static void cmd_mag_cpu1(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu1");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu1_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu1 set to %d\n", v);
}
static void cmd_mag_cpu2(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu2");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu2_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu2 set to %d\n", v);
}
static void cmd_mag_cpu3(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu3");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu3_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu3 set to %d\n", v);
}
static void cmd_mag_cpu4(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu4");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu4_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu4 set to %d\n", v);
}
static void cmd_mag_cpu5(char *a) {
    int v = parse_s(a, -2048, 2047, "mag_cpu5");
    if (v < -2048 || v > 2047) return;
    main_mag_cpu5_write((uint32_t)((int32_t)v & 0x0fff));
    uc_commit();
    printf("mag_cpu5 set to %d\n", v);
}

static void cmd_lowspeed_dbg_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    if (v > 7) { puts("lowspeed_dbg_select must be 0..7"); return; }
    main_lowspeed_dbg_select_write(v);
    uc_commit();
    printf("lowspeed_dbg_select = %u\n", v);
}

static void cmd_highspeed_dbg_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    if (v > 3) { puts("highspeed_dbg_select must be 0..3"); return; }
    main_highspeed_dbg_select_write(v);
    uc_commit();
    printf("highspeed_dbg_select = %u\n", v);
}

static void cmd_phase_dn(char *a, int ch) {
    unsigned p = parse_u(a, 1u << 26, "phase_down");
    if (p >= (1u << 26)) return;
    switch (ch) {
        case 1: main_phase_inc_down_1_write(p); break;
        case 2: main_phase_inc_down_2_write(p); break;
        case 3: main_phase_inc_down_3_write(p); break;
        case 4: main_phase_inc_down_4_write(p); break;
        case 5: main_phase_inc_down_5_write(p); break;
        default: return;
    }
    uc_commit();
    printf("Downconversion phase ch%d increment set to %u\n", ch, p);
}
static void cmd_phase_down_1(char *a){ cmd_phase_dn(a, 1); }
static void cmd_phase_down_2(char *a){ cmd_phase_dn(a, 2); }
static void cmd_phase_down_3(char *a){ cmd_phase_dn(a, 3); }
static void cmd_phase_down_4(char *a){ cmd_phase_dn(a, 4); }
static void cmd_phase_down_5(char *a){ cmd_phase_dn(a, 5); }

/* ---- Muxes / gains ---- */
static void cmd_output_sel_ch1(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x0fu;
    main_output_select_ch1_write(v);
    uc_commit();
    printf("output_select_ch1 set to %u\n", v);
}

static void cmd_output_sel_ch2(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0) & 0x0fu;
    main_output_select_ch2_write(v);
    uc_commit();
    printf("output_select_ch2 set to %u\n", v);
}

static void cmd_input_select(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    main_input_select_write(v);
    uc_commit();
    printf("Main input select register set to %u\n", v);
}

static void cmd_ups_in_mux(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    main_upsampler_input_mux_write(v);
    uc_commit();
    printf("Upsampler input mux register set to %u\n", v);
}

static void cmd_gain(char *a, int idx) {
    int32_t g = (int32_t)strtol(a ? a : "0", NULL, 0);
    switch (idx) {
        case 1: main_gain1_write((uint32_t)g); break;
        case 2: main_gain2_write((uint32_t)g); break;
        case 3: main_gain3_write((uint32_t)g); break;
        case 4: main_gain4_write((uint32_t)g); break;
        case 5: main_gain5_write((uint32_t)g); break;
        default: return;
    }
    uc_commit();
    printf("Gain%d register set to %ld (0x%08lX)\n",
           idx, (long)g, (unsigned long)g);
}
static void cmd_gain1(char *a){ cmd_gain(a, 1); }
static void cmd_gain2(char *a){ cmd_gain(a, 2); }
static void cmd_gain3(char *a){ cmd_gain(a, 3); }
static void cmd_gain4(char *a){ cmd_gain(a, 4); }
static void cmd_gain5(char *a){ cmd_gain(a, 5); }

static void cmd_final_shift(char *a) {
    int32_t fs = (int32_t)strtol(a ? a : "0", NULL, 0);
    main_final_shift_write((uint32_t)fs);
    uc_commit();
    printf("final_shift set to %ld (0x%08lX)\n",
           (long)fs, (unsigned long)fs);
}

static void cmd_cap_enable(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    v = v ? 1u : 0u;
    main_cap_enable_write(v);
    uc_commit();
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void cmd_upsampler_x(char *a) {
    int v = parse_s(a, -32768, 32767, "upsampler_x");
    if (v < -32768 || v > 32767) return;
    write_upsampler_inputs_all_x((int16_t)v);
    uc_commit();
    printf("upsampler_input_x[1..5] = %d\n", v);
}

static void cmd_upsampler_y(char *a) {
    int v = parse_s(a, -32768, 32767, "upsampler_y");
    if (v < -32768 || v > 32767) return;
    write_upsampler_inputs_all_y((int16_t)v);
    uc_commit();
    printf("upsampler_input_y[1..5] = %d\n", v);
}

static void cap_start_cmd(char *a) {
    (void)a;
    main_cap_arm_write(0);
    uc_commit();

    main_cap_arm_write(1);
    uc_commit();

    main_cap_arm_write(0);
    uc_commit();

    puts("Capture started.");
}

static void cap_status_cmd(char *a) {
    (void)a;
    unsigned d = main_cap_done_read();
    printf("Capture %s\n", d ? "DONE" : "IN-PROGRESS");
}

static void cap_dump_cmd(char *a) {
    (void)a;
    if (!main_cap_done_read()) {
        puts("Capture not done yet. Use 'cap_status' or wait.");
        return;
    }

    puts("#idx,value");
    for (unsigned i = 0; i < 2048; ++i) {
        main_cap_idx_write(i);
        uc_commit();
        (void)main_cap_data_read();
        int16_t v = (int16_t)main_cap_data_read();
        printf("%u,%d\n", i, v);
    }
}
/* ========================================================================= */
/*                         FIFO DSP test harness                             */
/* ========================================================================= */

static inline void dsp_process(int16_t in_x, int16_t in_y,
                               int16_t *out_x, int16_t *out_y) {
    /* Passthrough. */
    *out_x = in_x;
    *out_y = in_y;
}

static unsigned dsp_pump_step(unsigned max_in, unsigned max_out) {
    (void)max_out; /* no loopback push anymore */

    unsigned popped = 0;
    unsigned i;

    for (i = 0; i < max_in; i++) {
        if ((main_ds_fifo_flags_read() & 0x1u) != 0u) {
            iq6_frame_t frame;
            ds_fifo_read_frame(&frame);
            int16_t in_x = frame.x[0];
            int16_t in_y = frame.y[0];

            /* optional processing hook, but no UPS push */
            int16_t out_x = 0, out_y = 0;
            dsp_process(in_x, in_y, &out_x, &out_y);
            (void)out_x;
            (void)out_y;

            /* store for later analysis if wanted */
            dsp_swq_x[dsp_swq_w] = in_x;
            dsp_swq_y[dsp_swq_w] = in_y;
            dsp_swq_w = (dsp_swq_w + 1u) % DSP_SWQ_LEN;
            if (dsp_swq_count < DSP_SWQ_LEN) {
                dsp_swq_count++;
            } else {
                /* overwrite oldest if buffer full */
                dsp_swq_r = (dsp_swq_r + 1u) % DSP_SWQ_LEN;
            }

            popped++;
        } else {
            break;
        }
    }

    return popped;
}

static void fifo_clear_flags(void) {
    main_ds_fifo_clear_write(1);
    main_ups_fifo_clear_write(1);
}

static void cmd_dsp_test(char *args) {
    char *tok = strtok(args, " \t");
    unsigned limit = tok ? (unsigned)strtoul(tok, NULL, 0) : 0;

    /* Temporarily disable background pump so dsp_test owns the flow. */
    int prev_run = dsp_pump_enable;
    dsp_pump_enable = 0;

    unsigned processed = 0;
    unsigned stall = 0;
    const unsigned STALL_MAX = 1000000u;

    while (!limit || processed < limit) {
        unsigned step = dsp_pump_step(64, 64);
        if (step == 0u) {
            if (++stall >= STALL_MAX) break;
            continue;
        }
        stall = 0;
        processed += step;
    }

    dsp_pump_enable = prev_run;

    printf("dsp_test processed %u samples (stall=%u)\n", processed, stall);
}

static void cmd_ds_pop(char *a) {
    (void)a;
    iq6_frame_t frame;
    ds_fifo_read_frame(&frame);
    printf("ds_fifo:"
           " ch1=(%d,%d)"
           " ch2=(%d,%d)"
           " ch3=(%d,%d)"
           " ch4=(%d,%d)"
           " ch5=(%d,%d)"
           " ref=(%d,%d)\n",
           (int)frame.x[0], (int)frame.y[0],
           (int)frame.x[1], (int)frame.y[1],
           (int)frame.x[2], (int)frame.y[2],
           (int)frame.x[3], (int)frame.y[3],
           (int)frame.x[4], (int)frame.y[4],
           (int)frame.x[5], (int)frame.y[5]);
}

static void cmd_ds_status(char *a) {
    (void)a;
    unsigned flags = (unsigned)(main_ds_fifo_flags_read() & 0xffu);
    unsigned overflow = (unsigned)(main_ds_fifo_overflow_read() & 1u);
    unsigned underflow = (unsigned)(main_ds_fifo_underflow_read() & 1u);
    printf("ds_fifo: readable=%u overflow=%u underflow=%u\n", flags & 1u, overflow, underflow);
    main_ds_fifo_clear_write(1);
}

static void cmd_ups_push(char *args) {
    char *tokx = strtok(args, " \t");
    char *toky = strtok(NULL, " \t");
    if (!tokx || !toky) { puts("Usage: ups_push <x> <y>"); return; }
    int x = parse_s(tokx, -32768, 32767, "ups_x");
    int y = parse_s(toky, -32768, 32767, "ups_y");
    if (x < -32768 || x > 32767 || y < -32768 || y > 32767) return;
    ups_fifo_write_replicated((int16_t)x, (int16_t)y);
    printf("ups_fifo push: replicated x=%d y=%d to ch1..ch5\n", x, y);
}

static void cmd_ups_status(char *a) {
    (void)a;
    unsigned flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);
    unsigned overflow = (unsigned)(main_ups_fifo_overflow_read() & 1u);
    unsigned underflow = (unsigned)(main_ups_fifo_underflow_read() & 1u);
    printf("ups_fifo: writable=%u overflow=%u underflow=%u\n",
           (flags >> 1) & 1u, overflow, underflow);
    main_ups_fifo_clear_write(1);
}

static void cmd_dsp_run(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    dsp_pump_enable = v ? 1 : 0;
    if (dsp_pump_enable) {
        dsp_swq_r = 0;
        dsp_swq_w = 0;
        dsp_swq_count = 0;
        dsp_work_tokens = 0;
        main_ds_fifo_clear_write(1);
    } else {
        dsp_work_tokens = 0;
    }
    printf("dsp_run = %u\n", dsp_pump_enable);
}

static void cmd_fft_fs(char *a) {
    uint32_t v = (uint32_t)strtoul(a ? a : "0", NULL, 0);
    if (v == 0u) {
        puts("Usage: fft_fs <Hz>, Hz must be > 0");
        return;
    }
    fft_fs_hz = v;
    printf("fft_fs = %lu Hz\n", (unsigned long)fft_fs_hz);
}

static void run_fft_ds(char *args, int peak_only) {
    char *tok = strtok(args, " \t");
    unsigned n = tok ? (unsigned)strtoul(tok, NULL, 0) : 32u;
    if (!is_pow2_u(n) || n < 8u || n > FFT_MAX_N) {
        printf("Usage: fft_ds [N], N must be power-of-2 and <= %u\n", FFT_MAX_N);
        return;
    }

    for (unsigned i = 0; i < n; i++) {
        if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
            printf("Not enough DS FIFO samples: got %u/%u\n", i, n);
            return;
        }
        iq6_frame_t frame;
        ds_fifo_read_frame(&frame);
        int16_t sx = frame.x[0];
        int16_t sy = frame.y[0];
        fft_in[i].r = (kiss_fft_scalar)sx;
        fft_in[i].i = (kiss_fft_scalar)sy;
    }

    size_t cfg_need = 0;
    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES) {
        printf("fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)cfg_need, FFT_CFG_MAX_BYTES);
        return;
    }

    size_t cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    kiss_fft_cfg cfg = kiss_fft_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg) {
        puts("kiss_fft_alloc failed (static cfg)");
        return;
    }

    kiss_fft(cfg, fft_in, fft_out);

    uint64_t peak_pwr = 0;
    unsigned peak_k = 0;
    unsigned bins = (n / 2u);
    for (unsigned k = 0; k < bins; k++) {
        int32_t re = (int32_t)fft_out[k].r;
        int32_t im = (int32_t)fft_out[k].i;
        uint64_t pwr = (uint64_t)(re * re) + (uint64_t)(im * im);
        if (!peak_only) {
            printf("bin[%4u] re=%7ld im=%7ld pwr=%10llu\n",
                   k, (long)re, (long)im, (unsigned long long)pwr);
        }
        if (k > 0u && pwr > peak_pwr) {
            peak_pwr = pwr;
            peak_k = k;
        }
    }

    if (bins > 1u) {
        uint64_t f_hz = ((uint64_t)peak_k * (uint64_t)fft_fs_hz) / (uint64_t)n;
        printf("peak: bin=%u f=%llu Hz (Fs=%lu, N=%u, pwr=%llu)\n",
               peak_k,
               (unsigned long long)f_hz,
               (unsigned long)fft_fs_hz,
               n,
               (unsigned long long)peak_pwr);
    }
}

static void cmd_fft_ds(char *args) {
    run_fft_ds(args, 0);
}

static void cmd_fft_ds_peak(char *args) {
    run_fft_ds(args, 1);
}

static void cmd_cap_arm_pulse(char *a) {
    (void)a;

    main_cap_arm_write(0);
    uc_commit();

    main_cap_arm_write(1);
    uc_commit();

    main_cap_arm_write(0);
    uc_commit();

    puts("cap_arm pulsed");
}

static void cmd_cap_done(char *a) {
    (void)a;
    printf("cap_done = %u\n", (unsigned)(main_cap_done_read() & 1u));
}

static void cmd_cap_rd(char *args) {
    char *tok = strtok(args, " \t");
    if (!tok) { puts("Usage: cap_rd <idx>"); return; }

    unsigned idx = (unsigned)strtoul(tok, NULL, 0);
    if (idx > 2047) { puts("idx must be 0..2047"); return; }

    main_cap_idx_write(idx);
    uc_commit();

    /* dummy read to allow CDC/update latency */
    (void)main_cap_data_read();
    uint32_t v = main_cap_data_read();

    int16_t s = (int16_t)(v & 0xffff);
    printf("cap[%u] = %d (0x%04x)\n", idx, (int)s, (unsigned)(v & 0xffff));
}

static void cmd_phase_print(char *a) {
    (void)a;
    printf("Phase %ld\n", (long)phase);
}

static void cmd_magnitude(char *a) {
    (void)a;
    printf("Magnitude %d\n", mag);
}

/* ========================================================================= */
/*                     UberDDR3 + S2MM (capture-to-DDR) CLI                   */
/* ========================================================================= */

static inline uint8_t ub_size_to_code(const char *s) {
    if (!s) return 0;             // 00 = bus width
    if (!strcmp(s,"bus")) return 0;
    if (!strcmp(s,"32"))  return 1;
    if (!strcmp(s,"16"))  return 2;
    if (!strcmp(s,"8"))   return 3;
    return 0;
}

static void ub_help(char *args) {
    (void)args;
    puts_help_header("UberDDR3/S2MM commands");
    puts("  ub_info");
    puts("      Print DDR calibration state and CSR base.");
    puts("  ub_mode");
    puts("      Print current capture mode (cap_enable).");
    puts("  ub_setmode <0|1>");
    puts("      Set cap_enable (0=ramp, 1=capture design) and commit.");
    puts("  ub_ramp <addr_hex> [beats] [size]");
    puts("      FORCE ramp mode (cap_enable=0), then start S2MM into DDR.");
    puts("  ub_cap  <addr_hex> [beats] [size]");
    puts("      FORCE capture mode (cap_enable=1), then start S2MM into DDR.");
    puts("  ub_start <addr_hex> [beats] [size]");
    puts("      Start S2MM using CURRENT cap_enable mode.");
    puts("  ub_wait");
    puts("      Poll until DMA not busy; flush caches; print error if any.");
    puts("  ub_hexdump <addr_hex> <bytes>");
    puts("      Dump memory to verify write.");
    puts("  ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
    puts("      Send DDR memory region via UDP to PC.");
    puts("      Example: ub_send 0xA0000000 8192 192.168.0.2 5000");
    puts("");
}

static void cmd_ub_info(char *a) {
    (void)a;
    int cal = 0;
    cal = ubddr3_calib_done_read();
#ifdef CSR_UBDDR3_BASE
    printf("UBDDR3 CSR base: 0x%08lx  calib_done: %d",
           (unsigned long)CSR_UBDDR3_BASE, cal);
#else
    printf("UBDDR3 CSR base: <not exported>  calib_done: %d", cal);
#endif
    printf("\n");
}

static void cmd_ub_mode(char *a) {
    (void)a;
    unsigned v = main_cap_enable_read() & 1u;
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

static void cmd_ub_setmode(char *a) {
    unsigned v = (unsigned)strtoul(a ? a : "0", NULL, 0);
    v = v ? 1u : 0u;
    main_cap_enable_write(v);
    uc_commit();
    printf("cap_enable = %u (%s)\n", v, v ? "CAPTURE(design)->DDR" : "RAMP->DDR");
}

/* Core DMA start helper used by ub_ramp/ub_cap/ub_start */
static void ub_dma_start(uint64_t addr, uint32_t beats, uint8_t size_code) {
    (void)beats;
    (void)size_code;

    ubddr3_dma_inc_write(1);
    ubddr3_dma_size_write(size_code);
    ubddr3_dma_addr0_write((uint32_t)(addr & 0xffffffffu));
    ubddr3_dma_addr1_write((uint32_t)(addr >> 32));

    ubddr3_dma_req_write(1);

}

/* ub_start: run DMA using current cap_enable mode */
static void cmd_ub_start(char *args) {
    char *p = args;
    char *tok_addr  = strtok(p, " \t");
    char *tok_beats = strtok(NULL, " \t");
    char *tok_size  = strtok(NULL, " \t");
    uint64_t addr;
    uint32_t beats;
    uint8_t  sz;

    if (!tok_addr) {
        puts("Usage: ub_start <addr_hex> [beats] [size]");
        return;
    }

    addr = strtoull(tok_addr, NULL, 0);
    beats = (uint32_t)(tok_beats ? strtoul(tok_beats, NULL, 0) : 256u);
    sz = ub_size_to_code(tok_size);

    unsigned mode = main_cap_enable_read() & 1u;

    printf("S2MM start: mode=%s addr=0x%08lx_%08lx beats=%u size=%s\n",
           mode ? "CAPTURE" : "RAMP",
           (unsigned long)(addr >> 32), (unsigned long)(addr & 0xffffffffu),
           (unsigned)beats,
           (sz==0)?"bus":(sz==1)?"32":(sz==2)?"16":"8");

    main_cap_beats_write(beats);
    uc_commit();
    ub_dma_start(addr, beats, sz);
}

/* ub_ramp: force ramp mode then start */
static void cmd_ub_ramp2(char *args) {
    main_cap_enable_write(0);
    uc_commit();
    cmd_ub_start(args);
}

/* ub_cap: force capture mode then start */
static void cmd_ub_cap(char *args) {
    main_cap_enable_write(1);
    uc_commit();
    cmd_ub_start(args);
}

static void cmd_ub_wait(char *a) {
    (void)a;
    printf("Waiting for DMA ... "); fflush(stdout);
    while (ubddr3_dma_busy_read()) ;
    ub_cache_sync();
    puts("done.");
    if (ubddr3_dma_err_read())
        puts("DMA error flag is set!");
}

static void cmd_ub_hexdump(char *a) {
    char *tok_addr = strtok(a, " \t");
    char *tok_len  = strtok(NULL, " \t");
    if (!tok_addr || !tok_len) {
        puts("Usage: ub_hexdump <addr_hex> <bytes>");
        return;
    }
    uint64_t addr = strtoull(tok_addr, NULL, 0);
    uint32_t len  = (uint32_t)strtoul(tok_len, NULL, 0);

    volatile uint8_t *p = (volatile uint8_t*)(uintptr_t)addr;
    for (uint32_t i = 0; i < len; i++) {
        if ((i & 0x0f) == 0)
            printf("\n%08lx: ", (unsigned long)((addr + i) & 0xffffffffu));
        printf("%02x ", p[i]);
    }
    puts("");
}

static void cmd_cap_beats(char *a) {
    uint32_t v = (uint32_t)strtoul(a ? a : "256", NULL, 0);
    if (v == 0) { puts("cap_beats must be >= 1"); return; }
    main_cap_beats_write(v);
    uc_commit();
    printf("cap_beats = %u\n", (unsigned)v);
}

/* ========================================================================= */
/*                           UDP DDR streamer (FAST)                          */
/* ========================================================================= */

#define UBD3_MAGIC 0x55424433u /* "UBD3" */

struct __attribute__((packed)) ubd3_hdr {
    uint32_t magic;
    uint32_t seq;
    uint32_t offset;
    uint32_t total;
};

/* Your board IP (change if you want) */
#define UBD3_BOARD_IP IPTOINT(192,168,0,123)

/* Keep payload below MTU. 1400 is safe; 1472 may work with MTU1500. */
#define UBD3_PAYLOAD_MAX 1400u

/* Call udp_service() once every N packets (power-of-two recommended) */
#define UBD3_SERVICE_EVERY 1u

/* Small pacing delay to reduce host-side RX overruns on long bursts. */
#define UBD3_PACE_WAIT_MS 1u

/* 0 disables progress completely */
#define UBD3_PROGRESS_EVERY 0u

static inline void u32le_store(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 0);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static int parse_ipv4(const char *s, uint32_t *out_ip) {
    unsigned a,b,c,d;
    if (!s) return -1;
    if (sscanf(s, "%u.%u.%u.%u", &a,&b,&c,&d) != 4) return -1;
    if (a>255 || b>255 || c>255 || d>255) return -1;
    *out_ip = IPTOINT(a,b,c,d);
    return 0;
}

/* Fast sender: minimal prints, fewer udp_service() calls, direct header store. */
static void cmd_ub_send(char *args) {
    char *tok_addr = strtok(args, " \t");
    char *tok_len  = strtok(NULL, " \t");
    char *tok_ip   = strtok(NULL, " \t");
    char *tok_port = strtok(NULL, " \t");

    if (!tok_addr || !tok_len || !tok_ip || !tok_port) {
        puts("Usage: ub_send <addr_hex> <bytes> <dst_ip> <dst_port>");
        return;
    }

    uint64_t addr  = strtoull(tok_addr, NULL, 0);
    uint32_t total = (uint32_t)strtoul(tok_len, NULL, 0);

    uint32_t dst_ip = 0;
    if (parse_ipv4(tok_ip, &dst_ip) != 0) {
        puts("Error: bad dst_ip format (use a.b.c.d)");
        return;
    }

    uint16_t dst_port = (uint16_t)strtoul(tok_port, NULL, 0);
    uint16_t src_port = dst_port;

    if (total == 0) {
        puts("Error: bytes must be > 0");
        return;
    }

    static const unsigned char board_mac[6] = {0x02,0x00,0x00,0x00,0x00,0xAB};

    eth_init();
    udp_set_mac(board_mac);
    udp_set_ip(UBD3_BOARD_IP);
    udp_start(board_mac, UBD3_BOARD_IP);

    /* ARP resolve (minimal) */
    int ok = 0;
    for (unsigned i = 0; i < 200000; i++) {
        udp_service();
        if (udp_arp_resolve(dst_ip) != 0) { ok = 1; break; }
    }
    if (!ok) {
        puts("No ARP reply.");
        return;
    }

    volatile uint8_t *p = (volatile uint8_t*)(uintptr_t)addr;

    const uint32_t hdr_sz   = 16u;
    const uint32_t max_data = (UBD3_PAYLOAD_MAX > hdr_sz) ? (UBD3_PAYLOAD_MAX - hdr_sz) : 0u;
    if (max_data < 64u) {
        puts("Error: UBD3_PAYLOAD_MAX too small");
        return;
    }

    uint32_t sent = 0;
    uint32_t seq  = 0;

    /* If SERVICE_EVERY is not power-of-two, fallback to modulo logic */
    const uint32_t service_mask = (UBD3_SERVICE_EVERY && ((UBD3_SERVICE_EVERY & (UBD3_SERVICE_EVERY - 1u)) == 0u))
    ? (UBD3_SERVICE_EVERY - 1u)
    : 0u;

    while (sent < total) {
        /* keep the network stack alive (not every packet) */
        if (UBD3_SERVICE_EVERY) {
            if (service_mask) {
                if ((seq & service_mask) == 0u) udp_service();
            } else {
                if ((seq % UBD3_SERVICE_EVERY) == 0u) udp_service();
            }
        }

        uint32_t chunk = total - sent;
        if (chunk > max_data) chunk = max_data;

        uint8_t *tx = (uint8_t*)udp_get_tx_buffer();
        if (!tx) return;

        /* Write header directly (little-endian) */
        u32le_store(tx + 0,  UBD3_MAGIC);
        u32le_store(tx + 4,  seq);
        u32le_store(tx + 8,  sent);
        u32le_store(tx + 12, total);

        memcpy(tx + hdr_sz, (const void*)(p + sent), chunk);
        (void)udp_send(src_port, dst_port, (unsigned)(hdr_sz + chunk));
        if (UBD3_PACE_WAIT_MS)
            busy_wait(UBD3_PACE_WAIT_MS);

        sent += chunk;
        seq++;

    if (UBD3_PROGRESS_EVERY && ((seq % UBD3_PROGRESS_EVERY) == 0u)) {
        printf("sent %lu / %lu\n", (unsigned long)sent, (unsigned long)total);
    }
    }
}

/* ========================================================================= */
/*                           Command registration                             */
/* ========================================================================= */

static const struct cmd_entry uc_tbl[] = {
    /* UberClock commands */
    {"help_uc",              uc_help,                 "UberClock help"},
{"fft64_peak", cmd_fft64_peak, "64-point FFT over DS FIFO IQ samples, print peak only"},

    {"phase_nco",            cmd_phase_nco,           "Set input CORDIC NCO phase increment"},
    {"nco_mag",              cmd_nco_mag,             "Set NCO magnitude (signed 12-bit)"},

    {"phase_down_1",         cmd_phase_down_1,        "Set downconversion ch1 phase inc"},
    {"phase_down_2",         cmd_phase_down_2,        "Set downconversion ch2 phase inc"},
    {"phase_down_3",         cmd_phase_down_3,        "Set downconversion ch3 phase inc"},
    {"phase_down_4",         cmd_phase_down_4,        "Set downconversion ch4 phase inc"},
    {"phase_down_5",         cmd_phase_down_5,        "Set downconversion ch5 phase inc"},
    {"phase_down_ref",       cmd_phase_down_ref,      "Set downconversion ref phase inc"},

    {"phase_cpu1",           cmd_phase_cpu1,          "Set CPU NCO phase inc ch1"},
    {"phase_cpu2",           cmd_phase_cpu2,          "Set CPU NCO phase inc ch2"},
    {"phase_cpu3",           cmd_phase_cpu3,          "Set CPU NCO phase inc ch3"},
    {"phase_cpu4",           cmd_phase_cpu4,          "Set CPU NCO phase inc ch4"},
    {"phase_cpu5",           cmd_phase_cpu5,          "Set CPU NCO phase inc ch5"},

    {"mag_cpu1",             cmd_mag_cpu1,            "Set CPU NCO magnitude ch1"},
    {"mag_cpu2",             cmd_mag_cpu2,            "Set CPU NCO magnitude ch2"},
    {"mag_cpu3",             cmd_mag_cpu3,            "Set CPU NCO magnitude ch3"},
    {"mag_cpu4",             cmd_mag_cpu4,            "Set CPU NCO magnitude ch4"},
    {"mag_cpu5",             cmd_mag_cpu5,            "Set CPU NCO magnitude ch5"},

    {"output_select_ch1",    cmd_output_sel_ch1,      "Select DAC1 source (0..15, 14=ref_y, 15=sum)"},
    {"output_select_ch2",    cmd_output_sel_ch2,      "Select DAC2 source (0..15, 14=ref_y, 15=sum)"},
    {"input_select",         cmd_input_select,        "Set input select register"},
    {"upsampler_input_mux",  cmd_ups_in_mux,          "Set upsampler input mux (0..2)"},

    {"lowspeed_dbg_select",  cmd_lowspeed_dbg_select, "Select low-speed debug source (0..7, 6=ref_y)"},
    {"highspeed_dbg_select", cmd_highspeed_dbg_select,"Select high-speed debug source (0..3, 1=filter_in_1)"},

    {"upsampler_x",          cmd_upsampler_x,         "Write upsampler_input_x1..x5 (signed 16-bit)"},
    {"upsampler_y",          cmd_upsampler_y,         "Write upsampler_input_y1..y5 (signed 16-bit)"},
    {"ds_pop",               cmd_ds_pop,              "Pop one 6-channel downsampled frame from FIFO"},
    {"ds_status",            cmd_ds_status,           "Show downsample FIFO readable/overflow"},
    {"ups_push",             cmd_ups_push,            "Push one replicated 5-channel frame into upsampler FIFO"},
    {"ups_status",           cmd_ups_status,          "Show upsampler FIFO writable/overflow"},
    {"dsp_test",             cmd_dsp_test,            "Run DSP loop over FIFO samples (optional N)"},
    {"dsp_run",              cmd_dsp_run,             "Enable/disable non-blocking DSP pump"},
    {"fft_fs",               cmd_fft_fs,              "Set DS sample rate (Hz) used by fft_ds"},
    {"fft_ds",               cmd_fft_ds,              "Run FFT over downsample FIFO IQ samples and print bins"},
    {"fft_ds_peak",          cmd_fft_ds_peak,         "Run FFT over downsample FIFO IQ samples and print peak only"},
    {"track3",               cmd_track3,              "Sweep phase_down_<ch> until the 3-tone pattern is found"},
    {"trackq_start",         cmd_trackq_start,        "Start 3-point quadratic tracking: trackq_start <C300_mixer_hz> <A100_mixer_hz> <C100_mixer_hz> [N] [center] [delta1] [delta2] [delta3]"},
    {"trackq_probe",         cmd_trackq_probe,        "Capture one 3-point tracking snapshot"},
    {"loopback_single_sweep", cmd_loopback_single_sweep, "DAC1 to ADC1 single-tone sweep: <ch> <span_Hz> <step_Hz> [settle_ms]"},
    {"trackq_ref_sweep", cmd_trackq_ref_sweep, "Reference-controlled RF sweep: <ch> <center_RF_Hz> <span_Hz> <step_Hz> [settle_ms]"},
    {"trackq_single_follow", cmd_trackq_single_follow, "Quiet adaptive peak: <ch> <span_Hz> <cycles 1..21> [settle_ms]"},
    {"trackq_single_cycle", cmd_trackq_single_cycle, "Quiet repeated triplets: <ch> <span_Hz> <cycles 1..21> [settle_ms]"},
    {"trackq_single_sweep", cmd_trackq_single_sweep, "Quiet single-tone up/down sweep: <ch> <span_Hz> <step_Hz> [settle_ms]"},
    {"trackq_sweep",         cmd_trackq_sweep,        "Quiet buffered up/down sweep: <ch> <span_Hz> <step_Hz> [settle_ms]"},
    {"trackq_hold",          cmd_trackq_hold,         "Quiet fixed mixer test: <ch> [samples=90] [wait_ms=2000]"},
    {"loopback_check",       cmd_loopback_check,      "DAC2 to ADC1 vs reference: <ch> [samples=90] [wait_ms=2000]"},
    {"trackq_check",         cmd_trackq_check,        "Check three powers near resonance: <ch 1..3>"},
    {"trackq_stop",          cmd_trackq_stop,         "Stop 3-point quadratic tracking"},
    {"trackq_deadband",      cmd_trackq_deadband,     "Show/set fallback deadband: [channel 1..3] [percent 0..100]"},
    {"trackq_observer_diag", cmd_trackq_observer_diag, "Enable coherent observer measurements: 0|1"},
    {"trackq_hybrid", cmd_trackq_hybrid, "Shadow C300 temperature/reference synthesis experiment: 0|1"},
    {"trackq_diag",          cmd_trackq_diag,         "Enable/disable per-update tracking decisions: 0|1"},
    {"ref_fft_start",        cmd_ref_fft_start,       "Compare reference FFT64/FFT2048/time-domain crossings every second; stops trackq"},
    {"ref_fft_stop",         cmd_ref_fft_stop,        "Stop reference FFT comparison"},

    {"gain1",                cmd_gain1,               "Set gain1"},
    {"gain2",                cmd_gain2,               "Set gain2"},
    {"gain3",                cmd_gain3,               "Set gain3"},
    {"gain4",                cmd_gain4,               "Set gain4"},
    {"gain5",                cmd_gain5,               "Set gain5"},
    {"final_shift",          cmd_final_shift,         "Set final shift"},

    {"cap_arm",              cmd_cap_arm_pulse,       "Pulse cap_arm"},
    {"cap_done",             cmd_cap_done,            "Read cap_done"},
    {"cap_rd",               cmd_cap_rd,              "Read cap_data at index"},

    {"cap_enable",           cmd_cap_enable,          "0=ramp, 1=capture design to DDR"},
    {"cap_beats",            cmd_cap_beats,           "Set capture length in 256-bit beats"},

    {"phase",                cmd_phase_print,         "Print current CORDIC phase (if wired)"},
    {"magnitude",            cmd_magnitude,           "Print current CORDIC magnitude (if wired)"},
    {"cap_start",            cap_start_cmd,           "Start LS debug"},
    {"cap_status",           cap_status_cmd,          "LS debug status"},
    {"cap_dump",             cap_dump_cmd,             "Ls dump cmd"},
    /* UberDDR3 / S2MM commands */
    {"ub_help",              ub_help,                 "UberDDR3/S2MM help"},
    {"ub_info",              cmd_ub_info,             "Show UBDDR3 info/state"},
    {"ub_mode",              cmd_ub_mode,             "Show current cap_enable mode"},
    {"ub_setmode",           cmd_ub_setmode,          "Set cap_enable (0=ramp,1=capture)"},
    {"ub_start",             cmd_ub_start,            "Start S2MM using current mode"},
    {"ub_ramp",              cmd_ub_ramp2,            "Force ramp mode then start S2MM"},
    {"ub_cap",               cmd_ub_cap,              "Force capture mode then start S2MM"},
    {"ub_wait",              cmd_ub_wait,             "Wait until DMA done"},
    {"ub_hexdump",           cmd_ub_hexdump,          "Hexdump DDR memory"},
    {"ub_send",              cmd_ub_send,             "Send DDR memory region via UDP"},
    {"fft32_ds_y", cmd_fft32_ds_y, "Real FFT of 32 Y samples from DS FIFO"},
    {"sig3_start", cmd_sig3_start, "Start 5 independent 3-tone software generators"},
    {"sig3_stop",  cmd_sig3_stop,  "Stop 5 independent 3-tone software generators"},
    {"sig3_amp",   cmd_sig3_amp,   "Set 3-tone per-tone amplitude: sig3_amp <val> | <ch> <val>"},
    {"sig3_freqs", cmd_sig3_freqs, "Set channel 3-tone frequencies: sig3_freqs <ch> <f1> <f2> <f3>"},
    {"sig3_enable_ch",  cmd_sig3_enable_ch,  "Enable one sig3 channel: sig3_enable_ch <ch>"},
    {"sig3_disable_ch", cmd_sig3_disable_ch, "Disable one sig3 channel: sig3_disable_ch <ch>"},
};

void uberclock_register_cmds(void) {
    console_register(uc_tbl, (unsigned)(sizeof(uc_tbl) / sizeof(uc_tbl[0])));
}

/* ========================================================================= */
/*                            FSM                                             */
/* ========================================================================= */

enum fsm_states {IDLE, S1, S2};
char curr_state;
uint32_t fsm_counter, max_mag, current_phase_inc, max_mag_phase_inc, shooting_phase_inc ;
int8_t sgn = 1;

void fsm_init(void) {
 curr_state = IDLE; 
 ce_ticks = 0;
 max_mag = 0;
 max_mag_phase_inc = 0; 
 shooting_phase_inc = 10328467;
}
void tran(void) {
    switch (curr_state) {
        case IDLE: {
            if (ce_ticks == 9999) {
                curr_state = S1;
            }  else if (ce_ticks == 1) {
                main_phase_inc_nco_write(shooting_phase_inc);
                main_phase_inc_down_1_write(shooting_phase_inc + 1000);  
                puts("Input NCO phase increment set");
            }
        }
        break;
        case S1: {
                     // cmd_magnitude(NULL);
                     if (mag < 30) {
                       curr_state = IDLE;
                       ce_ticks = 0;
                       shooting_phase_inc = shooting_phase_inc + 6;
                       
                     }else 

                     if ( (uint32_t)mag + 10  > max_mag  ) {
                       puts("mag greater");
                       max_mag = mag; 
                       max_mag_phase_inc = shooting_phase_inc;
                       shooting_phase_inc = shooting_phase_inc + sgn * 6;
                       curr_state = IDLE;
                       ce_ticks = 0;
                    } else {
                       //  sgn = -sgn;
                       // shooting_phase_inc = shooting_phase_inc - sgn * 2;
                        main_phase_inc_nco_write(shooting_phase_inc - 6);
                       ce_ticks = 0;
                       curr_state = S2;
                    }
                 }
            break;
        
        case S2: {
            puts("S2");
            // cmd_magnitude(NULL);
            // curr_state = IDLE;
                 }
            break;
    }
}

/* ========================================================================= */
/*                            Init / poll functions                           */
/* ========================================================================= */

void uberclock_init(void) {
    main_phase_inc_nco_write(10324440);

    main_phase_inc_down_1_write(uc_phase_inc_from_hz(TRACKQ_C300_START_MIXER_HZ, TRACK3_RF_FS_HZ));
    main_phase_inc_down_2_write(uc_phase_inc_from_hz(TRACKQ_A100_START_MIXER_HZ, TRACK3_RF_FS_HZ));
    main_phase_inc_down_3_write(uc_phase_inc_from_hz(TRACKQ_C100_START_MIXER_HZ, TRACK3_RF_FS_HZ));
    main_phase_inc_down_4_write(80644);
    main_phase_inc_down_5_write(80640);

    main_phase_inc_down_ref_write(uc_phase_inc_from_hz(9999000u, TRACK3_RF_FS_HZ));

    main_nco_mag_write((uint32_t)(300 & 0x0fff));

    main_phase_inc_cpu1_write(52429);
    main_phase_inc_cpu2_write(52429);
    main_phase_inc_cpu3_write(52429);
    main_phase_inc_cpu4_write(52429);
    main_phase_inc_cpu5_write(52429);

    main_mag_cpu1_write((uint32_t)(0 & 0x0fff));
    main_mag_cpu2_write((uint32_t)(0 & 0x0fff));
    main_mag_cpu3_write((uint32_t)(0 & 0x0fff));
    main_mag_cpu4_write((uint32_t)(0 & 0x0fff));
    main_mag_cpu5_write((uint32_t)(0 & 0x0fff));

    main_input_select_write(0);
    main_upsampler_input_mux_write(1);

    main_gain1_write(0x40000000);
    main_gain2_write(0x40000000);
    main_gain3_write(0x40000000);
    main_gain4_write(0x00000000);
    main_gain5_write(0x00000000);

    main_output_select_ch1_write(15);
    main_output_select_ch2_write(0);

    main_final_shift_write(2);

    main_lowspeed_dbg_select_write(5);
    main_highspeed_dbg_select_write(0);

    write_upsampler_inputs_all_x(0);
    write_upsampler_inputs_all_y(0);

    main_upsampler_input_mux_write(1);
    main_cap_enable_write(1);
    cmd_dsp_run("1");
    //fsm_init();
    sig3_start();
    uc_commit();

    dsp_swq_r = 0;
    dsp_swq_w = 0;
    dsp_swq_count = 0;
    fifo_clear_flags();

    evm_pending_write(1);
    evm_enable_write(1);
    irq_attach(EVM_INTERRUPT, ce_down_isr);
    irq_setmask(irq_getmask() | (1u << EVM_INTERRUPT));

    printf("UberClock init done.\n");
}

void uberclock_poll(void) {
    if (ref_fft_enabled)
        ref_fft_step();
    else
        trackq_step();

    while (ce_event) {
        ce_event--;
        service_one_ce_event();
    }
}
