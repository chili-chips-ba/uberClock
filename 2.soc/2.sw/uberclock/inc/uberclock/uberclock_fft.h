/**
 * @file uberclock_fft.h
 * @brief FFT and spectral analysis interface.
 */

#ifndef UBERCLOCK_FFT_H
#define UBERCLOCK_FFT_H

#include <stdint.h>

/** Check if value is power of two */
int uberclock_fft_is_power_of_two(unsigned value);

/** Set FFT sample rate */
void uberclock_fft_set_sample_rate(uint32_t sample_rate_hz);

/** Get FFT sample rate */
uint32_t uberclock_fft_sample_rate(void);

/** Capture I/Q samples */
int uberclock_fft_capture_ds_iq(unsigned sample_count);

/** Capture Y32 samples */
int uberclock_fft_capture_ds_y32(void);

/** Capture real channel samples */
int uberclock_fft_capture_ds_ch1_real(unsigned sample_count, unsigned settle_samples);

/** Execute FFT */
int uberclock_fft_execute(unsigned sample_count);

/** Compute bin power */
uint64_t uberclock_fft_bin_power(unsigned bin_index);

/** Compute band power */
uint64_t uberclock_fft_band_power(unsigned bin_index, unsigned half_bins, unsigned sample_count);

/** Compute power at frequency */
uint64_t uberclock_compute_power_at_hz(uint32_t frequency_hz, unsigned sample_count);

/** Compute band power at frequency */
uint64_t uberclock_compute_band_power_at_hz(uint32_t frequency_hz, unsigned sample_count);

#endif
