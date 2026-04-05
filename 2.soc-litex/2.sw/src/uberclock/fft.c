#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uberclock/uberclock_internal.h"

#define FFT_MAX_N 2048u
#define FFT_CFG_MAX_BYTES 12288u
#define TRACK_MODE_DEFAULT_N 2048u
#define TRACK_MODE_SETTLE_SAMPLES 256u
#define TRACK_MODE_DEFAULT_START_HZ 10001000u
#define TRACK_MODE_DEFAULT_MAX_STEPS 2000u
#define TRACK_MODE_RF_FS_HZ 65000000u

static kiss_fft_cpx fft_in[FFT_MAX_N];
static kiss_fft_cpx fft_out[FFT_MAX_N];
static uint8_t fft_cfg_mem[FFT_CFG_MAX_BYTES];
static uint32_t fft_fs_hz = 10000u;

static int prv_is_pow2_u(unsigned x) {
    return (x != 0u) && ((x & (x - 1u)) == 0u);
}

static uint32_t uberclock_int_phase_inc_from_hz(uint32_t f_hz, uint32_t fs_hz) {
    return (uint32_t)(((uint64_t)f_hz << 26) / (uint64_t)fs_hz);
}

static int prv_capture_ds_fft_ch1(unsigned n, unsigned settle) {
    unsigned i;

    uberclock_int_clear_ds_flags();

    for (i = 0; i < settle; i++) {
        iq5_frame_t frame;
        if (!uberclock_int_ds_wait_readable(200000u)) {
            printf("track_mode settle timeout at sample %u/%u\n", i, settle);
            return 0;
        }
        uberclock_int_ds_fifo_read_frame(&frame);
    }

    for (i = 0; i < n; i++) {
        iq5_frame_t frame;
        if (!uberclock_int_ds_wait_readable(200000u)) {
            printf("track_mode capture timeout at sample %u/%u\n", i, n);
            return 0;
        }
        uberclock_int_ds_fifo_read_frame(&frame);
        fft_in[i].r = (kiss_fft_scalar)frame.x[0];
        fft_in[i].i = (kiss_fft_scalar)frame.y[0];
    }

    return 1;
}

static uint64_t prv_fft_bin_power(unsigned k) {
    int32_t re = (int32_t)fft_out[k].r;
    int32_t im = (int32_t)fft_out[k].i;
    return (uint64_t)((int64_t)re * re) + (uint64_t)((int64_t)im * im);
}

static uint64_t prv_fft_band_power(unsigned k, unsigned bins) {
    uint64_t pwr = 0;
    unsigned start = (k > bins) ? (k - bins) : 0u;
    unsigned stop = k + bins;
    unsigned i;

    if (stop >= (FFT_MAX_N / 2u)) {
        stop = (FFT_MAX_N / 2u) - 1u;
    }

    for (i = start; i <= stop; i++) {
        pwr += prv_fft_bin_power(i);
    }

    return pwr;
}

