// SPDX-FileCopyrightText: 2026 Ahmed Imamović
// SPDX-FileCopyrightText: 2026 Tarik Hamedović
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdio.h>
#include <stdlib.h>
#include "uberclock/uberclock_config.h"
#include "uberclock/uberclock_runtime.h"
#include "uberclock/uberclock_fft.h"
#include "uberclock/uberclock_fifo.h"
#include "uberclock/uberclock_hw.h"
#include "uberclock/uberclock_channels.h"
#include "uberclock/uberclock_siggen.h"
#include "uberclock/uberclock_track.h"

static int16_t track_samples[UBERCLOCK_TRACK_CHANNEL_COUNT][UBERCLOCK_FFT_MAX_N];
static uint32_t track_log_iteration;

static uint32_t track_default_delta_hz(unsigned channel_index) {
    switch (channel_index) {
        case 0u: return UBERCLOCK_TRACK_CH1_DELTA_HZ;
        case 1u: return UBERCLOCK_TRACK_CH2_DELTA_HZ;
        case 2u: return UBERCLOCK_TRACK_CH3_DELTA_HZ;
        default: return UBERCLOCK_TRACK_CH1_DELTA_HZ;
    }
}

static int triplet_matches(uint64_t left_power, uint64_t center_power, uint64_t right_power) {
    uint64_t min_side;
    uint64_t max_side;

    if (center_power <= left_power || center_power <= right_power) {
        return 0;
    }
    if (left_power == 0u || right_power == 0u) {
        return 0;
    }

    min_side = (left_power < right_power) ? left_power : right_power;
    max_side = (left_power > right_power) ? left_power : right_power;

    if ((min_side * 100u) < (center_power * UBERCLOCK_TRACK_SIDE_MIN_PCT)) {
        return 0;
    }
    if ((max_side * 100u) > (center_power * UBERCLOCK_TRACK_SIDE_MAX_PCT)) {
        return 0;
    }
    if ((min_side * 100u) < (max_side * UBERCLOCK_TRACK_SIDE_BALANCE_PCT)) {
        return 0;
    }

    return 1;
}

static int32_t clamp_tracking_step_hz(int32_t correction_hz) {
    if (correction_hz > UBERCLOCK_TRACK_MAX_STEP_HZ) {
        return UBERCLOCK_TRACK_MAX_STEP_HZ;
    }
    if (correction_hz < -UBERCLOCK_TRACK_MAX_STEP_HZ) {
        return -UBERCLOCK_TRACK_MAX_STEP_HZ;
    }
    return correction_hz;
}

static int32_t clamp_weak_tracking_step_hz(int32_t correction_hz) {
    if (correction_hz > 1) {
        return 1;
    }
    if (correction_hz < -1) {
        return -1;
    }
    return correction_hz;
}

static int vertex_is_confident(uint64_t left_power, uint64_t center_power, uint64_t right_power) {
    uint64_t side_span;

    if (!triplet_matches(left_power, center_power, right_power)) {
        return 0;
    }

    side_span = (left_power > right_power) ? (left_power - right_power) : (right_power - left_power);
    return ((side_span * 100u) >= (center_power * UBERCLOCK_TRACK_MIN_CONF_PCT));
}

static int32_t estimate_side_error_mhz(uint64_t left_power, uint64_t right_power, uint32_t delta_hz) {
    int64_t difference;
    uint64_t power_sum;
    uint64_t magnitude;

    power_sum = left_power + right_power;
    if (power_sum == 0u) {
        return 0;
    }

    difference = (int64_t)right_power - (int64_t)left_power;
    magnitude = (difference < 0ll) ? (uint64_t)(-difference) : (uint64_t)difference;
    if ((magnitude * 100u) < (power_sum * UBERCLOCK_TRACK_WEAK_DEADBAND_PCT)) {
        return 0;
    }

    difference = difference / UBERCLOCK_TRACK_WEAK_GAIN_DEN;
    difference = (difference * (int64_t)delta_hz * 1000ll) / (int64_t)power_sum;
    if (difference > UBERCLOCK_TRACK_WEAK_MAX_ERR_MHZ) {
        return UBERCLOCK_TRACK_WEAK_MAX_ERR_MHZ;
    }
    if (difference < -UBERCLOCK_TRACK_WEAK_MAX_ERR_MHZ) {
        return -UBERCLOCK_TRACK_WEAK_MAX_ERR_MHZ;
    }
    return (int32_t)difference;
}

