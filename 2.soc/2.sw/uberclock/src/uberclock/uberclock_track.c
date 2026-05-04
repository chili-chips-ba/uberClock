#include <stdio.h>
#include <stdlib.h>
#include "uberclock/uberclock_config.h"
#include "uberclock/uberclock_runtime.h"
#include "uberclock/uberclock_fft.h"
#include "uberclock/uberclock_fifo.h"
#include "uberclock/uberclock_hw.h"
#include "uberclock/uberclock_channels.h"
#include "uberclock/uberclock_track.h"

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

void uberclock_track_poll(void) {
    struct uberclock_track_state *track = uberclock_track_state();
    uint64_t left_power;
    uint64_t center_power;
    uint64_t right_power;
    int64_t numerator;
    int64_t denominator;
    int64_t center_hz_milli;
    int64_t delta_hz_milli;
    int64_t vertex_hz_milli;
    int32_t error_mhz;
    int32_t correction_hz;
    int32_t applied_hz;
    int64_t filter_delta_mhz;
    int64_t control_mhz;
    uint32_t phase_hz;
    const char *mode;
    int weak_mode;

    if (!track->enabled) {
        return;
    }
    if (uberclock_fft_sample_rate() == 0u) {
        puts("trackq stopped: fft_fs must be > 0");
        track->enabled = 0;
        return;
    }
    if (uberclock_runtime_state()->ce_ticks < track->next_tick) {
        return;
    }
    if (track->center_hz <= track->delta_hz) {
        puts("trackq stopped: center_hz must exceed delta_hz");
        track->enabled = 0;
        return;
    }
    if (!uberclock_fft_capture_ds_ch1_real(track->n, track->settle)) {
        track->enabled = 0;
        return;
    }

    left_power = uberclock_compute_band_power_at_hz(track->center_hz - track->delta_hz, track->n);
    center_power = uberclock_compute_band_power_at_hz(track->center_hz, track->n);
    right_power = uberclock_compute_band_power_at_hz(track->center_hz + track->delta_hz, track->n);

    center_hz_milli = (int64_t)track->center_hz * 1000ll;
    vertex_hz_milli = center_hz_milli;
    error_mhz = 0;
    correction_hz = 0;
    mode = "hold";
    weak_mode = 0;

    if (vertex_is_confident(left_power, center_power, right_power)) {
        numerator = (int64_t)left_power - (int64_t)right_power;
        denominator = 2ll * ((int64_t)left_power - (2ll * (int64_t)center_power) + (int64_t)right_power);
        if (denominator < 0ll) {
            delta_hz_milli = (int64_t)track->delta_hz * 1000ll;
            vertex_hz_milli = center_hz_milli + ((delta_hz_milli * numerator) / denominator);
            error_mhz = (int32_t)(vertex_hz_milli - center_hz_milli);
            mode = "reg";
        }
    } else {
        error_mhz = estimate_side_error_mhz(left_power, right_power, track->delta_hz);
        if (error_mhz > 0) {
            mode = "weak+";
            weak_mode = 1;
        } else if (error_mhz < 0) {
            mode = "weak-";
            weak_mode = 1;
        } else {
            track->filtered_error_mhz =
                (track->filtered_error_mhz *
                 (UBERCLOCK_TRACK_ERR_ALPHA_DEN - UBERCLOCK_TRACK_ERR_ALPHA_NUM)) /
                UBERCLOCK_TRACK_ERR_ALPHA_DEN;
            mode = "weak";
        }
    }

    filter_delta_mhz =
        ((int64_t)(error_mhz - track->filtered_error_mhz) * (int64_t)UBERCLOCK_TRACK_ERR_ALPHA_NUM) /
        (int64_t)UBERCLOCK_TRACK_ERR_ALPHA_DEN;
    track->filtered_error_mhz += (int32_t)filter_delta_mhz;

    control_mhz =
        ((int64_t)track->filtered_error_mhz * (int64_t)UBERCLOCK_TRACK_KP_NUM) /
        (int64_t)UBERCLOCK_TRACK_KP_DEN;
    track->step_accumulator_mhz += (int32_t)control_mhz;
    correction_hz = track->step_accumulator_mhz / 1000;
    correction_hz = weak_mode ? clamp_weak_tracking_step_hz(correction_hz)
                              : clamp_tracking_step_hz(correction_hz);
    track->step_accumulator_mhz -= correction_hz * 1000;

    phase_hz = uberclock_phase_inc_to_hz(uberclock_channel_get_phase_down(0u), UBERCLOCK_TRACK_RF_FS_HZ);
    applied_hz = correction_hz;
    phase_hz = (uint32_t)((int32_t)phase_hz + applied_hz);
    (void)uberclock_channel_set_phase_down(0u, uberclock_phase_inc_from_hz(phase_hz, UBERCLOCK_TRACK_RF_FS_HZ));
    uberclock_commit_config();

    printf("trackq: pwr={%llu,%llu,%llu} vertex=%ld.%03ldHz corr=%ldHz mode=%s phase_down_1=%luHz\n",
           (unsigned long long)left_power,
           (unsigned long long)center_power,
           (unsigned long long)right_power,
           (long)(vertex_hz_milli / 1000ll),
           (long)labs(vertex_hz_milli % 1000ll),
           (long)applied_hz,
           mode,
           (unsigned long)phase_hz);

    track->next_tick = uberclock_runtime_state()->ce_ticks + UBERCLOCK_TRACK_INTERVAL_TICKS;
}