static void prv_cmd_fft32_ds_y(char *args) {
    const unsigned n = 32u;
    const uint32_t fs_hz = 10000u;
    kiss_fft_cpx in[32];
    kiss_fft_cpx out[32];
    uint8_t cfg_mem[768];
    size_t cfg_need = 0;
    size_t cfg_len;
    kiss_fft_cfg cfg;
    unsigned i;
    unsigned k;
    uint64_t peak_pwr = 0;
    unsigned peak_k = 0;

    (void)args;

    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > sizeof(cfg_mem)) {
        printf("fft32 cfg too big: need %lu bytes\n", (unsigned long)cfg_need);
        return;
    }

    cfg_len = sizeof(cfg_mem);
    cfg = kiss_fft_alloc((int)n, 0, cfg_mem, &cfg_len);
    if (!cfg) {
        puts("kiss_fft_alloc failed");
        return;
    }

    for (i = 0; i < n; i++) {
        iq5_frame_t frame;
        if ((uberclock_int_read_ds_flags() & 0x1u) == 0u) {
            printf("Not enough DS FIFO samples: got %u/%u\n", i, n);
            return;
        }
        uberclock_int_ds_fifo_read_frame(&frame);
        in[i].r = (kiss_fft_scalar)frame.y[0];
        in[i].i = (kiss_fft_scalar)0;
    }

    kiss_fft(cfg, in, out);
    puts("bin,freq_hz,re,im,pwr");

    for (k = 0; k < (n / 2u); k++) {
        int32_t re = (int32_t)out[k].r;
        int32_t im = (int32_t)out[k].i;
        uint64_t pwr = (uint64_t)((int64_t)re * re) + (uint64_t)((int64_t)im * im);
        uint64_t f_hz = ((uint64_t)k * (uint64_t)fs_hz) / (uint64_t)n;

        printf("%2u,%5llu,%8ld,%8ld,%12llu\n",
               k, (unsigned long long)f_hz, (long)re, (long)im, (unsigned long long)pwr);

        if (k > 0u && pwr > peak_pwr) {
            peak_pwr = pwr;
            peak_k = k;
        }
    }

    printf("fft32_ds_y peak: bin=%u f=%llu Hz pwr=%llu  (Fs=%lu, N=%u, df=%lu Hz)\n",
           peak_k,
           (unsigned long long)(((uint64_t)peak_k * (uint64_t)fs_hz) / (uint64_t)n),
           (unsigned long long)peak_pwr,
           (unsigned long)fs_hz,
           n,
           (unsigned long)(fs_hz / n));
}