static int validate_tracking_request(const char *name,
                                     unsigned sample_count,
                                     uint32_t center_hz,
                                     uint32_t delta_hz) {
    if (!uberclock_fft_is_power_of_two(sample_count) ||
        sample_count < 8u ||
        sample_count > UBERCLOCK_FFT_MAX_N) {
        printf("%s requires N to be power-of-2 and <= %u\n", name, UBERCLOCK_FFT_MAX_N);
        return 0;
    }
    if (center_hz <= delta_hz) {
        printf("%s requires center_hz > delta_hz\n", name);
        return 0;
    }
    if (uberclock_fft_sample_rate() == 0u) {
        printf("%s requires fft_fs > 0\n", name);
        return 0;
    }

    return 1;
}

static int capture_ds_fft_channel(unsigned channel_index, unsigned sample_count, unsigned settle_samples) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    unsigned sample_index;

    for (sample_index = 0u; sample_index < settle_samples; ++sample_index) {
        struct uberclock_iq_frame frame;

        if (!uberclock_wait_for_ds_sample("settle", sample_index, settle_samples)) {
            return 0;
        }
        (void)uberclock_ds_fifo_pop_frame(&frame);
        uberclock_runtime_service_ce_events(4u);
    }

    for (sample_index = 0u; sample_index < sample_count; ++sample_index) {
        struct uberclock_iq_frame frame;

        if (!uberclock_wait_for_ds_sample("capture", sample_index, sample_count)) {
            return 0;
        }
        (void)uberclock_ds_fifo_pop_frame(&frame);
        fft->fft_in[sample_index].r = (kiss_fft_scalar)frame.x[channel_index];
        fft->fft_in[sample_index].i = (kiss_fft_scalar)0;
        uberclock_runtime_service_ce_events(4u);
    }

    return 1;
}

static int capture_ds_track_multi(unsigned sample_count, unsigned settle_samples) {
    unsigned sample_index;

    for (sample_index = 0u; sample_index < settle_samples; ++sample_index) {
        struct uberclock_iq_frame frame;

        if (!uberclock_wait_for_ds_sample("settle", sample_index, settle_samples)) {
            return 0;
        }
        (void)uberclock_ds_fifo_pop_frame(&frame);
        uberclock_runtime_service_ce_events(4u);
    }

    for (sample_index = 0u; sample_index < sample_count; ++sample_index) {
        struct uberclock_iq_frame frame;

        if (!uberclock_wait_for_ds_sample("capture", sample_index, sample_count)) {
            return 0;
        }
        (void)uberclock_ds_fifo_pop_frame(&frame);
        track_samples[0][sample_index] = frame.x[0];
        track_samples[1][sample_index] = frame.x[1];
        track_samples[2][sample_index] = frame.x[2];
        uberclock_runtime_service_ce_events(4u);
    }

    return 1;
}

static int16_t sine_lookup(uint32_t phase) {
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
    uint8_t quadrant = (uint8_t)(phase >> 30);
    uint8_t index = (uint8_t)((phase >> 24) & 0x3fu);

    switch (quadrant) {
        case 0u: return sine_q64[index];
        case 1u: return sine_q64[63u - index];
        case 2u: return (int16_t)(-sine_q64[index]);
        default: return (int16_t)(-sine_q64[63u - index]);
    }
}

