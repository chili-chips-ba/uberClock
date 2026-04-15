/**
 * @file uberclock_hw.h
 * @brief Low-level hardware / CSR interface.
 */

#ifndef UBERCLOCK_HW_H
#define UBERCLOCK_HW_H

#include <stdint.h>

/** Commit configuration to hardware */
void uberclock_commit_config(void);

/** Convert frequency to phase increment */
uint32_t uberclock_phase_inc_from_hz(uint32_t frequency_hz, uint32_t sample_rate_hz);

/** Convert phase increment to frequency */
uint32_t uberclock_phase_inc_to_hz(uint32_t phase_increment, uint32_t sample_rate_hz);

/** Set NCO phase */
void uberclock_set_nco_phase_increment(uint32_t phase_increment);

/** Set NCO magnitude */
void uberclock_set_nco_magnitude(int16_t magnitude);

/** Set reference phase */
void uberclock_set_phase_down_reference(uint32_t phase_increment);

/** Routing / selection controls */
void uberclock_set_input_select(uint32_t input_select);
void uberclock_set_upsampler_input_mux(uint32_t mux_select);
void uberclock_set_output_select_ch1(uint32_t output_select);
void uberclock_set_output_select_ch2(uint32_t output_select);

/** Debug selection */
void uberclock_set_lowspeed_debug_select(uint32_t debug_select);
void uberclock_set_highspeed_debug_select(uint32_t debug_select);

/** Final output scaling */
void uberclock_set_final_shift(int32_t final_shift);

/** Inject samples */
void uberclock_set_upsampler_input_x(int16_t sample);
void uberclock_set_upsampler_input_y(int16_t sample);

/** Capture control */
void uberclock_set_capture_enable(unsigned enabled);
unsigned uberclock_get_capture_enable(void);
void uberclock_set_capture_beats(uint32_t beats);
void uberclock_capture_arm_pulse(void);
unsigned uberclock_capture_done(void);
int16_t uberclock_capture_read_sample(unsigned index);

#endif