static void prv_run_fft_ds(char *args, int peak_only) {
    char *tok = strtok(args, " \t");
    unsigned n = tok ? (unsigned)strtoul(tok, NULL, 0) : 32u;
    size_t cfg_need = 0;
    size_t cfg_len;
    kiss_fft_cfg cfg;
    unsigned i;
    unsigned k;
    unsigned bins;
    uint64_t peak_pwr = 0;
    unsigned peak_k = 0;

    if (!prv_is_pow2_u(n) || n < 8u || n > FFT_MAX_N) {
        printf("Usage: fft_ds [N], N must be power-of-2 and <= %u\n", FFT_MAX_N);
        return;
    }

    for (i = 0; i < n; i++) {
        iq5_frame_t frame;
        if ((uberclock_int_read_ds_flags() & 0x1u) == 0u) {
            printf("Not enough DS FIFO samples: got %u/%u\n", i, n);
            return;
        }
        uberclock_int_ds_fifo_read_frame(&frame);
        fft_in[i].r = (kiss_fft_scalar)frame.x[0];
        fft_in[i].i = (kiss_fft_scalar)frame.y[0];
    }

    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES) {
        printf("fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)cfg_need, FFT_CFG_MAX_BYTES);
        return;
    }

    cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    cfg = kiss_fft_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg) {
        puts("kiss_fft_alloc failed (static cfg)");
        return;
    }

    kiss_fft(cfg, fft_in, fft_out);

    bins = (n / 2u);
    for (k = 0; k < bins; k++) {
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

static void prv_cmd_track_mode(char *args) {
    char *tok_start = strtok(args, " \t");
    char *tok_steps = strtok(NULL, " \t");
    uint32_t start_hz = tok_start ? (uint32_t)strtoul(tok_start, NULL, 0) : TRACK_MODE_DEFAULT_START_HZ;
    unsigned max_steps = tok_steps ? (unsigned)strtoul(tok_steps, NULL, 0) : TRACK_MODE_DEFAULT_MAX_STEPS;
    unsigned n = TRACK_MODE_DEFAULT_N;
    uint32_t phase_inc;
    size_t cfg_need = 0;
    size_t cfg_len;
    kiss_fft_cfg cfg;
    unsigned k980;
    unsigned k1000;
    unsigned k1020;
    unsigned step;
    uint64_t best_p1000 = 0;
    uint32_t best_phase_inc = 0;
    unsigned have_dominant_peak = 0;
    uint64_t prev_p1000 = 0;
    unsigned have_prev_p1000 = 0;
    const unsigned band_bins = 1u;

    if (!prv_is_pow2_u(n) || n > FFT_MAX_N) {
        puts("track_mode internal FFT length invalid");
        return;
    }
    if (fft_fs_hz == 0u) {
        puts("track_mode requires fft_fs > 0");
        return;
    }

    phase_inc = uberclock_int_phase_inc_from_hz(start_hz, TRACK_MODE_RF_FS_HZ);
    k980 = (unsigned)(((uint64_t)980u * (uint64_t)n + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);
    k1000 = (unsigned)(((uint64_t)1000u * (uint64_t)n + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);
    k1020 = (unsigned)(((uint64_t)1020u * (uint64_t)n + (fft_fs_hz / 2u)) / (uint64_t)fft_fs_hz);

    if (k1020 >= (n / 2u)) {
        printf("track_mode bin selection invalid for fft_fs=%lu Hz, N=%u\n",
               (unsigned long)fft_fs_hz, n);
        return;
    }

    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES) {
        printf("track_mode fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)cfg_need, FFT_CFG_MAX_BYTES);
        return;
    }

    cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    cfg = kiss_fft_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg) {
        puts("track_mode kiss_fft_alloc failed");
        return;
    }

    printf("track_mode: start_hz=%lu start_inc=%lu step_inc=5 bins={980:%u 1000:%u 1020:%u}\n",
           (unsigned long)start_hz,
           (unsigned long)phase_inc,
           k980, k1000, k1020);

    for (step = 0; step < max_steps; step++) {
        uint64_t p980;
        uint64_t p1000;
        uint64_t p1020;

        uberclock_int_write_phase_down(1, phase_inc);
        uberclock_int_commit();

        if (!prv_capture_ds_fft_ch1(n, TRACK_MODE_SETTLE_SAMPLES)) {
            return;
        }

        kiss_fft(cfg, fft_in, fft_out);
        p980 = prv_fft_band_power(k980, band_bins);
        p1000 = prv_fft_band_power(k1000, band_bins);
        p1020 = prv_fft_band_power(k1020, band_bins);

        printf("track_mode step=%u inc=%lu band=%u p980=%llu p1000=%llu p1020=%llu\n",
               step,
               (unsigned long)phase_inc,
               band_bins,
               (unsigned long long)p980,
               (unsigned long long)p1000,
               (unsigned long long)p1020);

        if (have_prev_p1000 && (p1000 > prev_p1000) && (p1000 > p980) && (p1000 > p1020)) {
            if (!have_dominant_peak || (p1000 >= best_p1000)) {
                have_dominant_peak = 1u;
                best_p1000 = p1000;
                best_phase_inc = phase_inc;
            } else {
                uberclock_int_write_phase_down(1, best_phase_inc);
                uberclock_int_commit();
                printf("track_mode found mode: inc=%lu rf_hz~=%.3f peak=%llu\n",
                       (unsigned long)best_phase_inc,
                       ((double)best_phase_inc * (double)TRACK_MODE_RF_FS_HZ) / (double)(1u << 26),
                       (unsigned long long)best_p1000);
                return;
            }
        } else if (have_dominant_peak) {
            uberclock_int_write_phase_down(1, best_phase_inc);
            uberclock_int_commit();
            printf("track_mode found mode: inc=%lu rf_hz~=%.3f peak=%llu\n",
                   (unsigned long)best_phase_inc,
                   ((double)best_phase_inc * (double)TRACK_MODE_RF_FS_HZ) / (double)(1u << 26),
                   (unsigned long long)best_p1000);
            return;
        }

        prev_p1000 = p1000;
        have_prev_p1000 = 1u;
        phase_inc += 5u;
    }

    if (have_dominant_peak) {
        uberclock_int_write_phase_down(1, best_phase_inc);
        uberclock_int_commit();
        printf("track_mode found mode at sweep end: inc=%lu rf_hz~=%.3f peak=%llu\n",
               (unsigned long)best_phase_inc,
               ((double)best_phase_inc * (double)TRACK_MODE_RF_FS_HZ) / (double)(1u << 26),
               (unsigned long long)best_p1000);
    } else {
        printf("track_mode no mode found in %u steps, last_inc=%lu\n",
               max_steps, (unsigned long)phase_inc);
    }
}

static void prv_cmd_fft64_peak(char *args) {
    const unsigned n = 64u;
    size_t cfg_need = 0;
    size_t cfg_len;
    kiss_fft_cfg cfg;
    unsigned i;
    uint64_t peak_pwr = 0;
    unsigned peak_k = 0;

    (void)args;

    for (i = 0; i < n; i++) {
        iq5_frame_t frame;
        if ((uberclock_int_read_ds_flags() & 0x1u) == 0u) {
            printf("Not enough DS FIFO samples: got %u/%u\n", i, n);
            return;
        }
        uberclock_int_ds_fifo_read_frame(&frame);
        fft_in[i].r = (kiss_fft_scalar)frame.x[0];
        fft_in[i].i = (kiss_fft_scalar)frame.y[0];
    }

    (void)kiss_fft_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES) {
        printf("fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)cfg_need, FFT_CFG_MAX_BYTES);
        return;
    }

    cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    cfg = kiss_fft_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg) {
        puts("kiss_fft_alloc failed");
        return;
    }

    kiss_fft(cfg, fft_in, fft_out);

    for (i = 1; i < (n / 2u); i++) {
        int32_t re = (int32_t)fft_out[i].r;
        int32_t im = (int32_t)fft_out[i].i;
        uint64_t pwr = (uint64_t)((int64_t)re * re) + (uint64_t)((int64_t)im * im);

        if (pwr > peak_pwr) {
            peak_pwr = pwr;
            peak_k = i;
        }
    }

    printf("fft64 peak: bin=%u f=%llu Hz pwr=%llu (Fs=%lu, N=%u, df=%lu Hz)\n",
           peak_k,
           (unsigned long long)(((uint64_t)peak_k * (uint64_t)fft_fs_hz) / (uint64_t)n),
           (unsigned long long)peak_pwr,
           (unsigned long)fft_fs_hz,
           n,
           (unsigned long)(fft_fs_hz / n));
}

static void prv_cmd_fft_fs(char *a) {
    uint32_t v = (uint32_t)strtoul(a ? a : "0", NULL, 0);
    if (v == 0u) {
        puts("Usage: fft_fs <Hz>, Hz must be > 0");
        return;
    }
    fft_fs_hz = v;
    printf("fft_fs = %lu Hz\n", (unsigned long)fft_fs_hz);
}

static void prv_cmd_fft_ds(char *args) {
    prv_run_fft_ds(args, 0);
}

static void prv_cmd_fft_ds_peak(char *args) {
    prv_run_fft_ds(args, 1);
}

static const struct cmd_entry g_fft_cmds[] = {
    {"fft32_ds_y",  prv_cmd_fft32_ds_y,  "Real FFT of 32 Y samples from DS FIFO"},
    {"fft64_peak",  prv_cmd_fft64_peak,  "64-point FFT over DS FIFO IQ samples, print peak only"},
    {"fft_fs",      prv_cmd_fft_fs,      "Set DS sample rate (Hz) used by fft_ds"},
    {"fft_ds",      prv_cmd_fft_ds,      "Run FFT over downsample FIFO IQ samples and print bins"},
    {"fft_ds_peak", prv_cmd_fft_ds_peak, "Run FFT over downsample FIFO IQ samples and print peak only"},
    {"track_mode",  prv_cmd_track_mode,  "Sweep phase_down_1 until FFT 1 kHz bin is strongest"},
};

void uberclock_fft_register_cmds(void) {
    console_register(g_fft_cmds, sizeof(g_fft_cmds) / sizeof(g_fft_cmds[0]));
}