static uint64_t track_power_at_hz_samples(const int16_t *samples, uint32_t frequency_hz, unsigned sample_count) {
    uint32_t phase = 0u;
    uint32_t phase_increment = (uint32_t)(((uint64_t)frequency_hz << 32) / uberclock_fft_sample_rate());
    int64_t accumulator_i = 0;
    int64_t accumulator_q = 0;
    unsigned sample_index;

    for (sample_index = 0u; sample_index < sample_count; ++sample_index) {
        int32_t sample = (int32_t)samples[sample_index];
        int32_t cosine_q15 = (int32_t)sine_lookup(phase + 0x40000000u);
        int32_t sine_q15 = (int32_t)sine_lookup(phase);

        accumulator_i += ((int64_t)sample * (int64_t)cosine_q15) >> UBERCLOCK_TRACK_CORR_SHIFT;
        accumulator_q -= ((int64_t)sample * (int64_t)sine_q15) >> UBERCLOCK_TRACK_CORR_SHIFT;
        phase += phase_increment;

        if ((sample_index & (UBERCLOCK_TRACK_BG_SERVICE_PERIOD - 1u)) == (UBERCLOCK_TRACK_BG_SERVICE_PERIOD - 1u)) {
            uberclock_runtime_service_ce_events(1u);
        }
    }

    return (uint64_t)(accumulator_i * accumulator_i) + (uint64_t)(accumulator_q * accumulator_q);
}

static uint64_t track_band_power_at_hz_samples(const int16_t *samples,
                                               uint32_t frequency_hz,
                                               unsigned sample_count) {
    uint64_t center_power = track_power_at_hz_samples(samples, frequency_hz, sample_count);
    uint32_t frequency_resolution_hz;
    uint64_t lower_power;
    uint64_t upper_power;

    if (uberclock_fft_sample_rate() == 0u || sample_count == 0u) {
        return center_power;
    }

    frequency_resolution_hz =
        (uint32_t)((((uint64_t)uberclock_fft_sample_rate()) + (sample_count / 2u)) / (uint64_t)sample_count);
    if (frequency_resolution_hz == 0u) {
        return center_power;
    }

    lower_power = track_power_at_hz_samples(
        samples,
        (frequency_hz > frequency_resolution_hz) ? (frequency_hz - frequency_resolution_hz) : 0u,
        sample_count);
    upper_power = track_power_at_hz_samples(samples, frequency_hz + frequency_resolution_hz, sample_count);

    return center_power + ((lower_power + upper_power) >> 1);
}

static int64_t track_phase_inc_to_mhz(uint32_t phase_increment, uint32_t sample_rate_hz) {
    return (int64_t)((((uint64_t)phase_increment * (uint64_t)sample_rate_hz * 1000ull) + (1u << 25)) >> 26);
}

static int64_t center_tone_hz_milli(unsigned channel_index) {
    struct uberclock_track_state *track = uberclock_track_state();

    if (channel_index < UBERCLOCK_CHANNEL_COUNT &&
        uberclock_siggen_state()->enabled &&
        uberclock_siggen_channel_enabled(channel_index) &&
        track[channel_index].center_hz == uberclock_siggen_channel_frequency(channel_index, 1u)) {
        return uberclock_siggen_increment_to_mhz(uberclock_siggen_channel_increment(channel_index, 1u), 10000u);
    }

    return (int64_t)track[channel_index].center_hz * 1000ll;
}

