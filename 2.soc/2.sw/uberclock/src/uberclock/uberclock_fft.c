#include <stdio.h>
#include <stdint.h>
#include "uberclock/uberclock_fft.h"
#include "uberclock/uberclock_runtime.h"
#include "uberclock/uberclock_fifo.h"

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

int uberclock_fft_is_power_of_two(unsigned value) {
    return (value != 0u) && ((value & (value - 1u)) == 0u);
}

void uberclock_fft_set_sample_rate(uint32_t sample_rate_hz) {
    uberclock_fft_context()->sample_rate_hz = sample_rate_hz;
}

uint32_t uberclock_fft_sample_rate(void) {
    return uberclock_fft_context()->sample_rate_hz;
}

int uberclock_fft_capture_ds_iq(unsigned sample_count) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    unsigned sample_index;
    int16_t sample_x;
    int16_t sample_y;

    for (sample_index = 0u; sample_index < sample_count; ++sample_index) {
        if (!uberclock_ds_fifo_pop_simple(&sample_x, &sample_y)) {
            printf("Not enough DS FIFO samples: got %u/%u\n", sample_index, sample_count);
            return 0;
        }

        fft->fft_in[sample_index].r = (kiss_fft_scalar)sample_x;
        fft->fft_in[sample_index].i = (kiss_fft_scalar)sample_y;
    }

    return 1;
}

int uberclock_fft_capture_ds_y32(void) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    unsigned sample_index;
    int16_t sample_x;
    int16_t sample_y;

    for (sample_index = 0u; sample_index < 32u; ++sample_index) {
        if (!uberclock_ds_fifo_pop_simple(&sample_x, &sample_y)) {
            printf("Not enough DS FIFO samples: got %u/%u\n", sample_index, 32u);
            return 0;
        }

        fft->fft_in[sample_index].r = (kiss_fft_scalar)sample_y;
        fft->fft_in[sample_index].i = (kiss_fft_scalar)0;
    }

    return 1;
}

int uberclock_fft_capture_ds_ch1_real(unsigned sample_count, unsigned settle_samples) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    unsigned sample_index;
    int16_t sample_x;
    int16_t sample_y;

    for (sample_index = 0u; sample_index < settle_samples; ++sample_index) {
        if (!uberclock_wait_for_ds_sample("settle", sample_index, settle_samples)) {
            return 0;
        }
        (void)uberclock_ds_fifo_pop_capture(&sample_x, &sample_y);
        uberclock_runtime_service_ce_events(4u);
    }

    for (sample_index = 0u; sample_index < sample_count; ++sample_index) {
        if (!uberclock_wait_for_ds_sample("capture", sample_index, sample_count)) {
            return 0;
        }

        (void)uberclock_ds_fifo_pop_capture(&sample_x, &sample_y);
        fft->fft_in[sample_index].r = (kiss_fft_scalar)sample_x;
        fft->fft_in[sample_index].i = (kiss_fft_scalar)0;
        uberclock_runtime_service_ce_events(4u);
    }

    return 1;
}

int uberclock_fft_execute(unsigned sample_count) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    size_t config_need = 0u;
    size_t config_length;
    kiss_fft_cfg config;

    (void)kiss_fft_alloc((int)sample_count, 0, NULL, &config_need);
    if (config_need > (size_t)UBERCLOCK_FFT_CFG_MAX_BYTES) {
        printf("fft cfg too big: need %lu bytes (max %u)\n",
               (unsigned long)config_need,
               UBERCLOCK_FFT_CFG_MAX_BYTES);
        return 0;
    }

    config_length = (size_t)UBERCLOCK_FFT_CFG_MAX_BYTES;
    config = kiss_fft_alloc((int)sample_count, 0, fft->cfg_mem, &config_length);
    if (!config) {
        puts("kiss_fft_alloc failed");
        return 0;
    }

    kiss_fft(config, fft->fft_in, fft->fft_out);
    return 1;
}

uint64_t uberclock_fft_bin_power(unsigned bin_index) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    int32_t real_part = (int32_t)fft->fft_out[bin_index].r;
    int32_t imag_part = (int32_t)fft->fft_out[bin_index].i;

    return (uint64_t)((int64_t)real_part * real_part) + (uint64_t)((int64_t)imag_part * imag_part);
}

uint64_t uberclock_fft_band_power(unsigned bin_index, unsigned half_bins, unsigned sample_count) {
    uint64_t power = 0u;
    unsigned first_bin = (bin_index > half_bins) ? (bin_index - half_bins) : 0u;
    unsigned last_bin = bin_index + half_bins;
    unsigned nyquist_bins = sample_count / 2u;
    unsigned current_bin;

    if (last_bin >= nyquist_bins) {
        last_bin = nyquist_bins - 1u;
    }

    for (current_bin = first_bin; current_bin <= last_bin; ++current_bin) {
        power += uberclock_fft_bin_power(current_bin);
    }

    return power;
}

uint64_t uberclock_compute_power_at_hz(uint32_t frequency_hz, unsigned sample_count) {
    struct uberclock_fft_context *fft = uberclock_fft_context();
    uint32_t phase = 0u;
    uint32_t phase_increment = (uint32_t)(((uint64_t)frequency_hz << 32) / fft->sample_rate_hz);
    int64_t accumulator_i = 0;
    int64_t accumulator_q = 0;
    unsigned sample_index;

    for (sample_index = 0u; sample_index < sample_count; ++sample_index) {
        int32_t sample = (int32_t)fft->fft_in[sample_index].r;
        int32_t cosine_q15 = (int32_t)sine_lookup(phase + 0x40000000u);
        int32_t sine_q15 = (int32_t)sine_lookup(phase);

        accumulator_i += ((int64_t)sample * (int64_t)cosine_q15) >> UBERCLOCK_TRACK_CORR_SHIFT;
        accumulator_q -= ((int64_t)sample * (int64_t)sine_q15) >> UBERCLOCK_TRACK_CORR_SHIFT;
        phase += phase_increment;

        if ((sample_index & (UBERCLOCK_TRACK_BG_SERVICE_PERIOD - 1u)) == 0u) {
            uberclock_runtime_service_ce_events(1u);
        }
    }

    return (uint64_t)(accumulator_i * accumulator_i) + (uint64_t)(accumulator_q * accumulator_q);
}

uint64_t uberclock_compute_band_power_at_hz(uint32_t frequency_hz, unsigned sample_count) {
    uint64_t center_power = uberclock_compute_power_at_hz(frequency_hz, sample_count);
    uint32_t frequency_resolution_hz;
    uint64_t lower_power;
    uint64_t upper_power;

    if (uberclock_fft_sample_rate() == 0u || sample_count == 0u) {
        return center_power;
    }

    frequency_resolution_hz = (uint32_t)((((uint64_t)uberclock_fft_sample_rate()) + (sample_count / 2u)) /
    (uint64_t)sample_count);
    if (frequency_resolution_hz == 0u) {
        return center_power;
    }

    lower_power = uberclock_compute_power_at_hz(
        (frequency_hz > frequency_resolution_hz) ? (frequency_hz - frequency_resolution_hz) : 0u,
                                                sample_count);
    upper_power = uberclock_compute_power_at_hz(frequency_hz + frequency_resolution_hz, sample_count);

    return center_power + ((lower_power + upper_power) >> 1);
}
