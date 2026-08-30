// SPDX-FileCopyrightText: 2026 Ahmed Imamovic
// SPDX-License-Identifier: CC-BY-SA-4.0

// trackq.c
// Tracking algorithm: track3 coarse-lock acquisition, trackq continuous
// closed-loop tracking, the referent-clock ppm estimate, and the FFT/DS-FIFO
// debug commands built on the same machinery.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <generated/csr.h>
#include "uberclock.h"
#include "sig3.h"
#include "trackq.h"
#include "kiss_fft.h"
#include "kiss_fftr.h"

#define FFT_MAX_N 2048u
#define FFT_CFG_MAX_BYTES 12288u
static kiss_fft_cpx fft_in[FFT_MAX_N];
static kiss_fft_cpx fft_out[FFT_MAX_N];
static uint8_t fft_cfg_mem[FFT_CFG_MAX_BYTES];
/* Real-only input for kiss_fftr call sites (track3 sweep, referent ppm
 * estimate): both grab a single real ADC-domain channel and used to zero-pad
 * an imaginary half into fft_in for a full complex transform. kiss_fftr does
 * the same math in ~half the cycles for genuinely real input. fft_out is
 * still large enough to hold its nfft/2+1 complex output bins for any
 * n <= FFT_MAX_N. Genuinely complex I/Q call sites (fft64_peak, fft_ds) keep
 * using fft_in/kiss_fft unchanged. */
static kiss_fft_scalar fft_real_in[FFT_MAX_N];
static uint32_t fft_fs_hz = 10000u;
static int16_t track_samples[TRACKQ_CHANNELS][FFT_MAX_N];
static int16_t track_samples_ref[FFT_MAX_N];

#define DS_FIFO_HW_DEPTH           16384u

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
    {0, 0u, TRACK3_DEFAULT_N, TRACK3_DEFAULT_SETTLE, TRACK3_DEFAULT_CENTER_HZ, TRACKQ_CH1_DELTA_HZ,  0u, 0, 0},
    {0, 1u, TRACK3_DEFAULT_N, TRACK3_DEFAULT_SETTLE, TRACK3_DEFAULT_CENTER_HZ, TRACKQ_CH2_DELTA_HZ, 0u, 0, 0},
    {0, 2u, TRACK3_DEFAULT_N, TRACK3_DEFAULT_SETTLE, TRACK3_DEFAULT_CENTER_HZ, TRACKQ_CH3_DELTA_HZ, 0u, 0, 0},
};
static uint32_t trackq_log_iteration = 0u;
/* Referent-clock master-NCO filter state: 0 means "not yet seeded". */
static int64_t trackq_ref_filt_fs_hz = 0ll;
static int64_t trackq_ref_applied_fs_hz = 0ll;

static uint32_t trackq_default_delta_hz(unsigned channel) {
    switch (channel) {
        case 0: return TRACKQ_CH1_DELTA_HZ;
        case 1: return TRACKQ_CH2_DELTA_HZ;
        case 2: return TRACKQ_CH3_DELTA_HZ;
        default: return TRACKQ_CH1_DELTA_HZ;
    }
}

static inline int is_pow2_u(unsigned x) {
    return (x != 0u) && ((x & (x - 1u)) == 0u);
}

static int64_t trackq_center_tone_hz_milli(unsigned channel) {
    return sig3_channel_center_hz_milli(channel, trackq[channel].center_hz);
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

        fft_real_in[i] = (kiss_fft_scalar)sx;
        track3_service_background_budget(4);
    }

    return 1;
}

static int capture_ds_track_multi(unsigned n, unsigned settle) {
    unsigned i;

    /* trackq_step() only calls this roughly once per second per due channel;
     * nothing else drains ds_fifo between calls, so hardware keeps pushing
     * frames at 10 kHz the whole time this function is idle. Without a flush
     * here, the settle/capture loops below would silently consume whatever
     * stale backlog piled up since the last call instead of fresh samples,
     * and the backlog only grows call over call until ds_fifo pins at full
     * and every capture runs on ~1.6s-old data forever. Drop it and
     * resynchronize to the live stream before capturing, same as the manual
     * flush already used in cmd_trackq_probe. */
    main_ds_fifo_clear_write(1);
    (void)ds_fifo_flush_all(DS_FIFO_HW_DEPTH);

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
        track_samples[0][i] = frame.x[0];
        track_samples[1][i] = frame.x[1];
        track_samples[2][i] = frame.x[2];
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
        track3_service_background_budget(1u);
}