void uberclock_track_poll(void) {
    struct uberclock_track_state *track = uberclock_track_state();
    unsigned channel_index;
    unsigned capture_n = 0u;
    unsigned capture_settle = UBERCLOCK_TRACK_DEFAULT_SETTLE;
    int any_due = 0;
    int any_enabled = 0;
    int64_t vertex_hf_mhz_log[UBERCLOCK_TRACK_CHANNEL_COUNT] = {0, 0, 0};

    for (channel_index = 0u; channel_index < UBERCLOCK_TRACK_CHANNEL_COUNT; ++channel_index) {
        if (!track[channel_index].enabled) {
            continue;
        }
        any_enabled = 1;
        if (uberclock_fft_sample_rate() == 0u) {
            puts("trackq stopped: fft_fs must be > 0");
            track[channel_index].enabled = 0;
            continue;
        }
        if (track[channel_index].center_hz <= track[channel_index].delta_hz) {
            puts("trackq stopped: center_hz must exceed delta_hz");
            track[channel_index].enabled = 0;
            continue;
        }
        if (uberclock_runtime_state()->ce_ticks >= track[channel_index].next_tick) {
            any_due = 1;
            if (capture_n == 0u) {
                capture_n = track[channel_index].n;
                capture_settle = track[channel_index].settle;
            }
        }
    }

    if (!any_enabled || !any_due) {
        return;
    }

    if (!capture_ds_track_multi(capture_n, capture_settle)) {
        for (channel_index = 0u; channel_index < UBERCLOCK_TRACK_CHANNEL_COUNT; ++channel_index) {
            track[channel_index].enabled = 0;
        }
        return;
    }

    for (channel_index = 0u; channel_index < UBERCLOCK_TRACK_CHANNEL_COUNT; ++channel_index) {
        uint64_t left_power;
        uint64_t center_power;
        uint64_t right_power;
        int64_t numerator;
        int64_t denominator;
        int64_t center_hz_milli;
        int64_t center_base_hz_milli;
        int64_t delta_hz_milli;
        int64_t vertex_hz_milli;
        int64_t phase_hz_milli;
        int32_t correction_hz;
        int32_t error_mhz;
        int64_t filter_delta_mhz;
        int64_t control_mhz;
        uint32_t phase_hz;
        int weak_mode;

        center_base_hz_milli = center_tone_hz_milli(channel_index);
        vertex_hf_mhz_log[channel_index] =
            track_phase_inc_to_mhz(uberclock_channel_get_phase_down(channel_index), UBERCLOCK_TRACK_RF_FS_HZ) +
            center_base_hz_milli;
        if (!track[channel_index].enabled ||
            uberclock_runtime_state()->ce_ticks < track[channel_index].next_tick) {
            continue;
        }

        left_power = track_band_power_at_hz_samples(track_samples[channel_index],
                                                    track[channel_index].center_hz - track[channel_index].delta_hz,
                                                    track[channel_index].n);
        center_power = track_band_power_at_hz_samples(track_samples[channel_index],
                                                      track[channel_index].center_hz,
                                                      track[channel_index].n);
        right_power = track_band_power_at_hz_samples(track_samples[channel_index],
                                                     track[channel_index].center_hz + track[channel_index].delta_hz,
                                                     track[channel_index].n);

        center_hz_milli = (int64_t)track[channel_index].center_hz * 1000ll;
        vertex_hz_milli = center_hz_milli;
        error_mhz = 0;
        weak_mode = 0;

        if (vertex_is_confident(left_power, center_power, right_power)) {
            numerator = (int64_t)left_power - (int64_t)right_power;
            denominator = 2ll * ((int64_t)left_power - (2ll * (int64_t)center_power) +
                                 (int64_t)right_power);
            if (denominator < 0ll) {
                delta_hz_milli = (int64_t)track[channel_index].delta_hz * 1000ll;
                vertex_hz_milli = center_hz_milli + ((delta_hz_milli * numerator) / denominator);
                error_mhz = (int32_t)(vertex_hz_milli - center_hz_milli);
            }
        } else {
            error_mhz = estimate_side_error_mhz(left_power, right_power, track[channel_index].delta_hz);
            if (error_mhz != 0) {
                weak_mode = 1;
            } else {
                track[channel_index].filtered_error_mhz =
                    (track[channel_index].filtered_error_mhz *
                     (UBERCLOCK_TRACK_ERR_ALPHA_DEN - UBERCLOCK_TRACK_ERR_ALPHA_NUM)) /
                    UBERCLOCK_TRACK_ERR_ALPHA_DEN;
            }
        }

        filter_delta_mhz =
            ((int64_t)(error_mhz - track[channel_index].filtered_error_mhz) *
             (int64_t)UBERCLOCK_TRACK_ERR_ALPHA_NUM) /
            (int64_t)UBERCLOCK_TRACK_ERR_ALPHA_DEN;
        track[channel_index].filtered_error_mhz += (int32_t)filter_delta_mhz;

        control_mhz =
            ((int64_t)track[channel_index].filtered_error_mhz * (int64_t)UBERCLOCK_TRACK_KP_NUM) /
            (int64_t)UBERCLOCK_TRACK_KP_DEN;
        track[channel_index].step_accumulator_mhz += (int32_t)control_mhz;
        correction_hz = track[channel_index].step_accumulator_mhz / 1000;
        correction_hz = weak_mode ? clamp_weak_tracking_step_hz(correction_hz)
                                  : clamp_tracking_step_hz(correction_hz);
        track[channel_index].step_accumulator_mhz -= correction_hz * 1000;

        phase_hz = uberclock_phase_inc_to_hz(uberclock_channel_get_phase_down(channel_index),
                                             UBERCLOCK_TRACK_RF_FS_HZ);
        phase_hz = (uint32_t)((int32_t)phase_hz + correction_hz);
        (void)uberclock_channel_set_phase_down(channel_index,
                                               uberclock_phase_inc_from_hz(phase_hz, UBERCLOCK_TRACK_RF_FS_HZ));
        phase_hz_milli =
            track_phase_inc_to_mhz(uberclock_phase_inc_from_hz(phase_hz, UBERCLOCK_TRACK_RF_FS_HZ),
                                   UBERCLOCK_TRACK_RF_FS_HZ);
        vertex_hf_mhz_log[channel_index] =
            phase_hz_milli + center_base_hz_milli + (vertex_hz_milli - center_hz_milli);
        track[channel_index].next_tick = uberclock_runtime_state()->ce_ticks + UBERCLOCK_TRACK_INTERVAL_TICKS;
    }

    uberclock_commit_config();
    track_log_iteration++;
    if ((track_log_iteration % 5u) == 0u) {
        printf("trackq hf vertex: ch1=%ld.%03ldHz ch2=%ld.%03ldHz ch3=%ld.%03ldHz\n",
               (long)(vertex_hf_mhz_log[0] / 1000ll), (long)llabs(vertex_hf_mhz_log[0] % 1000ll),
               (long)(vertex_hf_mhz_log[1] / 1000ll), (long)llabs(vertex_hf_mhz_log[1] % 1000ll),
               (long)(vertex_hf_mhz_log[2] / 1000ll), (long)llabs(vertex_hf_mhz_log[2] % 1000ll));
    }
}

