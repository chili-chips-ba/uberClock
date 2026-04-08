// uberclock.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <irq.h>
#include <libbase/uart.h>
#include <generated/csr.h>
#include <generated/soc.h>
#include "uberclock.h"
#include "console.h"
#include "kissfft/kiss_fft.h"
#include "libliteeth/udp.h"   // LiteEth UDP stack header
                              //
static volatile uint32_t ce_ticks = 0; 
static inline unsigned parse_u(const char *s, unsigned max, const char *what);
static inline int parse_s(const char *s, int minv, int maxv, const char *what);
static inline void uc_commit(void);

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

        main_ds_fifo_pop_write(1);

        /* Read both to complete the FIFO transaction; use Y only */
        (void)main_ds_fifo_x_read();
        int16_t sy = (int16_t)(main_ds_fifo_y_read() & 0xffffu);

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

static volatile int sig3_enable = 0;

/* 32-bit DDS phase accumulators */
static uint32_t sig3_ph_940  = 0;
static uint32_t sig3_ph_1000 = 0;
static uint32_t sig3_ph_1060 = 0;

/* phase increments for Fs = 10 kHz */
static uint32_t sig3_inc_940  = 0;
static uint32_t sig3_inc_1000 = 0;
static uint32_t sig3_inc_1060 = 0;

/* per-tone amplitude in output counts */
static int16_t sig3_amp = 3000;

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
    sig3_ph_940  = 0;
    sig3_ph_1000 = 0;
    sig3_ph_1060 = 0;

    sig3_inc_940  = sig3_phase_inc( 990, 10000);
    sig3_inc_1000 = sig3_phase_inc(1000, 10000);
    sig3_inc_1060 = sig3_phase_inc(1010, 10000);

    sig3_enable = 1;
    puts("3-tone software generator enabled");
}

static void sig3_stop(void) {
    sig3_enable = 0;
    puts("3-tone software generator disabled");
}

static inline int16_t sig3_clamp_s16(int32_t x) {
    if (x >  32767) return  32767;
    if (x < -32768) return -32768;
    return (int16_t)x;
}

static int sig3_step(int16_t *x, int16_t *y) {
    if (!sig3_enable) return 0;

    sig3_ph_940  += sig3_inc_940;
    sig3_ph_1000 += sig3_inc_1000;
    sig3_ph_1060 += sig3_inc_1060;

    int32_t s1 = ((int32_t)sig3_amp * (int32_t)sig3_sin_u32(sig3_ph_940))  / 32767;
    int32_t s2 = ((int32_t)sig3_amp * (int32_t)sig3_sin_u32(sig3_ph_1000)) / 32767;
    int32_t s3 = ((int32_t)sig3_amp * (int32_t)sig3_sin_u32(sig3_ph_1060)) / 32767;

    *x = sig3_clamp_s16(s1 + s2 + s3);
    *y = 0;
    return 1;
}

