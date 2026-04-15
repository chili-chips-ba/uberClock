#ifndef UBERCLOCK_FFT_H
#define UBERCLOCK_FFT_H

#include <stdint.h>

int uberclock_fft_is_power_of_two(unsigned value);
void uberclock_fft_set_sample_rate(uint32_t sample_rate_hz);
uint32_t uberclock_fft_sample_rate(void);

int uberclock_fft_capture_ds_iq(unsigned sample_count);
int uberclock_fft_capture_ds_y32(void);
int uberclock_fft_capture_ds_ch1_real(unsigned sample_count, unsigned settle_samples);
int uberclock_fft_execute(unsigned sample_count);
uint64_t uberclock_fft_bin_power(unsigned bin_index);
uint64_t uberclock_fft_band_power(unsigned bin_index, unsigned half_bins, unsigned sample_count);
uint64_t uberclock_compute_power_at_hz(uint32_t frequency_hz, unsigned sample_count);
uint64_t uberclock_compute_band_power_at_hz(uint32_t frequency_hz, unsigned sample_count);

#endif