void uberclock_track_stop(void) {
    struct uberclock_track_state *track = uberclock_track_state();
    unsigned channel_index;

    for (channel_index = 0u; channel_index < UBERCLOCK_TRACK_CHANNEL_COUNT; ++channel_index) {
        track[channel_index].enabled = 0;
    }
    puts("trackq_stop: quadratic tracking disabled on ch1..ch3");
}

int uberclock_track3_run(uint32_t start_hz,
                         unsigned channel_index,
                         uint32_t step_hz,
                         unsigned max_steps,
                         unsigned sample_count,
                         uint32_t center_hz,
                         uint32_t delta_hz) {
    uint32_t original_phase_increment;
    uint32_t sweep_hz;
    unsigned step_index;

    if (channel_index >= UBERCLOCK_CHANNEL_COUNT) {
        puts("track3 channel must be 1..5");
        return -1;
    }
    if (step_hz == 0u || max_steps == 0u) {
        puts("track3 requires step_hz > 0 and max_steps > 0");
        return -1;
    }
    if (!validate_tracking_request("track3", sample_count, center_hz, delta_hz)) {
        return -1;
    }

    uberclock_siggen_set_channel_symmetric(channel_index, center_hz, delta_hz);
    uberclock_siggen_enable_channel(channel_index);

    original_phase_increment = uberclock_channel_get_phase_down(channel_index);
    sweep_hz = start_hz;
    printf("track3: ch=%u start=%lu Hz step=%lu Hz max_steps=%u N=%u center=%lu Hz delta=%lu Hz Fs=%lu Hz sig3={%lu,%lu,%lu} Hz\n",
           channel_index + 1u,
           (unsigned long)start_hz,
           (unsigned long)step_hz,
           max_steps,
           sample_count,
           (unsigned long)center_hz,
           (unsigned long)delta_hz,
           (unsigned long)uberclock_fft_sample_rate(),
           (unsigned long)(center_hz - delta_hz),
           (unsigned long)center_hz,
           (unsigned long)(center_hz + delta_hz));

    for (step_index = 0u; step_index < max_steps; ++step_index) {
        uint32_t phase_increment = uberclock_phase_inc_from_hz(sweep_hz, UBERCLOCK_TRACK_RF_FS_HZ);
        unsigned left_bin;
        unsigned center_bin;
        unsigned right_bin;
        uint64_t left_power;
        uint64_t center_power;
        uint64_t right_power;

        (void)uberclock_channel_set_phase_down(channel_index, phase_increment);
        uberclock_commit_config();

        if (!capture_ds_fft_channel(channel_index, sample_count, UBERCLOCK_TRACK_DEFAULT_SETTLE)) {
            (void)uberclock_channel_set_phase_down(channel_index, original_phase_increment);
            uberclock_commit_config();
            return -1;
        }
        if (!uberclock_fft_execute(sample_count)) {
            (void)uberclock_channel_set_phase_down(channel_index, original_phase_increment);
            uberclock_commit_config();
            return -1;
        }

        left_bin = (unsigned)((((uint64_t)(center_hz - delta_hz) * (uint64_t)sample_count) +
                               (uberclock_fft_sample_rate() / 2u)) /
                              (uint64_t)uberclock_fft_sample_rate());
        center_bin = (unsigned)((((uint64_t)center_hz * (uint64_t)sample_count) +
                                 (uberclock_fft_sample_rate() / 2u)) /
                                (uint64_t)uberclock_fft_sample_rate());
        right_bin = (unsigned)((((uint64_t)(center_hz + delta_hz) * (uint64_t)sample_count) +
                                (uberclock_fft_sample_rate() / 2u)) /
                               (uint64_t)uberclock_fft_sample_rate());

        if (right_bin >= (sample_count / 2u)) {
            puts("track3 expected bins exceed FFT Nyquist range");
            (void)uberclock_channel_set_phase_down(channel_index, original_phase_increment);
            uberclock_commit_config();
            return -1;
        }

        left_power = uberclock_fft_band_power(left_bin, UBERCLOCK_TRACK_DEFAULT_BAND_BINS, sample_count);
        center_power = uberclock_fft_band_power(center_bin, UBERCLOCK_TRACK_DEFAULT_BAND_BINS, sample_count);
        right_power = uberclock_fft_band_power(right_bin, UBERCLOCK_TRACK_DEFAULT_BAND_BINS, sample_count);

        printf("track3 step=%u phase_down_%u=%lu Hz inc=%lu bins={%u,%u,%u} pwr={%llu,%llu,%llu}\n",
               step_index,
               channel_index + 1u,
               (unsigned long)sweep_hz,
               (unsigned long)phase_increment,
               left_bin,
               center_bin,
               right_bin,
               (unsigned long long)left_power,
               (unsigned long long)center_power,
               (unsigned long long)right_power);

        if (triplet_matches(left_power, center_power, right_power)) {
            printf("track3 lock: phase_down_%u=%lu Hz inc=%lu center=%lu left=%lu right=%lu\n",
                   channel_index + 1u,
                   (unsigned long)sweep_hz,
                   (unsigned long)phase_increment,
                   (unsigned long)center_hz,
                   (unsigned long)(center_hz - delta_hz),
                   (unsigned long)(center_hz + delta_hz));
            return 0;
        }

        uberclock_runtime_wait_ticks(UBERCLOCK_TRACK_STEP_WAIT_TICKS);
        sweep_hz += step_hz;
    }

    (void)uberclock_channel_set_phase_down(channel_index, original_phase_increment);
    uberclock_commit_config();
    printf("track3 no lock found in %u steps; restored phase_down_%u=%lu Hz\n",
           max_steps,
           channel_index + 1u,
           (unsigned long)uberclock_phase_inc_to_hz(original_phase_increment, UBERCLOCK_TRACK_RF_FS_HZ));
    return -1;
}