static void sig3_push_one(void) {
    unsigned flags;
    int16_t x, y;

    flags = (unsigned)(main_ups_fifo_flags_read() & 0xffu);
    if (((flags >> 1) & 1u) == 0u)
        return;

    if (!sig3_step(&x, &y))
        return;

    main_ups_fifo_x_write((uint32_t)((int32_t)x & 0xffff));
    main_ups_fifo_y_write((uint32_t)((int32_t)y & 0xffff));
    main_ups_fifo_push_write(1);
}
static void cmd_sig3_amp(char *a) {
    int v = parse_s(a, 1, 10000, "sig3_amp");
    if (v < 1 || v > 10000) return;
    sig3_amp = (int16_t)v;
    printf("sig3 amplitude per tone = %d\n", sig3_amp);
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
static unsigned dsp_pump_step(unsigned max_in, unsigned max_out);

#define FFT_MAX_N 2048u
#define FFT_CFG_MAX_BYTES 32768u
static kiss_fft_cpx fft_in[FFT_MAX_N];
static kiss_fft_cpx fft_out[FFT_MAX_N];
static uint8_t fft_cfg_mem[FFT_CFG_MAX_BYTES];
static uint32_t fft_fs_hz = 10000u;

#define TRACK3_RF_FS_HZ            65000000u
#define TRACK3_DEFAULT_STEP_HZ     5u
#define TRACK3_DEFAULT_MAX_STEPS   400u
#define TRACK3_DEFAULT_N           2048u
#define TRACK3_DEFAULT_SETTLE      256u
#define TRACK3_DEFAULT_CENTER_HZ   1000u
#define TRACK3_DEFAULT_DELTA_HZ    10u
#define TRACKQ_DEFAULT_DELTA_HZ    10u
#define TRACK3_DEFAULT_BAND_BINS   1u
#define TRACKQ_INTERVAL_TICKS      10000u
#define TRACKQ_CORR_SHIFT          10u
#define TRACKQ_MAX_STEP_HZ         2
#define TRACKQ_FALLBACK_STEP_HZ    1
#define TRACKQ_DIR_DEADBAND_PCT    18u
#define TRACKQ_DIR_CONFIRM_VOTES   3u
#define TRACK3_FIFO_WAIT_POLLS     1000000u
#define TRACK3_SIDE_MIN_PCT        2u
#define TRACK3_SIDE_MAX_PCT        95u
#define TRACK3_SIDE_BALANCE_PCT    40u

struct trackq_state {
    int enabled;
    unsigned n;
    unsigned settle;
    uint32_t center_hz;
    uint32_t delta_hz;
    uint32_t next_tick;
    int pending_dir;
    unsigned pending_votes;
};

static struct trackq_state trackq = {
    0,
    TRACK3_DEFAULT_N,
    TRACK3_DEFAULT_SETTLE,
    TRACK3_DEFAULT_CENTER_HZ,
    TRACKQ_DEFAULT_DELTA_HZ,
    0u,
    0,
    0u
};

static inline int is_pow2_u(unsigned x) {
    return (x != 0u) && ((x & (x - 1u)) == 0u);
}

static uint32_t uc_phase_inc_from_hz(uint32_t f_hz, uint32_t fs_hz) {
    return (uint32_t)((((uint64_t)f_hz << 26) + (fs_hz / 2u)) / (uint64_t)fs_hz);
}

static uint32_t uc_phase_inc_to_hz(uint32_t phase_inc, uint32_t fs_hz) {
    return (uint32_t)((((uint64_t)phase_inc * (uint64_t)fs_hz) + (1u << 25)) >> 26);
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
        main_ds_fifo_pop_write(1);
        (void)main_ds_fifo_x_read();
        (void)main_ds_fifo_y_read();
        (void)main_ds_fifo_x_read();
        (void)main_ds_fifo_y_read();
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

static int capture_ds_fft_ch1(unsigned n, unsigned settle) {
    for (unsigned i = 0; i < settle; i++) {
        if (!track3_wait_ds_fifo("settle", i, settle)) {
            return 0;
        }
        main_ds_fifo_pop_write(1);
        (void)main_ds_fifo_x_read();
        (void)main_ds_fifo_y_read();
        (void)main_ds_fifo_x_read();
        (void)main_ds_fifo_y_read();
        track3_service_background_budget(4);
    }

    for (unsigned i = 0; i < n; i++) {
        int16_t sx;

        if (!track3_wait_ds_fifo("capture", i, n)) {
            return 0;
        }
        main_ds_fifo_pop_write(1);

        /* Match the DS FIFO read sequence used by the working FFT/debug paths. */
        (void)main_ds_fifo_x_read();
        (void)main_ds_fifo_y_read();
        sx = (int16_t)(main_ds_fifo_x_read() & 0xffffu);
        (void)main_ds_fifo_y_read();

        fft_in[i].r = (kiss_fft_scalar)sx;
        fft_in[i].i = (kiss_fft_scalar)0;
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

    if (stop >= bins) {
        stop = bins - 1u;
    }

    for (unsigned i = start; i <= stop; i++) {
        pwr += fft_bin_power_at(i);
    }

    return pwr;
}

static uint64_t track_power_at_hz(uint32_t f_hz, unsigned n) {
    uint32_t phase = 0u;
    uint32_t phase_inc = sig3_phase_inc(f_hz, fft_fs_hz);
    int64_t acc_i = 0;
    int64_t acc_q = 0;

    for (unsigned i = 0; i < n; i++) {
        int32_t sample = (int32_t)fft_in[i].r;
        int32_t cos_q15 = (int32_t)sig3_sin_u32(phase + 0x40000000u);
        int32_t sin_q15 = (int32_t)sig3_sin_u32(phase);

        acc_i += ((int64_t)sample * (int64_t)cos_q15) >> TRACKQ_CORR_SHIFT;
        acc_q -= ((int64_t)sample * (int64_t)sin_q15) >> TRACKQ_CORR_SHIFT;
        phase += phase_inc;
    }

    return (uint64_t)(acc_i * acc_i) + (uint64_t)(acc_q * acc_q);
}

static int track3_triplet_match(uint64_t left_pwr, uint64_t center_pwr, uint64_t right_pwr) {
    uint64_t min_side;
    uint64_t max_side;

    if (center_pwr <= left_pwr || center_pwr <= right_pwr) {
        return 0;
    }
    if (left_pwr == 0u || right_pwr == 0u) {
        return 0;
    }

    min_side = (left_pwr < right_pwr) ? left_pwr : right_pwr;
    max_side = (left_pwr > right_pwr) ? left_pwr : right_pwr;

    if ((min_side * 100u) < (center_pwr * TRACK3_SIDE_MIN_PCT)) {
        return 0;
    }
    if ((max_side * 100u) > (center_pwr * TRACK3_SIDE_MAX_PCT)) {
        return 0;
    }
    if ((min_side * 100u) < (max_side * TRACK3_SIDE_BALANCE_PCT)) {
        return 0;
    }

    return 1;
}

static int32_t trackq_clamp_step_hz(int32_t correction_hz) {
    if (correction_hz > TRACKQ_MAX_STEP_HZ) {
        return TRACKQ_MAX_STEP_HZ;
    }
    if (correction_hz < -TRACKQ_MAX_STEP_HZ) {
        return -TRACKQ_MAX_STEP_HZ;
    }
    return correction_hz;
}

static int trackq_side_bias(uint64_t left_pwr, uint64_t right_pwr) {
    uint64_t diff;
    uint64_t max_side;

    if (left_pwr == right_pwr) {
        return 0;
    }

    diff = (left_pwr > right_pwr) ? (left_pwr - right_pwr) : (right_pwr - left_pwr);
    max_side = (left_pwr > right_pwr) ? left_pwr : right_pwr;

    if ((diff * 100u) < (max_side * TRACKQ_DIR_DEADBAND_PCT)) {
        return 0;
    }

    return (right_pwr > left_pwr) ? 1 : -1;
}

static void trackq_step(void) {
    uint64_t left_pwr;
    uint64_t center_pwr;
    uint64_t right_pwr;
    int64_t num;
    int64_t den;
    int64_t center_hz_milli;
    int64_t h_hz_milli;
    int64_t vertex_hz_milli;
    int32_t correction_hz;
    int32_t applied_hz;
    uint32_t phase_inc;
    uint32_t phase_hz;
    int side_bias;
    const char *mode;

    if (!trackq.enabled) {
        return;
    }
    if (fft_fs_hz == 0u) {
        puts("trackq stopped: fft_fs must be > 0");
        trackq.enabled = 0;
        return;
    }
    if (ce_ticks < trackq.next_tick) {
        return;
    }
    if (trackq.center_hz <= trackq.delta_hz) {
        puts("trackq stopped: center_hz must exceed delta_hz");
        trackq.enabled = 0;
        return;
    }

    if (!capture_ds_fft_ch1(trackq.n, trackq.settle)) {
        trackq.enabled = 0;
        return;
    }

    left_pwr = track_power_at_hz(trackq.center_hz - trackq.delta_hz, trackq.n);
    center_pwr = track_power_at_hz(trackq.center_hz, trackq.n);
    right_pwr = track_power_at_hz(trackq.center_hz + trackq.delta_hz, trackq.n);

    center_hz_milli = (int64_t)trackq.center_hz * 1000ll;
    vertex_hz_milli = center_hz_milli;
    correction_hz = 0;
    mode = "hold";

    if (track3_triplet_match(left_pwr, center_pwr, right_pwr)) {
        num = (int64_t)left_pwr - (int64_t)right_pwr;
        den = 2ll * ((int64_t)left_pwr - (2ll * (int64_t)center_pwr) + (int64_t)right_pwr);
        if (den < 0ll) {
            h_hz_milli = (int64_t)trackq.delta_hz * 1000ll;
            vertex_hz_milli = center_hz_milli + ((h_hz_milli * num) / den);
            correction_hz = (int32_t)((vertex_hz_milli - center_hz_milli) / 1000ll);
            correction_hz = trackq_clamp_step_hz(correction_hz);
            mode = "quad";
        }
    }

    if (correction_hz == 0) {
        side_bias = trackq_side_bias(left_pwr, right_pwr);
        if (side_bias > 0) {
            if (trackq.pending_dir == side_bias) {
                trackq.pending_votes++;
            } else {
                trackq.pending_dir = side_bias;
                trackq.pending_votes = 1u;
            }
            if (trackq.pending_votes >= TRACKQ_DIR_CONFIRM_VOTES) {
                correction_hz = TRACKQ_FALLBACK_STEP_HZ;
                mode = "dir+";
                trackq.pending_votes = 0u;
            } else {
                mode = "wait+";
            }
        } else if (side_bias < 0) {
            if (trackq.pending_dir == side_bias) {
                trackq.pending_votes++;
            } else {
                trackq.pending_dir = side_bias;
                trackq.pending_votes = 1u;
            }
            if (trackq.pending_votes >= TRACKQ_DIR_CONFIRM_VOTES) {
                correction_hz = -TRACKQ_FALLBACK_STEP_HZ;
                mode = "dir-";
                trackq.pending_votes = 0u;
            } else {
                mode = "wait-";
            }
        } else {
            trackq.pending_dir = 0;
            trackq.pending_votes = 0u;
            mode = "hold";
        }
    } else {
        trackq.pending_dir = 0;
        trackq.pending_votes = 0u;
    }

    phase_inc = main_phase_inc_down_1_read();
    phase_hz = uc_phase_inc_to_hz(phase_inc, TRACK3_RF_FS_HZ);
    applied_hz = correction_hz;
    phase_hz = (uint32_t)((int32_t)phase_hz + applied_hz);
    main_phase_inc_down_1_write(uc_phase_inc_from_hz(phase_hz, TRACK3_RF_FS_HZ));
    uc_commit();

    printf("trackq: pwr={%llu,%llu,%llu} vertex=%ld.%03ldHz corr=%ldHz mode=%s phase_down_1=%luHz\n",
           (unsigned long long)left_pwr,
           (unsigned long long)center_pwr,
           (unsigned long long)right_pwr,
           (long)(vertex_hz_milli / 1000ll),
           (long)labs(vertex_hz_milli % 1000ll),
           (long)applied_hz,
           mode,
           (unsigned long)phase_hz);

    trackq.next_tick = ce_ticks + TRACKQ_INTERVAL_TICKS;
}
static void cmd_fft64_peak(char *args) {
    (void)args;

    const unsigned n = 64u;

    for (unsigned i = 0; i < n; i++) {
        if ((main_ds_fifo_flags_read() & 0x1u) == 0u) {
            printf("Not enough DS FIFO samples: got %u/%u\n", i, n);
            return;
        }

        main_ds_fifo_pop_write(1);

        /* safer read pattern after pop */
        (void)main_ds_fifo_x_read();
        (void)main_ds_fifo_y_read();

        int16_t sx = (int16_t)(main_ds_fifo_x_read() & 0xffffu);
        int16_t sy = (int16_t)(main_ds_fifo_y_read() & 0xffffu);

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

    puts("  output_select_ch1    <0..15>");
    puts("  output_select_ch2    <0..15>");
    puts("  gain1|gain2|gain3|gain4|gain5 <int32>");
    puts("  final_shift          <0..7>");

    puts("  lowspeed_dbg_select  <0..4>");
    puts("  highspeed_dbg_select <0..3>");

    puts("  upsampler_x          <val>  (signed 16-bit)");
    puts("  upsampler_y          <val>  (signed 16-bit)");
    puts("  ds_pop                      (pop one downsampled sample)");
    puts("  ds_status                   (read downsample FIFO flags/overflow)");
    puts("  ups_push <x> <y>             (enqueue one upsampler sample)");
    puts("  ups_status                  (read upsampler FIFO flags/overflow)");
    puts("  fft_ds [N]                  (FFT over DS FIFO IQ samples, N=8/16/32)");
    puts("  fft_fs <Hz>                 (set DS sample rate used for fft_ds Hz print)");
    puts("  track3 <start_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]");
    puts("                              (sweep phase_down_1 until center tone dominates)");
    puts("  trackq_start [N] [center_hz] [delta_hz]");
    puts("                              (start 3-point quadratic tracking every 1 s)");
    puts("  trackq_stop                 (stop 3-point quadratic tracking)");

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
    main_upsampler_input_x_write((uint32_t)((int32_t)v & 0xffff));
    uc_commit();
    printf("upsampler_input_x = %d\n", v);
}

static void cmd_upsampler_y(char *a) {
    int v = parse_s(a, -32768, 32767, "upsampler_y");
    if (v < -32768 || v > 32767) return;
    main_upsampler_input_y_write((uint32_t)((int32_t)v & 0xffff));
    uc_commit();
    printf("upsampler_input_y = %d\n", v);
}

static void cap_start_cmd(char *args) {
    (void)args;
    main_cap_arm_write(0);
    uc_commit();

    main_cap_arm_write(1);
    uc_commit();

    main_cap_arm_write(0);
    uc_commit();

    puts("Capture started.");
}

static void cap_status_cmd(char *args) {
    (void)args;
    unsigned d = main_cap_done_read();
    printf("Capture %s\n", d ? "DONE" : "IN-PROGRESS");
}

static void cap_dump_cmd(char *args) {
    (void)args;
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
            main_ds_fifo_pop_write(1);

            /* safer read pattern after pop */
            (void)main_ds_fifo_x_read();
            (void)main_ds_fifo_y_read();

            int16_t in_x = (int16_t)(main_ds_fifo_x_read() & 0xffff);
            int16_t in_y = (int16_t)(main_ds_fifo_y_read() & 0xffff);

            /* optional processing hook for FIFO debugging */
            int16_t out_x = 0, out_y = 0;
            dsp_process(in_x, in_y, &out_x, &out_y);
            (void)out_x;
            (void)out_y;

            popped++;
        } else {
            break;
        }
    }

    return popped;
}

static void cmd_dsp_test(char *args) {
    char *tok = strtok(args, " \t");
    unsigned limit = tok ? (unsigned)strtoul(tok, NULL, 0) : 0;

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

    printf("dsp_test processed %u samples (stall=%u)\n", processed, stall);
}

static void cmd_ds_pop(char *a) {
    (void)a;
    main_ds_fifo_pop_write(1);
    uint32_t vx = main_ds_fifo_x_read();
    uint32_t vy = main_ds_fifo_y_read();
    int16_t sx = (int16_t)(vx & 0xffff);
    int16_t sy = (int16_t)(vy & 0xffff);
    printf("ds_fifo: x=%d y=%d\n", (int)sx, (int)sy);
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
    main_ups_fifo_x_write((uint32_t)((int32_t)x & 0xffff));
    main_ups_fifo_y_write((uint32_t)((int32_t)y & 0xffff));
    main_ups_fifo_push_write(1);
    printf("ups_fifo push: x=%d y=%d\n", x, y);
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

static void cmd_fft_fs(char *a) {
    uint32_t v = (uint32_t)strtoul(a ? a : "0", NULL, 0);
    if (v == 0u) {
        puts("Usage: fft_fs <Hz>, Hz must be > 0");
        return;
    }
    fft_fs_hz = v;
    printf("fft_fs = %lu Hz\n", (unsigned long)fft_fs_hz);
}

static void cmd_fft_ds(char *args) {
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
        main_ds_fifo_pop_write(1);
        int16_t sx = (int16_t)(main_ds_fifo_x_read() & 0xffffu);
        int16_t sy = (int16_t)(main_ds_fifo_y_read() & 0xffffu);
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
        printf("bin[%3u] re=%7ld im=%7ld pwr=%10llu\n",
               k, (long)re, (long)im, (unsigned long long)pwr);
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

static void cmd_track3(char *args) {
    char *tok_start  = strtok(args, " \t");
    char *tok_step   = strtok(NULL, " \t");
    char *tok_steps  = strtok(NULL, " \t");
    char *tok_n      = strtok(NULL, " \t");
    char *tok_center = strtok(NULL, " \t");
    char *tok_delta  = strtok(NULL, " \t");
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

    if (!tok_start) {
        puts("Usage: track3 <start_phase_down_hz> [step_hz] [max_steps] [N] [center_hz] [delta_hz]");
        return;
    }

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

    original_phase_inc = main_phase_inc_down_1_read();
    sweep_hz = start_hz;
    printf("track3: start=%lu Hz step=%lu Hz max_steps=%u N=%u center=%lu Hz delta=%lu Hz Fs=%lu Hz\n",
           (unsigned long)start_hz,
           (unsigned long)step_hz,
           max_steps,
           n,
           (unsigned long)center_hz,
           (unsigned long)delta_hz,
           (unsigned long)fft_fs_hz);

    for (unsigned step = 0; step < max_steps; step++) {
        uint32_t phase_inc = uc_phase_inc_from_hz(sweep_hz, TRACK3_RF_FS_HZ);
        unsigned left_k;
        unsigned center_k;
        unsigned right_k;
        uint64_t left_pwr;
        uint64_t center_pwr;
        uint64_t right_pwr;

        main_phase_inc_down_1_write(phase_inc);
        uc_commit();

        if (!capture_ds_fft_ch1(n, settle)) {
            main_phase_inc_down_1_write(original_phase_inc);
            uc_commit();
            return;
        }

        kiss_fft(cfg, fft_in, fft_out);

        left_k   = (unsigned)((((uint64_t)(center_hz - delta_hz) * (uint64_t)n) + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);
        center_k = (unsigned)((((uint64_t)center_hz             * (uint64_t)n) + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);
        right_k  = (unsigned)((((uint64_t)(center_hz + delta_hz) * (uint64_t)n) + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);

        if (right_k >= (n / 2u)) {
            puts("track3 expected bins exceed FFT Nyquist range");
            main_phase_inc_down_1_write(original_phase_inc);
            uc_commit();
            return;
        }

        left_pwr   = fft_band_power_at(left_k,   band_bins, n);
        center_pwr = fft_band_power_at(center_k, band_bins, n);
        right_pwr  = fft_band_power_at(right_k,  band_bins, n);

        printf("track3 step=%u phase_down_1=%lu Hz inc=%lu bins={%u,%u,%u} pwr={%llu,%llu,%llu}\n",
               step,
               (unsigned long)sweep_hz,
               (unsigned long)phase_inc,
               left_k,
               center_k,
               right_k,
               (unsigned long long)left_pwr,
               (unsigned long long)center_pwr,
               (unsigned long long)right_pwr);

        if (track3_triplet_match(left_pwr, center_pwr, right_pwr)) {
            printf("track3 lock: phase_down_1=%lu Hz inc=%lu center=%lu left=%lu right=%lu\n",
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

    main_phase_inc_down_1_write(original_phase_inc);
    uc_commit();
    printf("track3 no lock found in %u steps; restored phase_down_1=%lu Hz\n",
           max_steps,
           (unsigned long)uc_phase_inc_to_hz(original_phase_inc, TRACK3_RF_FS_HZ));
}

static void cmd_trackq_start(char *args) {
    char *tok_n      = strtok(args, " \t");
    char *tok_center = strtok(NULL, " \t");
    char *tok_delta  = strtok(NULL, " \t");
    unsigned n = tok_n ? (unsigned)strtoul(tok_n, NULL, 0) : TRACK3_DEFAULT_N;
    uint32_t center_hz = tok_center ? (uint32_t)strtoul(tok_center, NULL, 0) : TRACK3_DEFAULT_CENTER_HZ;
    uint32_t delta_hz = tok_delta ? (uint32_t)strtoul(tok_delta, NULL, 0) : TRACKQ_DEFAULT_DELTA_HZ;

    if (!is_pow2_u(n) || n < 8u || n > FFT_MAX_N) {
        printf("trackq_start requires N to be power-of-2 and <= %u\n", FFT_MAX_N);
        return;
    }
    if (center_hz <= delta_hz) {
        puts("trackq_start requires center_hz > delta_hz");
        return;
    }
    if (fft_fs_hz == 0u) {
        puts("trackq_start requires fft_fs > 0");
        return;
    }

    trackq.enabled = 1;
    trackq.n = n;
    trackq.settle = TRACK3_DEFAULT_SETTLE;
    trackq.center_hz = center_hz;
    trackq.delta_hz = delta_hz;
    trackq.next_tick = ce_ticks + TRACKQ_INTERVAL_TICKS;
    trackq.pending_dir = 0;
    trackq.pending_votes = 0u;

    printf("trackq_start: phase_down_1=%lu Hz N=%u center=%lu Hz delta=%lu Hz interval=1 s\n",
           (unsigned long)uc_phase_inc_to_hz(main_phase_inc_down_1_read(), TRACK3_RF_FS_HZ),
           n,
           (unsigned long)center_hz,
           (unsigned long)delta_hz);
}

static void cmd_trackq_stop(char *args) {
    (void)args;
    trackq.enabled = 0;
    puts("trackq_stop: quadratic tracking disabled");
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
    printf("UBDDR3 CSR base: 0x%08lx  calib_done: %d",
           (unsigned long)CSR_UBDDR3_BASE, cal);
    printf("  (UBDDR3_MEM_BASE: 0x%08lx)\n", (unsigned long)UBDDR3_MEM_BASE);
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

    if (!tok_addr) {
        puts("Usage: ub_start <addr_hex> [beats] [size]");
        return;
    }

    uint64_t addr  = strtoull(tok_addr, NULL, 0);
    uint32_t beats = (uint32_t)(tok_beats ? strtoul(tok_beats, NULL, 0) : 256);
    uint8_t  sz    = ub_size_to_code(tok_size);

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
#define UBD3_SERVICE_EVERY 64u

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

    {"output_select_ch1",    cmd_output_sel_ch1,      "Select DAC1 source (0..15)"},
    {"output_select_ch2",    cmd_output_sel_ch2,      "Select DAC2 source (0..15)"},
    {"input_select",         cmd_input_select,        "Set input select register"},
    {"upsampler_input_mux",  cmd_ups_in_mux,          "Set upsampler input mux (0..2)"},

    {"lowspeed_dbg_select",  cmd_lowspeed_dbg_select, "Select low-speed debug source (0..4)"},
    {"highspeed_dbg_select", cmd_highspeed_dbg_select,"Select high-speed debug source (0..3)"},

    {"upsampler_x",          cmd_upsampler_x,         "Write upsampler_input_x (signed 16-bit)"},
    {"upsampler_y",          cmd_upsampler_y,         "Write upsampler_input_y (signed 16-bit)"},
    {"ds_pop",               cmd_ds_pop,              "Pop one downsampled sample from FIFO"},
    {"ds_status",            cmd_ds_status,           "Show downsample FIFO readable/overflow"},
    {"ups_push",             cmd_ups_push,            "Push one sample into upsampler FIFO"},
    {"ups_status",           cmd_ups_status,          "Show upsampler FIFO writable/overflow"},
    {"dsp_test",             cmd_dsp_test,            "Run DSP loop over FIFO samples (optional N)"},
    {"fft_fs",               cmd_fft_fs,              "Set DS sample rate (Hz) used by fft_ds"},
    {"fft_ds",               cmd_fft_ds,              "Run FFT over DS FIFO IQ samples"},
    {"track3",               cmd_track3,              "Sweep phase_down_1 until 3-tone pattern is found"},
    {"trackq_start",         cmd_trackq_start,        "Start 3-point quadratic tracking"},
    {"trackq_stop",          cmd_trackq_stop,         "Stop 3-point quadratic tracking"},

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
    {"sig3_start", cmd_sig3_start, "Start 3-tone software generator"},
    {"sig3_stop",  cmd_sig3_stop,  "Stop 3-tone software generator"},
    {"sig3_amp",   cmd_sig3_amp,   "Set 3-tone per-tone amplitude"},
};

void uberclock_register_cmds(void) {
    console_register(uc_tbl, (unsigned)(sizeof(uc_tbl) / sizeof(uc_tbl[0])));
}

/* ========================================================================= */
/*                            Init / poll functions                           */
/* ========================================================================= */

void uberclock_init(void) {
    main_phase_inc_nco_write(10324440);

    main_phase_inc_down_1_write(10327486);
    main_phase_inc_down_2_write(80652);
    main_phase_inc_down_3_write(80648);
    main_phase_inc_down_4_write(80644);
    main_phase_inc_down_5_write(80640);

    main_phase_inc_down_ref_write(2581110);

    main_nco_mag_write((uint32_t)(500 & 0x0fff));

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
    main_gain2_write(0x00000000);
    main_gain3_write(0x00000000);
    main_gain4_write(0x00000000);
    main_gain5_write(0x00000000);

    main_output_select_ch1_write(5);
    main_output_select_ch2_write(0);

    main_final_shift_write(0);

    main_lowspeed_dbg_select_write(5);
    main_highspeed_dbg_select_write(0);

    main_upsampler_input_x_write(0);
    main_upsampler_input_y_write(0);

    main_upsampler_input_mux_write(1);
    main_cap_enable_write(1);
    sig3_start();
    uc_commit();

    main_ds_fifo_clear_write(1);
    main_ups_fifo_clear_write(1);

    evm_pending_write(1);
    evm_enable_write(1);
    irq_attach(EVM_INTERRUPT, ce_down_isr);
    irq_setmask(irq_getmask() | (1u << EVM_INTERRUPT));

    printf("UberClock init done.\n");
}

void uberclock_poll(void) {
    trackq_step();

    while (ce_event) {
        ce_event--;
        service_one_ce_event();
    }
}