static uint64_t track_power_at_hz(uint32_t f_hz, unsigned n) {
    uint32_t phase_acc = 0u;
    uint32_t phase_inc = sig3_phase_inc(f_hz, fft_fs_hz);
    int64_t acc_i = 0;
    int64_t acc_q = 0;
    unsigned i;

    for (i = 0; i < n; i++) {
        int32_t sample = (int32_t)fft_real_in[i];
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

static int32_t trackq_side_error_mhz(uint64_t left_pwr, uint64_t right_pwr, uint32_t delta_hz) {
    int64_t diff;
    uint64_t sum;
    uint64_t mag;

    sum = left_pwr + right_pwr;
    if (sum == 0u)
        return 0;

    diff = (int64_t)right_pwr - (int64_t)left_pwr;
    mag = (diff < 0ll) ? (uint64_t)(-diff) : (uint64_t)diff;
    if ((mag * 100u) < (sum * TRACKQ_WEAK_DEADBAND_PCT))
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

static int trackq_fft_peak_vertex_estimate_mhz(const int16_t *samples,
                                               unsigned n,
                                               int64_t *vertex_hz_milli_out) {
    size_t cfg_need = 0;
    size_t cfg_len;
    kiss_fftr_cfg cfg;
    unsigned bins;
    unsigned k_peak = 0u;
    uint64_t p_peak = 0u;
    unsigned k;
    int64_t x1, x2, x3;
    int64_t y1, y2, y3;
    int64_t den;
    int64_t num;

    if (!samples || !vertex_hz_milli_out || !is_pow2_u(n) || n < 8u || n > FFT_MAX_N || fft_fs_hz == 0u)
        return 0;

    for (k = 0u; k < n; k++) {
        fft_real_in[k] = (kiss_fft_scalar)samples[k];
    }

    (void)kiss_fftr_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES)
        return 0;

    cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    cfg = kiss_fftr_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg)
        return 0;

    kiss_fftr(cfg, fft_real_in, fft_out);

    bins = n / 2u;
    for (k = 1u; k + 1u < bins; k++) {
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
    if (y2 <= y1 || y2 <= y3)
        return 0;

    x1 = (int64_t)((((uint64_t)(k_peak - 1u) * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);
    x2 = (int64_t)((((uint64_t)k_peak * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);
    x3 = (int64_t)((((uint64_t)(k_peak + 1u) * (uint64_t)fft_fs_hz * 1000ull) + ((uint64_t)n / 2ull)) / (uint64_t)n);

    den = y1 - (2ll * y2) + y3;
    if (den >= 0ll)
        return 0;

    num = (x3 - x1) * (y1 - y3);
    *vertex_hz_milli_out = x2 + (num / (4ll * den));
    return 1;
}

void trackq_step(void) {
    unsigned i;
    unsigned capture_n = 0u;
    unsigned capture_settle = TRACK3_DEFAULT_SETTLE;
    int any_due = 0;
    int any_enabled = 0;
    int64_t bin_vertex_hf_mhz_log[TRACKQ_CHANNELS];
    int64_t ref_bin_vertex_baseband_mhz_log = (int64_t)TRACK3_DEFAULT_CENTER_HZ * 1000ll;
    int64_t ref_real_fs_mhz_log = (int64_t)TRACK3_RF_FS_HZ * 1000ll;
    int64_t ref_error_ppm_milli_log = 0ll;
    int64_t temp_delta_mc_log = 0ll;
    uint8_t bin_vertex_valid_log[TRACKQ_CHANNELS] = {0u, 0u, 0u};
    uint8_t ref_bin_vertex_valid_log = 0u;
    uint8_t temp_delta_valid_log = 0u;

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

    if (!capture_ds_track_multi(capture_n, capture_settle)) {
        for (i = 0; i < TRACKQ_CHANNELS; i++)
            trackq[i].enabled = 0;
        return;
    }

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
        int32_t applied_hz;
        uint32_t phase_inc;
        uint32_t phase_hz;
        int32_t error_mhz;
        int64_t filt_delta_mhz;
        int64_t ctrl_mhz;
        int confident;
        int weak_mode;

        center_base_hz_milli = trackq_center_tone_hz_milli(i);
        center_hz_milli = (int64_t)trackq[i].center_hz * 1000ll;
        bin_vertex_hf_mhz_log[i] = uc_phase_inc_to_mhz(phase_down_read(i), TRACK3_RF_FS_HZ) +
                                   center_base_hz_milli;
        if (!trackq[i].enabled || ce_ticks < trackq[i].next_tick)
            continue;

        left_pwr = track_band_power_at_hz_samples(track_samples[i], trackq[i].center_hz - trackq[i].delta_hz, trackq[i].n);
        center_pwr = track_band_power_at_hz_samples(track_samples[i], trackq[i].center_hz, trackq[i].n);
        right_pwr = track_band_power_at_hz_samples(track_samples[i], trackq[i].center_hz + trackq[i].delta_hz, trackq[i].n);

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
            error_mhz = trackq_side_error_mhz(left_pwr, right_pwr, trackq[i].delta_hz);
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
        trackq[i].step_accum_mhz += (int32_t)ctrl_mhz;
        correction_hz = trackq[i].step_accum_mhz / 1000;
        correction_hz = weak_mode ? trackq_clamp_weak_step_hz(correction_hz) : trackq_clamp_step_hz(correction_hz);
        trackq[i].step_accum_mhz -= correction_hz * 1000;

        phase_inc = phase_down_read(i);
        phase_hz = uc_phase_inc_to_hz(phase_inc, TRACK3_RF_FS_HZ);
        applied_hz = correction_hz;
        phase_hz = (uint32_t)((int32_t)phase_hz + applied_hz);
        phase_down_write(i, uc_phase_inc_from_hz(phase_hz, TRACK3_RF_FS_HZ));
        phase_hz_milli = uc_phase_inc_to_mhz(uc_phase_inc_from_hz(phase_hz, TRACK3_RF_FS_HZ), TRACK3_RF_FS_HZ);
        if (trackq_bin_vertex_estimate_mhz(track_samples[i], trackq[i].center_hz, trackq[i].n, &bin_vertex_hz_milli)) {
            bin_vertex_hf_mhz_log[i] = phase_hz_milli + center_base_hz_milli + (bin_vertex_hz_milli - center_hz_milli);
            bin_vertex_valid_log[i] = 1u;
        }
        trackq[i].next_tick = ce_ticks + TRACKQ_INTERVAL_TICKS;
    }

    if (trackq_fft_peak_vertex_estimate_mhz(track_samples_ref, capture_n, &ref_bin_vertex_baseband_mhz_log)) {
        int64_t ref_phase_hz_milli = uc_phase_inc_to_mhz(main_phase_inc_down_ref_read(), TRACK3_RF_FS_HZ);
        int64_t ref_meas_hz_milli = ref_phase_hz_milli + ref_bin_vertex_baseband_mhz_log;
        int64_t nominal_fs_hz = (int64_t)TRACK3_RF_FS_HZ;

        if (ref_meas_hz_milli > 0ll) {
            int64_t fs_real_hz = (nominal_fs_hz * (int64_t)TRACKQ_REF_INPUT_HZ * 1000ll) / ref_meas_hz_milli;
            ref_real_fs_mhz_log = fs_real_hz * 1000ll;
            ref_error_ppm_milli_log = ((fs_real_hz - nominal_fs_hz) * 1000000ll * 1000ll) / nominal_fs_hz;

            if (fs_real_hz > 0ll && fs_real_hz <= 0xffffffffll &&
                llabs(fs_real_hz - nominal_fs_hz) <= TRACKQ_REF_MAX_DEVIATION_HZ) {
                int64_t applied_fs_hz;
                int64_t step_hz;

                if (trackq_ref_filt_fs_hz == 0ll) {
                    /* First valid reading: seed the filter instead of easing in from 0. */
                    trackq_ref_filt_fs_hz = fs_real_hz;
                } else {
                    int64_t filt_delta_hz = ((fs_real_hz - trackq_ref_filt_fs_hz) * TRACKQ_REF_ERR_ALPHA_NUM) /
                                             TRACKQ_REF_ERR_ALPHA_DEN;
                    trackq_ref_filt_fs_hz += filt_delta_hz;
                }

                if (trackq_ref_applied_fs_hz == 0ll)
                    trackq_ref_applied_fs_hz = trackq_ref_filt_fs_hz;

                step_hz = trackq_ref_filt_fs_hz - trackq_ref_applied_fs_hz;
                if (step_hz > TRACKQ_REF_MAX_STEP_HZ)
                    step_hz = TRACKQ_REF_MAX_STEP_HZ;
                else if (step_hz < -TRACKQ_REF_MAX_STEP_HZ)
                    step_hz = -TRACKQ_REF_MAX_STEP_HZ;
                trackq_ref_applied_fs_hz += step_hz;

                applied_fs_hz = trackq_ref_applied_fs_hz;
                if (applied_fs_hz > 0ll && applied_fs_hz <= 0xffffffffll)
                    main_phase_inc_nco_write(uc_phase_inc_from_hz(TRACKQ_NCO_TARGET_HZ, (uint32_t)applied_fs_hz));
            }
        }
        ref_bin_vertex_valid_log = 1u;
    }

    if (bin_vertex_valid_log[0] && bin_vertex_valid_log[1] && bin_vertex_valid_log[2]) {
        int64_t d1_mhz = bin_vertex_hf_mhz_log[0] - TRACKQ_TEMP_NOM_CH1_MHZ;
        int64_t d2_mhz = bin_vertex_hf_mhz_log[1] - TRACKQ_TEMP_NOM_CH2_MHZ;
        int64_t d3_mhz = bin_vertex_hf_mhz_log[2] - TRACKQ_TEMP_NOM_CH3_MHZ;

        temp_delta_mc_log =
            ((TRACKQ_TEMP_C1_NC_PER_MHZ * d1_mhz) +
             (TRACKQ_TEMP_C2_NC_PER_MHZ * d2_mhz) +
             (TRACKQ_TEMP_C3_NC_PER_MHZ * d3_mhz)) / 1000000ll;
        temp_delta_valid_log = 1u;
    }

    uc_commit();
    trackq_log_iteration++;
    if ((trackq_log_iteration % 5u) == 0u) {
        printf("trackq hf binfit: ch1=%s%ld.%03ldHz ch2=%s%ld.%03ldHz ch3=%s%ld.%03ldHz ref=%s%ld.%03ldHz fs=%s%ld.%03ldHz err=%s%ld.%03ldppm dtemp=%s%ld.%03ldC\n",
               bin_vertex_valid_log[0] ? "" : "(na)",
               (long)(bin_vertex_hf_mhz_log[0] / 1000ll), (long)llabs(bin_vertex_hf_mhz_log[0] % 1000ll),
               bin_vertex_valid_log[1] ? "" : "(na)",
               (long)(bin_vertex_hf_mhz_log[1] / 1000ll), (long)llabs(bin_vertex_hf_mhz_log[1] % 1000ll),
               bin_vertex_valid_log[2] ? "" : "(na)",
               (long)(bin_vertex_hf_mhz_log[2] / 1000ll), (long)llabs(bin_vertex_hf_mhz_log[2] % 1000ll),
               ref_bin_vertex_valid_log ? "" : "(na)",
               (long)(ref_bin_vertex_baseband_mhz_log / 1000ll), (long)llabs(ref_bin_vertex_baseband_mhz_log % 1000ll),
                ref_bin_vertex_valid_log ? "" : "(na)",
               (long)(ref_real_fs_mhz_log / 1000ll), (long)llabs(ref_real_fs_mhz_log % 1000ll),
               ref_bin_vertex_valid_log ? "" : "(na)",
               (long)(ref_error_ppm_milli_log / 1000ll), (long)llabs(ref_error_ppm_milli_log % 1000ll),
               temp_delta_valid_log ? "" : "(na)",
               (long)(temp_delta_mc_log / 1000ll), (long)llabs(temp_delta_mc_log % 1000ll));
    }
}


void cmd_track3(char *args) {
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
    kiss_fftr_cfg cfg;

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

    (void)kiss_fftr_alloc((int)n, 0, NULL, &cfg_need);
    if (cfg_need > (size_t)FFT_CFG_MAX_BYTES) {
        printf("track3 fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)cfg_need, FFT_CFG_MAX_BYTES);
        return;
    }

    cfg_len = (size_t)FFT_CFG_MAX_BYTES;
    cfg = kiss_fftr_alloc((int)n, 0, fft_cfg_mem, &cfg_len);
    if (!cfg) {
        puts("track3 kiss_fftr_alloc failed");
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

        kiss_fftr(cfg, fft_real_in, fft_out);

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

void cmd_trackq_start(char *args) {
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

    if (!is_pow2_u(n) || n < 8u || n > FFT_MAX_N) {
        printf("trackq_start requires N to be power-of-2 and <= %u\n", FFT_MAX_N);
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
    }

    printf("trackq_start: ch1=%lu Hz ch2=%lu Hz ch3=%lu Hz N=%u center=%lu Hz delta={%lu,%lu,%lu} Hz sig3={{%lu,%lu,%lu},{%lu,%lu,%lu},{%lu,%lu,%lu}} Hz interval=1 s\n",
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

void cmd_trackq_probe(char *args) {
    char *tok_n      = strtok(args, " \t");
    char *tok_center = strtok(NULL, " \t");
    char *tok_delta  = strtok(NULL, " \t");
    unsigned n = tok_n ? (unsigned)strtoul(tok_n, NULL, 0) : TRACK3_DEFAULT_N;
    uint32_t center_hz = tok_center ? (uint32_t)strtoul(tok_center, NULL, 0) : TRACK3_DEFAULT_CENTER_HZ;
    uint32_t delta_hz = tok_delta ? (uint32_t)strtoul(tok_delta, NULL, 0) : TRACKQ_CH1_DELTA_HZ;
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

void cmd_trackq_stop(char *args) {
    (void)args;
    for (unsigned i = 0; i < TRACKQ_CHANNELS; i++)
        trackq[i].enabled = 0;
    puts("trackq_stop: quadratic tracking disabled on ch1..ch3");
}
void cmd_fft64_peak(char *args) {
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

void cmd_fft32_ds_y(char *args) {
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

/* Background FIFO health monitor: the ds_fifo/ups_fifo sticky overflow/underflow
 * bits are otherwise only visible via manual ds_status/ups_status console commands,
 * so an unattended run that drops frames or replays stale samples would go
 * completely unnoticed. Poll them periodically and keep running counters. */
#define FIFO_HEALTH_INTERVAL_TICKS 1000u /* ~100 ms at the 10 kHz ce_down rate */
static uint32_t fifo_health_next_tick = 0u;
static uint32_t ds_fifo_overflow_count = 0u;
static uint32_t ds_fifo_underflow_count = 0u;
static uint32_t ups_fifo_overflow_count = 0u;
static uint32_t ups_fifo_underflow_count = 0u;

void trackq_fifo_health_poll(void) {
    unsigned ov, un;

    if (ce_ticks < fifo_health_next_tick)
        return;
    fifo_health_next_tick = ce_ticks + FIFO_HEALTH_INTERVAL_TICKS;

    ov = (unsigned)(main_ds_fifo_overflow_read() & 1u);
    un = (unsigned)(main_ds_fifo_underflow_read() & 1u);
    if (ov || un) {
        if (ov) ds_fifo_overflow_count++;
        if (un) ds_fifo_underflow_count++;
        main_ds_fifo_clear_write(1);
        printf("ds_fifo health: overflow=%u(total %lu) underflow=%u(total %lu) - tracking loop may be falling behind\n",
               ov, (unsigned long)ds_fifo_overflow_count, un, (unsigned long)ds_fifo_underflow_count);
    }

    ov = (unsigned)(main_ups_fifo_overflow_read() & 1u);
    un = (unsigned)(main_ups_fifo_underflow_read() & 1u);
    if (ov || un) {
        if (ov) ups_fifo_overflow_count++;
        if (un) ups_fifo_underflow_count++;
        main_ups_fifo_clear_write(1);
        printf("ups_fifo health: overflow=%u(total %lu) underflow=%u(total %lu) - TX chain may be replaying stale samples\n",
               ov, (unsigned long)ups_fifo_overflow_count, un, (unsigned long)ups_fifo_underflow_count);
    }
}

void cmd_fifo_health(char *a) {
    (void)a;
    printf("ds_fifo:  overflow_total=%lu underflow_total=%lu\n",
           (unsigned long)ds_fifo_overflow_count, (unsigned long)ds_fifo_underflow_count);
    printf("ups_fifo: overflow_total=%lu underflow_total=%lu\n",
           (unsigned long)ups_fifo_overflow_count, (unsigned long)ups_fifo_underflow_count);
}

void cmd_fft_fs(char *a) {
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

void cmd_fft_ds(char *args) {
    run_fft_ds(args, 0);
}

void cmd_fft_ds_peak(char *args) {
    run_fft_ds(args, 1);
}