int uberclock_trackq_start(uint32_t ch1_hz,
                           uint32_t ch2_hz,
                           uint32_t ch3_hz,
                           unsigned sample_count,
                           uint32_t center_hz,
                           uint32_t delta_ch1_hz,
                           uint32_t delta_ch2_hz,
                           uint32_t delta_ch3_hz) {
    struct uberclock_track_state *track = uberclock_track_state();
    uint32_t delta[UBERCLOCK_TRACK_CHANNEL_COUNT];
    uint32_t start_hz[UBERCLOCK_TRACK_CHANNEL_COUNT];
    unsigned channel_index;

    delta[0] = delta_ch1_hz ? delta_ch1_hz : track_default_delta_hz(0u);
    delta[1] = delta_ch2_hz ? delta_ch2_hz : track_default_delta_hz(1u);
    delta[2] = delta_ch3_hz ? delta_ch3_hz : track_default_delta_hz(2u);
    start_hz[0] = ch1_hz;
    start_hz[1] = ch2_hz;
    start_hz[2] = ch3_hz;

    for (channel_index = 0u; channel_index < UBERCLOCK_TRACK_CHANNEL_COUNT; ++channel_index) {
        if (delta[channel_index] == 0u) {
            puts("trackq_start requires delta_hz > 0");
            return -1;
        }
        if (!validate_tracking_request("trackq_start", sample_count, center_hz, delta[channel_index])) {
            return -1;
        }
    }

    for (channel_index = 0u; channel_index < UBERCLOCK_TRACK_CHANNEL_COUNT; ++channel_index) {
        uberclock_siggen_set_channel_symmetric(channel_index, center_hz, delta[channel_index]);
        uberclock_siggen_enable_channel(channel_index);
        if (start_hz[channel_index] != 0u) {
            (void)uberclock_channel_set_phase_down(
                channel_index,
                uberclock_phase_inc_from_hz(start_hz[channel_index], UBERCLOCK_TRACK_RF_FS_HZ));
        }
    }
    if (ch1_hz || ch2_hz || ch3_hz) {
        uberclock_commit_config();
    }

    for (channel_index = 0u; channel_index < UBERCLOCK_TRACK_CHANNEL_COUNT; ++channel_index) {
        track[channel_index].enabled = 1;
        track[channel_index].channel = channel_index;
        track[channel_index].n = sample_count;
        track[channel_index].settle = UBERCLOCK_TRACK_DEFAULT_SETTLE;
        track[channel_index].center_hz = center_hz;
        track[channel_index].delta_hz = delta[channel_index];
        track[channel_index].next_tick = uberclock_runtime_state()->ce_ticks + UBERCLOCK_TRACK_INTERVAL_TICKS;
        track[channel_index].filtered_error_mhz = 0;
        track[channel_index].step_accumulator_mhz = 0;
    }

    printf("trackq_start: ch1=%lu Hz ch2=%lu Hz ch3=%lu Hz N=%u center=%lu Hz delta={%lu,%lu,%lu} Hz sig3={{%lu,%lu,%lu},{%lu,%lu,%lu},{%lu,%lu,%lu}} Hz interval=2 s\n",
           (unsigned long)uberclock_phase_inc_to_hz(uberclock_channel_get_phase_down(0u), UBERCLOCK_TRACK_RF_FS_HZ),
           (unsigned long)uberclock_phase_inc_to_hz(uberclock_channel_get_phase_down(1u), UBERCLOCK_TRACK_RF_FS_HZ),
           (unsigned long)uberclock_phase_inc_to_hz(uberclock_channel_get_phase_down(2u), UBERCLOCK_TRACK_RF_FS_HZ),
           sample_count,
           (unsigned long)center_hz,
           (unsigned long)delta[0],
           (unsigned long)delta[1],
           (unsigned long)delta[2],
           (unsigned long)(center_hz - delta[0]), (unsigned long)center_hz, (unsigned long)(center_hz + delta[0]),
           (unsigned long)(center_hz - delta[1]), (unsigned long)center_hz, (unsigned long)(center_hz + delta[1]),
           (unsigned long)(center_hz - delta[2]), (unsigned long)center_hz, (unsigned long)(center_hz + delta[2]));
    return 0;
}