void uberclock_track_stop(void) {
    uberclock_track_state()->enabled = 0;
    puts("trackq_stop: quadratic tracking disabled");
}

int uberclock_track3_run(uint32_t start_hz,
                         uint32_t step_hz,
                         unsigned max_steps,
                         unsigned sample_count,
                         uint32_t center_hz,
                         uint32_t delta_hz) {
    uint32_t original_phase_increment;
    uint32_t sweep_hz;
    unsigned step_index;

    if (step_hz == 0u || max_steps == 0u) {
        puts("track3 requires step_hz > 0 and max_steps > 0");
        return -1;
    }
    if (!validate_tracking_request("track3", sample_count, center_hz, delta_hz)) {
        return -1;
    }

    original_phase_increment = uberclock_channel_get_phase_down(0u);
    sweep_hz = start_hz;
    printf("track3: start=%lu Hz step=%lu Hz max_steps=%u N=%u center=%lu Hz delta=%lu Hz Fs=%lu Hz\n",
           (unsigned long)start_hz,
           (unsigned long)step_hz,
           max_steps,
           sample_count,
           (unsigned long)center_hz,
           (unsigned long)delta_hz,
           (unsigned long)uberclock_fft_sample_rate());

    for (step_index = 0u; step_index < max_steps; ++step_index) {
        uint32_t phase_increment = uberclock_phase_inc_from_hz(sweep_hz, UBERCLOCK_TRACK_RF_FS_HZ);
        unsigned left_bin;
        unsigned center_bin;
        unsigned right_bin;
        uint64_t left_power;
        uint64_t center_power;
        uint64_t right_power;

        (void)uberclock_channel_set_phase_down(0u, phase_increment);
        uberclock_commit_config();

        if (!uberclock_fft_capture_ds_ch1_real(sample_count, UBERCLOCK_TRACK_DEFAULT_SETTLE)) {
            (void)uberclock_channel_set_phase_down(0u, original_phase_increment);
            uberclock_commit_config();
            return -1;
        }
        if (!uberclock_fft_execute(sample_count)) {
            (void)uberclock_channel_set_phase_down(0u, original_phase_increment);
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
            (void)uberclock_channel_set_phase_down(0u, original_phase_increment);
            uberclock_commit_config();
            return -1;
        }

        left_power = uberclock_fft_band_power(left_bin, UBERCLOCK_TRACK_DEFAULT_BAND_BINS, sample_count);
        center_power = uberclock_fft_band_power(center_bin, UBERCLOCK_TRACK_DEFAULT_BAND_BINS, sample_count);
        right_power = uberclock_fft_band_power(right_bin, UBERCLOCK_TRACK_DEFAULT_BAND_BINS, sample_count);

        printf("track3 step=%u phase_down_1=%lu Hz inc=%lu bins={%u,%u,%u} pwr={%llu,%llu,%llu}\n",
               step_index,
               (unsigned long)sweep_hz,
               (unsigned long)phase_increment,
               left_bin,
               center_bin,
               right_bin,
               (unsigned long long)left_power,
               (unsigned long long)center_power,
               (unsigned long long)right_power);

        if (triplet_matches(left_power, center_power, right_power)) {
            printf("track3 lock: phase_down_1=%lu Hz inc=%lu center=%lu left=%lu right=%lu\n",
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

    (void)uberclock_channel_set_phase_down(0u, original_phase_increment);
    uberclock_commit_config();
    printf("track3 no lock found in %u steps; restored phase_down_1=%lu Hz\n",
           max_steps,
           (unsigned long)uberclock_phase_inc_to_hz(original_phase_increment, UBERCLOCK_TRACK_RF_FS_HZ));
    return -1;
}

int uberclock_trackq_start(unsigned sample_count, uint32_t center_hz, uint32_t delta_hz) {
    struct uberclock_track_state *track = uberclock_track_state();

    if (!validate_tracking_request("trackq_start", sample_count, center_hz, delta_hz)) {
        return -1;
    }

    track->enabled = 1;
    track->n = sample_count;
    track->settle = UBERCLOCK_TRACK_DEFAULT_SETTLE;
    track->center_hz = center_hz;
    track->delta_hz = delta_hz;
    track->next_tick = uberclock_runtime_state()->ce_ticks + UBERCLOCK_TRACK_INTERVAL_TICKS;
    track->filtered_error_mhz = 0;
    track->step_accumulator_mhz = 0;

    printf("trackq_start: phase_down_1=%lu Hz N=%u center=%lu Hz delta=%lu Hz interval=1 s\n",
           (unsigned long)uberclock_phase_inc_to_hz(uberclock_channel_get_phase_down(0u), UBERCLOCK_TRACK_RF_FS_HZ),
           sample_count,
           (unsigned long)center_hz,
           (unsigned long)delta_hz);
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

    if (!uberclock_fft_capture_ds_ch1_real(sample_count, UBERCLOCK_TRACK_DEFAULT_SETTLE)) {
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