int uberclock_trackq_probe(unsigned sample_count, uint32_t center_hz, uint32_t delta_hz) {
    uint64_t left_power;
    uint64_t center_power;
    uint64_t right_power;
    int64_t numerator;
    int64_t denominator;
    int64_t center_hz_milli;
    int64_t delta_hz_milli;
    int64_t vertex_hz_milli;

    if (!validate_tracking_request("trackq_probe", sample_count, center_hz, delta_hz)) {
        return -1;
    }

    uberclock_ds_fifo_clear_status();
    (void)uberclock_ds_fifo_flush(sample_count + UBERCLOCK_TRACK_DEFAULT_SETTLE);
    uberclock_runtime_wait_ticks(sample_count + UBERCLOCK_TRACK_DEFAULT_SETTLE);

    if (!capture_ds_fft_channel(0u, sample_count, UBERCLOCK_TRACK_DEFAULT_SETTLE)) {
        puts("trackq_probe capture failed");
        return -1;
    }

    left_power = uberclock_compute_band_power_at_hz(center_hz - delta_hz, sample_count);
    center_power = uberclock_compute_band_power_at_hz(center_hz, sample_count);
    right_power = uberclock_compute_band_power_at_hz(center_hz + delta_hz, sample_count);

    center_hz_milli = (int64_t)center_hz * 1000ll;
    vertex_hz_milli = center_hz_milli;
    if (triplet_matches(left_power, center_power, right_power)) {
        numerator = (int64_t)left_power - (int64_t)right_power;
        denominator = 2ll * ((int64_t)left_power - (2ll * (int64_t)center_power) + (int64_t)right_power);
        if (denominator < 0ll) {
            delta_hz_milli = (int64_t)delta_hz * 1000ll;
            vertex_hz_milli = center_hz_milli + ((delta_hz_milli * numerator) / denominator);
        }
    }

    printf("trackq_probe: phase_down_1=%lu Hz N=%u center=%lu Hz delta=%lu Hz pwr={%llu,%llu,%llu} vertex=%ld.%03ldHz\n",
           (unsigned long)uberclock_phase_inc_to_hz(uberclock_channel_get_phase_down(0u), UBERCLOCK_TRACK_RF_FS_HZ),
           sample_count,
           (unsigned long)center_hz,
           (unsigned long)delta_hz,
           (unsigned long long)left_power,
           (unsigned long long)center_power,
           (unsigned long long)right_power,
           (long)(vertex_hz_milli / 1000ll),
           (long)labs(vertex_hz_milli % 1000ll));
    return 0;
}
